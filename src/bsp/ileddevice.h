#ifndef ILEDDEVICE_H
#define ILEDDEVICE_H

#include <QString>

class ILedDevice
{
public:
    virtual ~ILedDevice() = default;
    virtual bool isAvailable() const = 0;
    virtual bool state() const = 0;
    virtual bool setState(bool on, QString *error = nullptr) = 0;
    virtual bool toggle(QString *error = nullptr) = 0;
};

#endif // ILEDDEVICE_H

