#include "app/applicationcontroller.h"

#include "audio/pcmaudiocapture.h"
#include "bsp/sysfsleddevice.h"
#include "domain/commandparser.h"
#include "ui/mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

ApplicationController::ApplicationController(const AppConfig &config,
                                             MainWindow *window,
                                             QObject *parent)
    : QObject(parent), m_config(config), m_window(window)
{
    m_audio = new PcmAudioCapture(this);
    m_client = new IflytekIatClient(this);
    m_commands = new CommandParser;
    m_led = new SysfsLedDevice(config.ledBrightnessPath, config.ledTriggerPath,
                               config.ledActiveHigh, config.simulateHardware, this);

    m_audio->configure(config.sampleRate, config.channels, config.sampleSize, config.audioDevice);
    IatOptions options;
    options.endpoint = QUrl(config.endpoint);
    options.appId = config.appId;
    options.apiKey = config.apiKey;
    options.apiSecret = config.apiSecret;
    options.language = config.language;
    options.accent = config.accent;
    options.domain = config.domain;
    options.dynamicCorrection = config.dynamicCorrection;
    options.vadEosMs = config.vadEosMs;
    options.sampleRate = config.sampleRate;
    options.frameIntervalMs = config.frameIntervalMs;
    options.mockMode = config.mockMode;
    m_client->configure(options);

    QString wordError;
    const QString wordPath = resolveUserwordsPath();
    if (m_commands->loadUserwords(wordPath, &wordError))
        m_window->appendLog(QStringLiteral("已加载 %1 个场景词：%2")
                            .arg(m_commands->userwords().size()).arg(wordPath));
    else
        m_window->appendLog(wordError);

    connect(m_window, &MainWindow::startRequested, this, &ApplicationController::startRecognition);
    connect(m_window, &MainWindow::stopRequested, this, &ApplicationController::stopRecognition);
    connect(m_window, &MainWindow::ledToggleRequested, this, &ApplicationController::toggleLed);
    connect(m_audio, &PcmAudioCapture::pcmChunk, m_client, &IflytekIatClient::appendAudio);
    connect(m_audio, &PcmAudioCapture::levelChanged, m_window, &MainWindow::setAudioLevel);
    connect(m_audio, &PcmAudioCapture::errorOccurred, this, &ApplicationController::handleError);
    connect(m_client, &IflytekIatClient::partialResult, this, [this](const QString &text) {
        m_window->setTranscript(text, false);
    });
    connect(m_client, &IflytekIatClient::finalResult, this, &ApplicationController::handleFinalResult);
    connect(m_client, &IflytekIatClient::stateChanged, this, &ApplicationController::handleClientState);
    connect(m_client, &IflytekIatClient::errorOccurred, this, &ApplicationController::handleError);
    connect(m_client, &IflytekIatClient::diagnostic, m_window, &MainWindow::appendLog);
    connect(m_led, &SysfsLedDevice::stateChanged, m_window, &MainWindow::setLedState);
    connect(m_led, &SysfsLedDevice::errorOccurred, this, &ApplicationController::handleError);

    m_sessionTimer.setSingleShot(true);
    m_sessionTimer.setInterval(config.maxRecordSeconds * 1000);
    connect(&m_sessionTimer, &QTimer::timeout, this, &ApplicationController::stopRecognition);

    m_window->setRuntimeMode(config.mockMode, config.simulateHardware);
    m_window->setLedState(m_led->state());
    m_window->appendLog(QStringLiteral("配置来源：%1").arg(config.loadedFrom));
    if (!config.mockMode && !config.isCloudConfigured())
        m_window->appendLog(QStringLiteral("云端配置错误：%1").arg(config.cloudValidationErrors().join(QStringLiteral("；"))));
    if (!m_led->isAvailable())
        m_window->appendLog(QStringLiteral("未找到 LED sysfs 节点：%1").arg(config.ledBrightnessPath));
}

ApplicationController::~ApplicationController()
{
    delete m_commands;
}

void ApplicationController::startRecognition()
{
    if (!m_config.mockMode && !m_config.isCloudConfigured()) {
        handleError(m_config.cloudValidationErrors().join(QStringLiteral("；")));
        return;
    }

    m_window->setTranscript(QString(), false);
    m_client->startRecognition();

    if (m_config.mockMode) {
        m_window->setRecognitionState(QStringLiteral("演示识别中"), true);
        m_sessionTimer.start();
        return;
    }

    if (!m_audio->start()) {
        m_client->abortRecognition();
        m_window->setRecognitionState(QStringLiteral("录音失败"), false);
        return;
    }
    m_window->appendLog(QStringLiteral("录音设备：%1").arg(m_audio->selectedDeviceName()));
    m_sessionTimer.start();
}

void ApplicationController::stopRecognition()
{
    m_sessionTimer.stop();
    m_audio->stop();
    m_client->finishRecognition();
    m_window->setRecognitionState(QStringLiteral("等待最终结果"), false);
}

void ApplicationController::handleFinalResult(const QString &text)
{
    m_sessionTimer.stop();
    m_audio->stop();
    m_window->setTranscript(text, true);
    m_window->setRecognitionState(QStringLiteral("识别完成"), false);
    executeCommand(text);
}

void ApplicationController::handleClientState(IflytekIatClient::State state)
{
    switch (state) {
    case IflytekIatClient::State::Connecting:
        m_window->setRecognitionState(QStringLiteral("连接云端"), true);
        break;
    case IflytekIatClient::State::Streaming:
        m_window->setRecognitionState(QStringLiteral("正在聆听"), true);
        break;
    case IflytekIatClient::State::Finishing:
        m_window->setRecognitionState(QStringLiteral("正在收尾"), false);
        break;
    case IflytekIatClient::State::Error:
        m_window->setRecognitionState(QStringLiteral("发生错误"), false);
        break;
    default:
        break;
    }
}

void ApplicationController::handleError(const QString &message)
{
    m_sessionTimer.stop();
    m_audio->stop();
    m_window->setRecognitionState(QStringLiteral("发生错误"), false);
    m_window->appendLog(message);
}

void ApplicationController::toggleLed()
{
    QString error;
    if (m_led->toggle(&error))
        m_window->appendLog(QStringLiteral("手动切换 LED：%1").arg(m_led->state() ? QStringLiteral("开") : QStringLiteral("关")));
}

QString ApplicationController::resolveUserwordsPath() const
{
    if (m_config.userwordsPath.startsWith(QStringLiteral(":")))
        return m_config.userwordsPath;
    const QFileInfo configured(m_config.userwordsPath);
    if (configured.isAbsolute() && configured.exists())
        return configured.absoluteFilePath();

    const QString besideBinary = QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(m_config.userwordsPath);
    if (QFileInfo::exists(besideBinary))
        return besideBinary;
    const QString fromWorkingDirectory = QDir::current().absoluteFilePath(m_config.userwordsPath);
    if (QFileInfo::exists(fromWorkingDirectory))
        return fromWorkingDirectory;
    return QStringLiteral(":/defaults/userwords.txt");
}

void ApplicationController::executeCommand(const QString &text)
{
    const CommandParser::Result command = m_commands->parse(text);
    QString error;
    bool executed = false;
    switch (command.action) {
    case CommandParser::Action::LedOn:
        executed = m_led->setState(true, &error);
        break;
    case CommandParser::Action::LedOff:
        executed = m_led->setState(false, &error);
        break;
    case CommandParser::Action::LedToggle:
        executed = m_led->toggle(&error);
        break;
    case CommandParser::Action::None:
        break;
    }

    if (command.action == CommandParser::Action::None)
        m_window->appendLog(QStringLiteral("未执行外设动作：%1").arg(command.explanation));
    else if (executed)
        m_window->appendLog(QStringLiteral("已执行语音指令：%1").arg(command.explanation));
    else
        m_window->appendLog(QStringLiteral("指令执行失败：%1").arg(error));
}
