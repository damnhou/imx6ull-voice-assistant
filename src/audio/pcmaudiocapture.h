#ifndef PCMAUDIOCAPTURE_H
#define PCMAUDIOCAPTURE_H

#include <QAudioFormat>
#include <QAudioInput>
#include <QObject>

class QIODevice;

class PcmAudioCapture : public QObject
{
    Q_OBJECT
public:
    explicit PcmAudioCapture(QObject *parent = nullptr);
    ~PcmAudioCapture() override;

    void configure(int sampleRate, int channels, int sampleSize,
                   const QString &deviceName = QString());
    bool isRunning() const;
    QString selectedDeviceName() const;

public slots:
    bool start();
    void stop();

signals:
    void pcmChunk(const QByteArray &data);
    void levelChanged(qreal level);
    void errorOccurred(const QString &message);
    void runningChanged(bool running);

private slots:
    void readAvailableAudio();
    void handleStateChanged(QAudio::State state);

private:
    QAudioFormat requestedFormat() const;
    qreal calculateLevel(const QByteArray &data) const;

    QAudioInput *m_audioInput = nullptr;
    QIODevice *m_stream = nullptr;
    QString m_requestedDevice;
    QString m_selectedDevice;
    int m_sampleRate = 16000;
    int m_channels = 1;
    int m_sampleSize = 16;
};

#endif // PCMAUDIOCAPTURE_H

