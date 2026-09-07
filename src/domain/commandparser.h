#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <QString>
#include <QStringList>

class CommandParser
{
public:
    enum class Action { None, LedOn, LedOff, LedToggle };
    struct Result {
        Action action = Action::None;
        QString normalizedText;
        QString explanation;
    };

    bool loadUserwords(const QString &path, QString *error = nullptr);
    Result parse(const QString &recognizedText) const;
    QStringList userwords() const;

private:
    QStringList m_userwords;
};

#endif // COMMANDPARSER_H

