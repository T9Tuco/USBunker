#pragma once

#include <QString>
#include <QList>

namespace bunker {

struct DriveInfo {
    QString name;
    QString path;
    QString device;
    qint64  totalBytes = 0;
    qint64  freeBytes  = 0;
    bool    hasVault   = false;
};

QList<DriveInfo> detectRemovableDrives();

}
