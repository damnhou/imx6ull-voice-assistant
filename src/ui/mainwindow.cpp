#include "ui/mainwindow.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("i.MX6ULL 语音控制终端"));
    setMinimumSize(800, 480);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 15, 18, 15);
    root->setSpacing(12);

    auto *headerCard = new QFrame(central);
    headerCard->setObjectName(QStringLiteral("headerCard"));
    auto *header = new QHBoxLayout(headerCard);
    header->setContentsMargins(18, 11, 18, 11);
    auto *title = new QLabel(QStringLiteral("VoiceEdge · i.MX6ULL"), headerCard);
    title->setObjectName(QStringLiteral("titleLabel"));
    auto *subtitle = new QLabel(QStringLiteral("嵌入式流式语音识别与外设控制"), headerCard);
    subtitle->setObjectName(QStringLiteral("hintLabel"));
    m_stateBadge = new QLabel(QStringLiteral("就绪"), headerCard);
    m_stateBadge->setObjectName(QStringLiteral("stateBadge"));
    header->addWidget(title);
    header->addWidget(subtitle);
    header->addStretch();
    header->addWidget(m_stateBadge);
    root->addWidget(headerCard);

    auto *contentCard = new QFrame(central);
    contentCard->setObjectName(QStringLiteral("contentCard"));
    auto *content = new QVBoxLayout(contentCard);
    content->setContentsMargins(18, 12, 18, 12);
    auto *caption = new QLabel(QStringLiteral("识别文本"), contentCard);
    caption->setObjectName(QStringLiteral("hintLabel"));
    m_transcriptLabel = new QLabel(QStringLiteral("点击开始，说出“打开指示灯”"), contentCard);
    m_transcriptLabel->setObjectName(QStringLiteral("transcriptLabel"));
    m_transcriptLabel->setWordWrap(true);
    m_transcriptLabel->setAlignment(Qt::AlignCenter);
    m_transcriptLabel->setMinimumHeight(85);
    m_audioLevel = new QProgressBar(contentCard);
    m_audioLevel->setRange(0, 100);
    m_audioLevel->setValue(0);
    m_audioLevel->setTextVisible(false);
    content->addWidget(caption);
    content->addWidget(m_transcriptLabel, 1);
    content->addWidget(m_audioLevel);
    root->addWidget(contentCard, 1);

    auto *controlCard = new QFrame(central);
    controlCard->setObjectName(QStringLiteral("controlCard"));
    auto *controls = new QHBoxLayout(controlCard);
    controls->setContentsMargins(18, 12, 18, 12);
    m_recordButton = new QPushButton(QStringLiteral("开始识别"), controlCard);
    m_recordButton->setObjectName(QStringLiteral("recordButton"));
    m_recordButton->setCheckable(true);
    m_ledButton = new QPushButton(QStringLiteral("LED：关闭"), controlCard);
    m_ledButton->setObjectName(QStringLiteral("ledButton"));
    m_ledButton->setCheckable(true);
    m_modeLabel = new QLabel(controlCard);
    m_modeLabel->setObjectName(QStringLiteral("hintLabel"));
    controls->addWidget(m_recordButton, 2);
    controls->addWidget(m_ledButton, 1);
    controls->addWidget(m_modeLabel, 2);
    root->addWidget(controlCard);

    m_log = new QPlainTextEdit(central);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(100);
    m_log->setFixedHeight(72);
    root->addWidget(m_log);

    setCentralWidget(central);

    connect(m_recordButton, &QPushButton::clicked, this, [this](bool checked) {
        if (checked)
            emit startRequested();
        else
            emit stopRequested();
    });
    connect(m_ledButton, &QPushButton::clicked, this, [this](bool) {
        emit ledToggleRequested();
    });
}

void MainWindow::setRuntimeMode(bool mockMode, bool simulatedHardware)
{
    m_modeLabel->setText(QStringLiteral("识别：%1  ·  LED：%2")
                         .arg(mockMode ? QStringLiteral("演示") : QStringLiteral("讯飞云端"),
                              simulatedHardware ? QStringLiteral("模拟") : QStringLiteral("板载")));
}

void MainWindow::setRecognitionState(const QString &state, bool recording)
{
    m_stateBadge->setText(state);
    m_recordButton->blockSignals(true);
    m_recordButton->setChecked(recording);
    m_recordButton->setText(recording ? QStringLiteral("停止识别") : QStringLiteral("开始识别"));
    m_recordButton->blockSignals(false);
}

void MainWindow::setTranscript(const QString &text, bool finalText)
{
    m_transcriptLabel->setText(text.isEmpty() ? QStringLiteral("正在聆听…") : text);
    m_transcriptLabel->setStyleSheet(finalText
            ? QStringLiteral("color: #77e1b9;") : QString());
}

void MainWindow::setAudioLevel(qreal level)
{
    m_audioLevel->setValue(qBound(0, qRound(level * 100.0), 100));
}

void MainWindow::setLedState(bool on)
{
    m_ledButton->blockSignals(true);
    m_ledButton->setChecked(on);
    m_ledButton->setText(on ? QStringLiteral("LED：点亮") : QStringLiteral("LED：关闭"));
    m_ledButton->blockSignals(false);
}

void MainWindow::appendLog(const QString &message)
{
    m_log->appendPlainText(QStringLiteral("[%1] %2")
                           .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), message));
}

