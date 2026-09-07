#ifndef SYSFSLEDDEVICE_H
#define SYSFSLEDDEVICE_H

#include "bsp/ileddevice.h"

#include <QObject>

class SysfsLedDevice : public QObject, public ILedDevice
{
    Q_OBJECT
public:
    explicit SysfsLedDevice(const QString &brightnessPath,
                            const QString &triggerPath,
                            bool activeHigh,
                            bool simulated,
                            QObject *parent = nullptr);

    bool isAvailable() const override;
    bool state() const override;
    bool setState(bool on, QString *error = nullptr) override;
    bool toggle(QString *error = nullptr) override;
    bool isSimulated() const;

signals:
    void stateChanged(bool on);
    void errorOccurred(const QString &message);

private:
    bool disableKernelTrigger(QString *error);
    bool writeTextFile(const QString &path, const QByteArray &value, QString *error);

    QString m_brightnessPath;
    QString m_triggerPath;
    bool m_activeHigh = true;
    bool m_simulated = true;
    bool m_state = false;
};

#endif // SYSFSLEDDEVICE_H

