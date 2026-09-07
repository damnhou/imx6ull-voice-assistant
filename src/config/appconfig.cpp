#include "config/appconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSettings>
#include <QUrl>

namespace {
QString resolvedConfigPath(const QString &explicitPath)
{
    if (!explicitPath.isEmpty())
        return QFileInfo(explicitPath).absoluteFilePath();

    const QString besideBinary = QCoreApplication::applicationDirPath()
            + QStringLiteral("/config/voice_assistant.ini");
    if (QFileInfo::exists(besideBinary))
        return besideBinary;

    const QString workingDirectory = QDir::current().absoluteFilePath(
                QStringLiteral("config/voice_assistant.ini"));
    if (QFileInfo::exists(workingDirectory))
        return workingDirectory;

    return QStringLiteral(":/defaults/voice_assistant.ini");
}

QString envOrValue(const char *name, const QVariant &value)
{
    const QString fromEnvironment = QProcessEnvironment::systemEnvironment()
            .value(QString::fromLatin1(name));
    return fromEnvironment.isEmpty() ? value.toString().trimmed() : fromEnvironment.trimmed();
}
}

AppConfig AppConfig::load(const QString &explicitPath)
{
    AppConfig config;
    const QString path = resolvedConfigPath(explicitPath);
    QSettings settings(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    settings.setIniCodec("UTF-8");
#endif

    config.loadedFrom = path;
    config.mockMode = settings.value(QStringLiteral("runtime/mock_mode"), config.mockMode).toBool();
    config.maxRecordSeconds = settings.value(QStringLiteral("runtime/max_record_seconds"), config.maxRecordSeconds).toInt();

    config.endpoint = settings.value(QStringLiteral("iflytek/endpoint"), config.endpoint).toString().trimmed();
    config.appId = envOrValue("XFYUN_APP_ID", settings.value(QStringLiteral("iflytek/app_id")));
    config.apiKey = envOrValue("XFYUN_API_KEY", settings.value(QStringLiteral("iflytek/api_key")));
    config.apiSecret = envOrValue("XFYUN_API_SECRET", settings.value(QStringLiteral("iflytek/api_secret")));
    config.language = settings.value(QStringLiteral("iflytek/language"), config.language).toString().trimmed();
    config.accent = settings.value(QStringLiteral("iflytek/accent"), config.accent).toString().trimmed();
    config.domain = settings.value(QStringLiteral("iflytek/domain"), config.domain).toString().trimmed();
    config.dynamicCorrection = settings.value(QStringLiteral("iflytek/dynamic_correction"), config.dynamicCorrection).toBool();
    config.vadEosMs = settings.value(QStringLiteral("iflytek/vad_eos_ms"), config.vadEosMs).toInt();
    config.frameIntervalMs = settings.value(QStringLiteral("iflytek/frame_interval_ms"), config.frameIntervalMs).toInt();

    config.audioDevice = settings.value(QStringLiteral("audio/device"), config.audioDevice).toString();
    config.sampleRate = settings.value(QStringLiteral("audio/sample_rate"), config.sampleRate).toInt();
    config.channels = settings.value(QStringLiteral("audio/channels"), config.channels).toInt();
    config.sampleSize = settings.value(QStringLiteral("audio/sample_size"), config.sampleSize).toInt();

    config.simulateHardware = settings.value(QStringLiteral("hardware/simulate"), config.simulateHardware).toBool();
    config.ledName = settings.value(QStringLiteral("hardware/led_name"), config.ledName).toString().trimmed();
    config.ledBrightnessPath = settings.value(QStringLiteral("hardware/led_brightness"), config.ledBrightnessPath).toString().trimmed();
    config.ledTriggerPath = settings.value(QStringLiteral("hardware/led_trigger"), config.ledTriggerPath).toString().trimmed();
    config.ledActiveHigh = settings.value(QStringLiteral("hardware/active_high"), config.ledActiveHigh).toBool();
    config.userwordsPath = settings.value(QStringLiteral("commands/userwords"), config.userwordsPath).toString().trimmed();

    if (config.maxRecordSeconds < 1 || config.maxRecordSeconds > 60)
        config.maxRecordSeconds = 60;
    if (config.frameIntervalMs < 20 || config.frameIntervalMs > 100)
        config.frameIntervalMs = 40;
    if (config.vadEosMs < 1000 || config.vadEosMs > 10000)
        config.vadEosMs = 3000;

    return config;
}

bool AppConfig::isCloudConfigured() const
{
    return cloudValidationErrors().isEmpty();
}

QStringList AppConfig::cloudValidationErrors() const
{
    QStringList errors;
    const QUrl url(endpoint);
    if (!url.isValid() || url.scheme() != QStringLiteral("wss"))
        errors << QStringLiteral("iflytek/endpoint 必须是有效的 wss URL");
    if (appId.isEmpty())
        errors << QStringLiteral("缺少 AppID");
    if (apiKey.isEmpty())
        errors << QStringLiteral("缺少 APIKey");
    if (apiSecret.isEmpty())
        errors << QStringLiteral("缺少 APISecret");
    if (sampleRate != 16000 || channels != 1 || sampleSize != 16)
        errors << QStringLiteral("讯飞流式听写需要 16 kHz / 16 bit / 单声道 PCM");
    return errors;
}

