#include "bsp/sysfsleddevice.h"

#include <QFile>
#include <QFileInfo>

SysfsLedDevice::SysfsLedDevice(const QString &brightnessPath,
                               const QString &triggerPath,
                               bool activeHigh,
                               bool simulated,
                               QObject *parent)
    : QObject(parent),
      m_brightnessPath(brightnessPath),
      m_triggerPath(triggerPath),
      m_activeHigh(activeHigh),
      m_simulated(simulated)
{
    if (!m_simulated && !QFileInfo::exists(m_brightnessPath)) {
        const QString standardPath = QStringLiteral("/sys/class/leds/sys-led/brightness");
        if (QFileInfo::exists(standardPath))
            m_brightnessPath = standardPath;
    }
}

bool SysfsLedDevice::isAvailable() const
{
    return m_simulated || QFileInfo::exists(m_brightnessPath);
}

bool SysfsLedDevice::state() const
{
    return m_state;
}

bool SysfsLedDevice::setState(bool on, QString *error)
{
    if (m_simulated) {
        m_state = on;
        emit stateChanged(on);
        return true;
    }

    QString localError;
    if (!disableKernelTrigger(&localError)
            || !writeTextFile(m_brightnessPath,
                              (on == m_activeHigh) ? QByteArrayLiteral("1") : QByteArrayLiteral("0"),
                              &localError)) {
        if (error)
            *error = localError;
        emit errorOccurred(localError);
        return false;
    }

    m_state = on;
    emit stateChanged(on);
    return true;
}

bool SysfsLedDevice::toggle(QString *error)
{
    return setState(!m_state, error);
}

bool SysfsLedDevice::isSimulated() const
{
    return m_simulated;
}

bool SysfsLedDevice::disableKernelTrigger(QString *error)
{
    if (m_triggerPath.isEmpty() || !QFileInfo::exists(m_triggerPath))
        return true;
    return writeTextFile(m_triggerPath, QByteArrayLiteral("none"), error);
}

bool SysfsLedDevice::writeTextFile(const QString &path, const QByteArray &value, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("无法写入 %1：%2").arg(path, file.errorString());
        return false;
    }
    if (file.write(value) != value.size()) {
        if (error)
            *error = QStringLiteral("写入 %1 失败：%2").arg(path, file.errorString());
        return false;
    }
    return true;
}

