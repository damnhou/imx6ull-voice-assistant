#ifndef IFLYTEKIATCLIENT_H
#define IFLYTEKIATCLIENT_H

#include "cloud/iatresultparser.h"

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

struct IatOptions
{
    QUrl endpoint;
    QString appId;
    QString apiKey;
    QString apiSecret;
    QString language = QStringLiteral("zh_cn");
    QString accent = QStringLiteral("mandarin");
    QString domain = QStringLiteral("iat");
    bool dynamicCorrection = true;
    int vadEosMs = 3000;
    int sampleRate = 16000;
    int frameIntervalMs = 40;
    bool mockMode = true;
};

class IflytekIatClient : public QObject
{
    Q_OBJECT
public:
    enum class State { Idle, Connecting, Streaming, Finishing, Finished, Error };
    Q_ENUM(State)

    explicit IflytekIatClient(QObject *parent = nullptr);

    void configure(const IatOptions &options);
    State state() const;

public slots:
    void startRecognition();
    void appendAudio(const QByteArray &pcm);
    void finishRecognition();
    void abortRecognition();

signals:
    void stateChanged(IflytekIatClient::State state);
    void partialResult(const QString &text);
    void finalResult(const QString &text);
    void errorOccurred(const QString &message);
    void diagnostic(const QString &message);

private slots:
    void onConnected();
    void onTextMessage(const QString &message);
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void sendNextFrame();
    void advanceMockResult();

private:
    void setState(State state);
    void sendFrame(int status, const QByteArray &pcm);
    int bytesPerFrame() const;
    void fail(const QString &message);

    IatOptions m_options;
    QWebSocket m_socket;
    QTimer m_frameTimer;
    QTimer m_mockTimer;
    IatResultParser m_parser;
    QByteArray m_pendingAudio;
    State m_state = State::Idle;
    bool m_firstFrame = true;
    bool m_finishRequested = false;
    bool m_finalFrameSent = false;
    int m_mockStep = 0;
};

#endif // IFLYTEKIATCLIENT_H

