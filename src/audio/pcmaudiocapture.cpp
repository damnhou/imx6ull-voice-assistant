#include "audio/pcmaudiocapture.h"

#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QtEndian>
#include <QtMath>

PcmAudioCapture::PcmAudioCapture(QObject *parent)
    : QObject(parent)
{
}

PcmAudioCapture::~PcmAudioCapture()
{
    stop();
}

void PcmAudioCapture::configure(int sampleRate, int channels, int sampleSize,
                                const QString &deviceName)
{
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_sampleSize = sampleSize;
    m_requestedDevice = deviceName;
}

bool PcmAudioCapture::isRunning() const
{
    return m_audioInput && m_audioInput->state() != QAudio::StoppedState;
}

QString PcmAudioCapture::selectedDeviceName() const
{
    return m_selectedDevice;
}

bool PcmAudioCapture::start()
{
    if (isRunning())
        return true;

    QAudioDeviceInfo device = QAudioDeviceInfo::defaultInputDevice();
    if (!m_requestedDevice.isEmpty()) {
        const QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        for (const QAudioDeviceInfo &candidate : devices) {
            if (candidate.deviceName() == m_requestedDevice) {
                device = candidate;
                break;
            }
        }
    }

    if (device.isNull()) {
        emit errorOccurred(QStringLiteral("没有检测到录音设备，请检查 USB 声卡和 ALSA 配置"));
        return false;
    }

    const QAudioFormat format = requestedFormat();
    if (!device.isFormatSupported(format)) {
        emit errorOccurred(QStringLiteral("设备 %1 不支持 16 kHz/16 bit/单声道 PCM；为避免上传错误，不自动改用近似格式")
                           .arg(device.deviceName()));
        return false;
    }

    m_selectedDevice = device.deviceName();
    m_audioInput = new QAudioInput(device, format, this);
    m_audioInput->setBufferSize(4096);
    connect(m_audioInput, &QAudioInput::stateChanged,
            this, &PcmAudioCapture::handleStateChanged);
    m_stream = m_audioInput->start();
    if (!m_stream) {
        emit errorOccurred(QStringLiteral("打开录音设备失败"));
        delete m_audioInput;
        m_audioInput = nullptr;
        return false;
    }

    connect(m_stream, &QIODevice::readyRead,
            this, &PcmAudioCapture::readAvailableAudio);
    emit runningChanged(true);
    return true;
}

void PcmAudioCapture::stop()
{
    if (!m_audioInput)
        return;
    disconnect(m_stream, nullptr, this, nullptr);
    m_audioInput->stop();
    m_audioInput->deleteLater();
    m_audioInput = nullptr;
    m_stream = nullptr;
    emit levelChanged(0.0);
    emit runningChanged(false);
}

void PcmAudioCapture::readAvailableAudio()
{
    if (!m_stream)
        return;
    const QByteArray data = m_stream->readAll();
    if (data.isEmpty())
        return;
    emit levelChanged(calculateLevel(data));
    emit pcmChunk(data);
}

void PcmAudioCapture::handleStateChanged(QAudio::State state)
{
    if (!m_audioInput)
        return;
    if (state == QAudio::StoppedState && m_audioInput->error() != QAudio::NoError)
        emit errorOccurred(QStringLiteral("录音流异常停止，QAudio 错误码 %1").arg(m_audioInput->error()));
}

QAudioFormat PcmAudioCapture::requestedFormat() const
{
    QAudioFormat format;
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleSize(m_sampleSize);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

qreal PcmAudioCapture::calculateLevel(const QByteArray &data) const
{
    if (m_sampleSize != 16 || data.size() < 2)
        return 0.0;
    qint32 peak = 0;
    const int samples = data.size() / 2;
    const uchar *bytes = reinterpret_cast<const uchar *>(data.constData());
    for (int i = 0; i < samples; ++i) {
        const qint16 sample = qFromLittleEndian<qint16>(bytes + i * 2);
        peak = qMax(peak, qAbs(static_cast<qint32>(sample)));
    }
    return qBound<qreal>(0.0, static_cast<qreal>(peak) / 32768.0, 1.0);
}

