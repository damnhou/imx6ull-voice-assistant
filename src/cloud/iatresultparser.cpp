#include "cloud/iatresultparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

IatResultParser::ParseResult IatResultParser::consume(const QByteArray &message)
{
    ParseResult output;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        output.error = QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString());
        return output;
    }

    const QJsonObject root = document.object();
    output.sid = root.value(QStringLiteral("sid")).toString();
    const int code = root.value(QStringLiteral("code")).toInt(-1);
    if (code != 0) {
        output.error = QStringLiteral("讯飞服务错误 %1：%2")
                .arg(code)
                .arg(root.value(QStringLiteral("message")).toString());
        return output;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QJsonObject result = data.value(QStringLiteral("result")).toObject();
    const int sequence = result.value(QStringLiteral("sn")).toInt(m_segments.size());
    QString segmentText;

    const QJsonArray wordSegments = result.value(QStringLiteral("ws")).toArray();
    for (const QJsonValue &wordSegmentValue : wordSegments) {
        const QJsonArray candidates = wordSegmentValue.toObject()
                .value(QStringLiteral("cw")).toArray();
        if (!candidates.isEmpty())
            segmentText += candidates.first().toObject().value(QStringLiteral("w")).toString();
    }

    if (result.value(QStringLiteral("pgs")).toString() == QStringLiteral("rpl")) {
        const QJsonArray replaceRange = result.value(QStringLiteral("rg")).toArray();
        if (replaceRange.size() == 2) {
            const int first = replaceRange.at(0).toInt();
            const int last = replaceRange.at(1).toInt();
            for (int key = first; key <= last; ++key)
                m_segments.remove(key);
        }
    }

    m_segments.insert(sequence, segmentText);
    output.ok = true;
    output.finalFrame = data.value(QStringLiteral("status")).toInt() == 2;
    output.text = text();
    return output;
}

void IatResultParser::reset()
{
    m_segments.clear();
}

QString IatResultParser::text() const
{
    QString combined;
    for (auto it = m_segments.constBegin(); it != m_segments.constEnd(); ++it)
        combined += it.value();
    return combined;
}

