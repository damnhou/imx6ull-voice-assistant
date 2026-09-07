#include "cloud/iflytekauth.h"
#include "cloud/iatresultparser.h"
#include "domain/commandparser.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QUrlQuery>

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    void createsSignedAuthenticationUrl();
    void mergesDynamicCorrectionResults();
    void parsesLedCommands_data();
    void parsesLedCommands();
};

void CoreTests::createsSignedAuthenticationUrl()
{
    const QDateTime time(QDate(2019, 7, 10), QTime(7, 35, 43), Qt::UTC);
    const QUrl url = IflytekAuth::signedUrl(
                QUrl(QStringLiteral("wss://iat-api.xfyun.cn/v2/iat")),
                QStringLiteral("test-key"), QStringLiteral("test-secret"), time);
    const QUrlQuery query(url);
    QCOMPARE(url.scheme(), QStringLiteral("wss"));
    QCOMPARE(url.path(), QStringLiteral("/v2/iat"));
    QCOMPARE(query.queryItemValue(QStringLiteral("date")),
             QStringLiteral("Wed, 10 Jul 2019 07:35:43 GMT"));
    QCOMPARE(query.queryItemValue(QStringLiteral("host")), QStringLiteral("iat-api.xfyun.cn"));
    const QByteArray authorization = QByteArray::fromBase64(
                query.queryItemValue(QStringLiteral("authorization")).toLatin1());
    QVERIFY(authorization.contains("api_key=\"test-key\""));
    QVERIFY(authorization.contains("algorithm=\"hmac-sha256\""));
    QVERIFY(authorization.contains("signature=\""));
}

static QByteArray response(int sn, const QString &text, const QString &pgs,
                           const QJsonArray &range, int status)
{
    QJsonObject result;
    result.insert(QStringLiteral("sn"), sn);
    result.insert(QStringLiteral("pgs"), pgs);
    if (!range.isEmpty())
        result.insert(QStringLiteral("rg"), range);
    result.insert(QStringLiteral("ws"), QJsonArray{
                      QJsonObject{{QStringLiteral("cw"), QJsonArray{
                          QJsonObject{{QStringLiteral("w"), text}}
                      }}}
                  });
    return QJsonDocument(QJsonObject{
        {QStringLiteral("code"), 0},
        {QStringLiteral("sid"), QStringLiteral("test-sid")},
        {QStringLiteral("data"), QJsonObject{
             {QStringLiteral("status"), status},
             {QStringLiteral("result"), result}
         }}
    }).toJson(QJsonDocument::Compact);
}

void CoreTests::mergesDynamicCorrectionResults()
{
    IatResultParser parser;
    QCOMPARE(parser.consume(response(0, QStringLiteral("打开"), QStringLiteral("apd"), {}, 1)).text,
             QStringLiteral("打开"));
    QCOMPARE(parser.consume(response(1, QStringLiteral("灯"), QStringLiteral("apd"), {}, 1)).text,
             QStringLiteral("打开灯"));
    const auto corrected = parser.consume(response(2, QStringLiteral("指示灯"), QStringLiteral("rpl"),
                                                    QJsonArray{1, 1}, 2));
    QVERIFY(corrected.ok);
    QVERIFY(corrected.finalFrame);
    QCOMPARE(corrected.text, QStringLiteral("打开指示灯"));
}

void CoreTests::parsesLedCommands_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("action");
    QTest::newRow("on") << QStringLiteral("请打开开发板指示灯")
                         << static_cast<int>(CommandParser::Action::LedOn);
    QTest::newRow("off") << QStringLiteral("关闭 LED")
                          << static_cast<int>(CommandParser::Action::LedOff);
    QTest::newRow("toggle") << QStringLiteral("切换一下板载灯")
                             << static_cast<int>(CommandParser::Action::LedToggle);
    QTest::newRow("unrelated") << QStringLiteral("今天天气怎么样")
                                << static_cast<int>(CommandParser::Action::None);
}

void CoreTests::parsesLedCommands()
{
    QFETCH(QString, text);
    QFETCH(int, action);
    CommandParser parser;
    QCOMPARE(static_cast<int>(parser.parse(text).action), action);
}

QTEST_APPLESS_MAIN(CoreTests)
#include "tst_core.moc"

