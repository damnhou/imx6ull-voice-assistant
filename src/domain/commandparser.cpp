#include "domain/commandparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

bool CommandParser::loadUserwords(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("无法读取词表 %1：%2").arg(path, file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QStringList words;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith(QLatin1Char('#')))
            words << line;
    }
    words.removeDuplicates();
    m_userwords = words;
    return true;
}

CommandParser::Result CommandParser::parse(const QString &recognizedText) const
{
    Result result;
    result.normalizedText = recognizedText.toLower();
    result.normalizedText.remove(QRegularExpression(QStringLiteral("[\\s，。！？,.!?]")));

    const bool mentionsLed = result.normalizedText.contains(QStringLiteral("灯"))
            || result.normalizedText.contains(QStringLiteral("led"));
    if (!mentionsLed) {
        result.explanation = QStringLiteral("未检测到 LED/指示灯目标词");
        return result;
    }

    if (result.normalizedText.contains(QRegularExpression(QStringLiteral("(关闭|关掉|熄灭)")))) {
        result.action = Action::LedOff;
        result.explanation = QStringLiteral("匹配 LED 关闭意图");
    } else if (result.normalizedText.contains(QRegularExpression(QStringLiteral("(打开|开启|点亮)")))) {
        result.action = Action::LedOn;
        result.explanation = QStringLiteral("匹配 LED 打开意图");
    } else if (result.normalizedText.contains(QRegularExpression(QStringLiteral("(切换|翻转|反转)")))) {
        result.action = Action::LedToggle;
        result.explanation = QStringLiteral("匹配 LED 切换意图");
    } else {
        result.explanation = QStringLiteral("检测到 LED 目标，但没有匹配动作词");
    }
    return result;
}

QStringList CommandParser::userwords() const
{
    return m_userwords;
}

