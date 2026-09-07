#include "cloud/iflytekauth.h"

#include <QCryptographicHash>
#include <QLocale>
#include <QMessageAuthenticationCode>
#include <QUrlQuery>

QString IflytekAuth::rfc1123Date(const QDateTime &utcDateTime)
{
    const QDateTime utc = utcDateTime.toUTC();
    return QLocale(QLocale::C).toString(utc, QStringLiteral("ddd, dd MMM yyyy HH:mm:ss 'GMT'"));
}

QUrl IflytekAuth::signedUrl(const QUrl &endpoint,
                            const QString &apiKey,
                            const QString &apiSecret,
                            const QDateTime &utcNow)
{
    QUrl result(endpoint);
    const QString host = endpoint.host();
    const QString path = endpoint.path().isEmpty() ? QStringLiteral("/") : endpoint.path();
    const QString date = rfc1123Date(utcNow);

    const QByteArray signatureOrigin = QStringLiteral("host: %1\ndate: %2\nGET %3 HTTP/1.1")
            .arg(host, date, path).toUtf8();
    const QByteArray signature = QMessageAuthenticationCode::hash(
                signatureOrigin, apiSecret.toUtf8(), QCryptographicHash::Sha256).toBase64();
    const QByteArray authorizationOrigin = QStringLiteral(
                "api_key=\"%1\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"%2\"")
            .arg(apiKey, QString::fromLatin1(signature)).toUtf8();

    QUrlQuery query(result);
    query.addQueryItem(QStringLiteral("authorization"),
                       QString::fromLatin1(authorizationOrigin.toBase64()));
    query.addQueryItem(QStringLiteral("date"), date);
    query.addQueryItem(QStringLiteral("host"), host);
    result.setQuery(query);
    return result;
}

