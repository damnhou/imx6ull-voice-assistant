#ifndef IFLYTEKAUTH_H
#define IFLYTEKAUTH_H

#include <QDateTime>
#include <QString>
#include <QUrl>

class IflytekAuth
{
public:
    static QUrl signedUrl(const QUrl &endpoint,
                          const QString &apiKey,
                          const QString &apiSecret,
                          const QDateTime &utcNow = QDateTime::currentDateTimeUtc());

    static QString rfc1123Date(const QDateTime &utcDateTime);
};

#endif // IFLYTEKAUTH_H

