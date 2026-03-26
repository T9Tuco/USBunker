#include "usb.h"
#include "types.h"
#include <QStorageInfo>
#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace bunker {

namespace {

bool looksRemovable(const QStorageInfo& vol) {
#ifdef Q_OS_WIN
    std::wstring root = vol.rootPath().toStdWString();
    return GetDriveTypeW(root.c_str()) == DRIVE_REMOVABLE;
#else
    QString mount  = vol.rootPath();
    QString device = QString::fromUtf8(vol.device());

    if (mount.startsWith("/media/") || mount.startsWith("/run/media/"))
        return true;

    // typical usb block devices
    static const QStringList prefixes = {
        "/dev/sd", "/dev/disk/by-id/usb"
    };
    for (auto& p : prefixes)
        if (device.startsWith(p)) return true;

    return false;
#endif
}

} // anon

QList<DriveInfo> detectRemovableDrives() {
    QList<DriveInfo> out;

    for (const auto& vol : QStorageInfo::mountedVolumes()) {
        if (!vol.isValid() || !vol.isReady()) continue;
        if (!looksRemovable(vol)) continue;

        DriveInfo d;
        d.name  = vol.displayName();
        if (d.name.isEmpty()) d.name = vol.name();
        if (d.name.isEmpty()) d.name = "USB Drive";
        d.path       = vol.rootPath();
        d.device     = QString::fromUtf8(vol.device());
        d.totalBytes = vol.bytesTotal();
        d.freeBytes  = vol.bytesFree();
        d.hasVault   = QFile::exists(QDir(d.path).filePath(VAULT_FILE));

        out.append(d);
    }

    return out;
}

}
