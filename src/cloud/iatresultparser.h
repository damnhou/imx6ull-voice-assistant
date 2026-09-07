#ifndef IATRESULTPARSER_H
#define IATRESULTPARSER_H

#include <QMap>
#include <QByteArray>
#include <QString>

class IatResultParser
{
public:
    struct ParseResult {
        bool ok = false;
        bool finalFrame = false;
        QString text;
        QString sid;
        QString error;
    };

    ParseResult consume(const QByteArray &message);
    void reset();
    QString text() const;

private:
    QMap<int, QString> m_segments;
};

#endif // IATRESULTPARSER_H

