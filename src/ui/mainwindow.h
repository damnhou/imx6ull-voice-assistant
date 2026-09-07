#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void setRuntimeMode(bool mockMode, bool simulatedHardware);
    void setRecognitionState(const QString &state, bool recording);
    void setTranscript(const QString &text, bool finalText = false);
    void setAudioLevel(qreal level);
    void setLedState(bool on);
    void appendLog(const QString &message);

signals:
    void startRequested();
    void stopRequested();
    void ledToggleRequested();

private:
    QLabel *m_stateBadge = nullptr;
    QLabel *m_modeLabel = nullptr;
    QLabel *m_transcriptLabel = nullptr;
    QProgressBar *m_audioLevel = nullptr;
    QPushButton *m_recordButton = nullptr;
    QPushButton *m_ledButton = nullptr;
    QPlainTextEdit *m_log = nullptr;
};

#endif // MAINWINDOW_H

