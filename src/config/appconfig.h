#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QStringList>

class AppConfig
{
public:
    static AppConfig load(const QString &explicitPath = QString());

    bool isCloudConfigured() const;
    QStringList cloudValidationErrors() const;

    bool mockMode = true;
    int maxRecordSeconds = 60;

    QString endpoint = QStringLiteral("wss://iat-api.xfyun.cn/v2/iat");
    QString appId;
    QString apiKey;
    QString apiSecret;
    QString language = QStringLiteral("zh_cn");
    QString accent = QStringLiteral("mandarin");
    QString domain = QStringLiteral("iat");
    bool dynamicCorrection = true;
    int vadEosMs = 3000;
    int frameIntervalMs = 40;

    QString audioDevice;
    int sampleRate = 16000;
    int channels = 1;
    int sampleSize = 16;

    bool simulateHardware = true;
    QString ledName = QStringLiteral("sys-led");
    QString ledBrightnessPath = QStringLiteral("/sys/devices/platform/leds/leds/sys-led/brightness");
    QString ledTriggerPath = QStringLiteral("/sys/class/leds/sys-led/trigger");
    bool ledActiveHigh = true;

    QString userwordsPath = QStringLiteral("config/userwords.txt");
    QString loadedFrom;
};

#endif // APPCONFIG_H

