#include "cloud/iflytekiatclient.h"
#include "cloud/iflytekauth.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSslError>

IflytekIatClient::IflytekIatClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected, this, &IflytekIatClient::onConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &IflytekIatClient::onTextMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, &IflytekIatClient::onDisconnected);
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &IflytekIatClient::onSocketError);
    connect(&m_socket, &QWebSocket::sslErrors, this,
            [this](const QList<QSslError> &errors) {
        QStringList descriptions;
        for (const QSslError &error : errors)
            descriptions << error.errorString();
        fail(QStringLiteral("TLS 证书校验失败：%1。请在根文件系统安装 CA 证书，禁止跳过校验。")
             .arg(descriptions.join(QStringLiteral("；"))));
    });

    m_frameTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_frameTimer, &QTimer::timeout, this, &IflytekIatClient::sendNextFrame);
    connect(&m_mockTimer, &QTimer::timeout, this, &IflytekIatClient::advanceMockResult);
    m_mockTimer.setInterval(550);
}

void IflytekIatClient::configure(const IatOptions &options)
{
    m_options = options;
    m_frameTimer.setInterval(options.frameIntervalMs);
}

IflytekIatClient::State IflytekIatClient::state() const
{
    return m_state;
}

void IflytekIatClient::startRecognition()
{
    if (m_state == State::Connecting || m_state == State::Streaming || m_state == State::Finishing)
        return;

    m_pendingAudio.clear();
    m_parser.reset();
    m_firstFrame = true;
    m_finishRequested = false;
    m_finalFrameSent = false;
    m_mockStep = 0;

    if (m_options.mockMode) {
        setState(State::Streaming);
        emit diagnostic(QStringLiteral("演示模式：不建立云端连接"));
        m_mockTimer.start();
        return;
    }

    if (!m_options.endpoint.isValid() || m_options.endpoint.scheme() != QStringLiteral("wss")
            || m_options.appId.isEmpty() || m_options.apiKey.isEmpty() || m_options.apiSecret.isEmpty()) {
        fail(QStringLiteral("讯飞 WebAPI 配置不完整"));
        return;
    }

    setState(State::Connecting);
    const QUrl url = IflytekAuth::signedUrl(m_options.endpoint, m_options.apiKey, m_options.apiSecret);
    emit diagnostic(QStringLiteral("正在连接 %1（鉴权参数已隐藏）").arg(m_options.endpoint.host()));
    m_socket.open(url);
}

void IflytekIatClient::appendAudio(const QByteArray &pcm)
{
    if (m_state != State::Connecting && m_state != State::Streaming)
        return;
    m_pendingAudio.append(pcm);
    const int maximumBufferedBytes = m_options.sampleRate * 2 * 5;
    if (m_pendingAudio.size() > maximumBufferedBytes) {
        m_pendingAudio.remove(0, m_pendingAudio.size() - maximumBufferedBytes);
        emit diagnostic(QStringLiteral("网络发送滞后，已限制音频缓存为 5 秒"));
    }
}

void IflytekIatClient::finishRecognition()
{
    if (m_options.mockMode) {
        if (m_state == State::Streaming) {
            m_mockTimer.stop();
            emit finalResult(QStringLiteral("打开开发板指示灯"));
            setState(State::Finished);
        }
        return;
    }

    if (m_state == State::Connecting || m_state == State::Streaming) {
        m_finishRequested = true;
        setState(State::Finishing);
        if (!m_frameTimer.isActive() && m_socket.state() == QAbstractSocket::ConnectedState)
            m_frameTimer.start();
    }
}

void IflytekIatClient::abortRecognition()
{
    m_frameTimer.stop();
    m_mockTimer.stop();
    m_pendingAudio.clear();
    m_socket.abort();
    setState(State::Idle);
}

void IflytekIatClient::onConnected()
{
    emit diagnostic(QStringLiteral("WebSocket 握手成功"));
    setState(State::Streaming);
    m_frameTimer.start();
}

void IflytekIatClient::onTextMessage(const QString &message)
{
    const IatResultParser::ParseResult result = m_parser.consume(message.toUtf8());
    if (!result.ok) {
        fail(result.error);
        return;
    }

    emit partialResult(result.text);
    if (result.finalFrame) {
        m_frameTimer.stop();
        emit finalResult(result.text);
        emit diagnostic(QStringLiteral("识别完成，sid=%1").arg(result.sid));
        setState(State::Finished);
        m_socket.close();
    }
}

void IflytekIatClient::onDisconnected()
{
    m_frameTimer.stop();
    if (m_state != State::Finished && m_state != State::Idle && m_state != State::Error)
        fail(QStringLiteral("WebSocket 在最终结果返回前断开"));
}

void IflytekIatClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_state != State::Idle && m_state != State::Finished)
        fail(QStringLiteral("WebSocket 错误：%1").arg(m_socket.errorString()));
}

void IflytekIatClient::sendNextFrame()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;

    const int frameBytes = bytesPerFrame();
    if (!m_pendingAudio.isEmpty()) {
        const int amount = qMin(frameBytes, m_pendingAudio.size());
        const QByteArray frame = m_pendingAudio.left(amount);
        m_pendingAudio.remove(0, amount);
        sendFrame(m_firstFrame ? 0 : 1, frame);
        m_firstFrame = false;
        return;
    }

    if (m_finishRequested && !m_finalFrameSent) {
        if (m_firstFrame) {
            sendFrame(0, QByteArray());
            m_firstFrame = false;
        }
        sendFrame(2, QByteArray());
        m_finalFrameSent = true;
        m_frameTimer.stop();
    }
}

void IflytekIatClient::advanceMockResult()
{
    static const QStringList samples = {
        QStringLiteral("打开"),
        QStringLiteral("打开开发板"),
        QStringLiteral("打开开发板指示灯")
    };
    if (m_mockStep < samples.size()) {
        emit partialResult(samples.at(m_mockStep));
        ++m_mockStep;
    }
    if (m_mockStep >= samples.size()) {
        m_mockTimer.stop();
        emit finalResult(samples.last());
        setState(State::Finished);
    }
}

void IflytekIatClient::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void IflytekIatClient::sendFrame(int status, const QByteArray &pcm)
{
    QJsonObject data;
    data.insert(QStringLiteral("status"), status);
    data.insert(QStringLiteral("format"), QStringLiteral("audio/L16;rate=%1").arg(m_options.sampleRate));
    data.insert(QStringLiteral("encoding"), QStringLiteral("raw"));
    data.insert(QStringLiteral("audio"), QString::fromLatin1(pcm.toBase64()));

    QJsonObject root;
    if (status == 0) {
        root.insert(QStringLiteral("common"), QJsonObject{{QStringLiteral("app_id"), m_options.appId}});
        QJsonObject business;
        business.insert(QStringLiteral("language"), m_options.language);
        business.insert(QStringLiteral("domain"), m_options.domain);
        business.insert(QStringLiteral("accent"), m_options.accent);
        business.insert(QStringLiteral("vad_eos"), m_options.vadEosMs);
        if (m_options.dynamicCorrection)
            business.insert(QStringLiteral("dwa"), QStringLiteral("wpgs"));
        root.insert(QStringLiteral("business"), business);
    }
    root.insert(QStringLiteral("data"), data);
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

int IflytekIatClient::bytesPerFrame() const
{
    return m_options.sampleRate * 2 * m_options.frameIntervalMs / 1000;
}

void IflytekIatClient::fail(const QString &message)
{
    m_frameTimer.stop();
    m_mockTimer.stop();
    setState(State::Error);
    emit errorOccurred(message);
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
}

