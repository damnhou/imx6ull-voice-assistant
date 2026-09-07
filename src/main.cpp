#include "app/applicationcontroller.h"
#include "config/appconfig.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("VoiceEdge"));
    application.setApplicationVersion(QStringLiteral("0.1.0"));
    application.setOrganizationName(QStringLiteral("Portfolio"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("i.MX6ULL 流式语音识别与外设控制终端"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configOption(QStringList{QStringLiteral("c"), QStringLiteral("config")},
                                    QStringLiteral("加载指定 INI 配置文件"),
                                    QStringLiteral("file"));
    parser.addOption(configOption);
    parser.process(application);

    QFile style(QStringLiteral(":/defaults/style.qss"));
    if (style.open(QIODevice::ReadOnly))
        application.setStyleSheet(QString::fromUtf8(style.readAll()));

    const AppConfig config = AppConfig::load(parser.value(configOption));
    MainWindow window;
    ApplicationController controller(config, &window);
    Q_UNUSED(controller)
    window.show();
    return application.exec();
}

