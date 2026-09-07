#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include "cloud/iflytekiatclient.h"
#include "config/appconfig.h"

#include <QObject>
#include <QTimer>

class CommandParser;
class MainWindow;
class PcmAudioCapture;
class SysfsLedDevice;

class ApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationController(const AppConfig &config,
                                   MainWindow *window,
                                   QObject *parent = nullptr);
    ~ApplicationController() override;

private slots:
    void startRecognition();
    void stopRecognition();
    void handleFinalResult(const QString &text);
    void handleClientState(IflytekIatClient::State state);
    void handleError(const QString &message);
    void toggleLed();

private:
    QString resolveUserwordsPath() const;
    void executeCommand(const QString &text);

    AppConfig m_config;
    MainWindow *m_window = nullptr;
    PcmAudioCapture *m_audio = nullptr;
    IflytekIatClient *m_client = nullptr;
    CommandParser *m_commands = nullptr;
    SysfsLedDevice *m_led = nullptr;
    QTimer m_sessionTimer;
};

#endif // APPLICATIONCONTROLLER_H
