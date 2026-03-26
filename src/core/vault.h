#pragma once

#include "types.h"
#include "crypto.h"
#include <QObject>
#include <QString>
#include <vector>

namespace bunker {

constexpr char VAULT_MAGIC[8] = {'U','S','B','U','N','K','R','\0'};
constexpr int  HEADER_SIZE    = 256;
constexpr int  CHUNK_SIZE     = 4 * 1024 * 1024;

struct FileEntry {
    std::string path;
    uint64_t    size        = 0;
    bool        isDir       = false;
    Bytes       iv;
    Bytes       tag;
    uint64_t    dataOffset  = 0;
};

struct VaultHeader {
    uint32_t version    = FORMAT_VERSION;
    uint32_t flags      = 0;
    Bytes    salt;
    Bytes    keyCheck;
    uint64_t fileCount  = 0;
    uint64_t tableOff   = 0;
    uint64_t tableSize  = 0;
    Bytes    tableIV;
    Bytes    tableTag;
};

class VaultWorker : public QObject {
    Q_OBJECT
public:
    explicit VaultWorker(QObject* parent = nullptr);

public slots:
    void encrypt(const QString& drivePath, const QString& password);
    void decrypt(const QString& drivePath, const QString& password);

signals:
    void progress(int percent, const QString& status);
    void finished(bool ok, const QString& message);
};

}
