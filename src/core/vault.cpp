#include "vault.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <cstring>

namespace bunker {

namespace {

void put32(uint8_t* dst, uint32_t v) {
    for (int i = 0; i < 4; i++) dst[i] = (v >> (i * 8)) & 0xFF;
}
void put64(uint8_t* dst, uint64_t v) {
    for (int i = 0; i < 8; i++) dst[i] = (v >> (i * 8)) & 0xFF;
}
uint32_t get32(const uint8_t* s) {
    return s[0] | (uint32_t(s[1]) << 8) | (uint32_t(s[2]) << 16) | (uint32_t(s[3]) << 24);
}
uint64_t get64(const uint8_t* s) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= uint64_t(s[i]) << (i * 8);
    return v;
}

Bytes packHeader(const VaultHeader& h) {
    Bytes buf(HEADER_SIZE, 0);
    std::memcpy(buf.data(), VAULT_MAGIC, 8);
    put32(buf.data() + 8,  h.version);
    put32(buf.data() + 12, h.flags);
    std::memcpy(buf.data() + 16, h.salt.data(), 32);
    std::memcpy(buf.data() + 48, h.keyCheck.data(), 32);
    put64(buf.data() + 80, h.fileCount);
    put64(buf.data() + 88, h.tableOff);
    put64(buf.data() + 96, h.tableSize);
    if (!h.tableIV.empty())
        std::memcpy(buf.data() + 104, h.tableIV.data(), crypto::IV_LEN);
    if (!h.tableTag.empty())
        std::memcpy(buf.data() + 116, h.tableTag.data(), crypto::TAG_LEN);
    return buf;
}

VaultHeader unpackHeader(const Bytes& buf) {
    if (std::memcmp(buf.data(), VAULT_MAGIC, 8) != 0)
        throw std::runtime_error("Not a USBunker vault.");

    VaultHeader h;
    h.version   = get32(buf.data() + 8);
    h.flags     = get32(buf.data() + 12);
    h.salt.assign(buf.begin() + 16, buf.begin() + 48);
    h.keyCheck.assign(buf.begin() + 48, buf.begin() + 80);
    h.fileCount = get64(buf.data() + 80);
    h.tableOff  = get64(buf.data() + 88);
    h.tableSize = get64(buf.data() + 96);
    h.tableIV.assign(buf.begin() + 104, buf.begin() + 116);
    h.tableTag.assign(buf.begin() + 116, buf.begin() + 132);
    return h;
}

Bytes packTable(const std::vector<FileEntry>& entries) {
    Bytes buf;
    for (auto& e : entries) {
        size_t off = buf.size();
        buf.resize(off + 4);
        put32(buf.data() + off, static_cast<uint32_t>(e.path.size()));
        buf.insert(buf.end(), e.path.begin(), e.path.end());

        off = buf.size();
        buf.resize(off + 8);
        put64(buf.data() + off, e.size);

        buf.push_back(e.isDir ? 1 : 0);
        buf.insert(buf.end(), e.iv.begin(), e.iv.end());
        buf.insert(buf.end(), e.tag.begin(), e.tag.end());

        off = buf.size();
        buf.resize(off + 8);
        put64(buf.data() + off, e.dataOffset);
    }
    return buf;
}

std::vector<FileEntry> unpackTable(const Bytes& buf, uint64_t count) {
    std::vector<FileEntry> entries;
    size_t pos = 0;
    for (uint64_t i = 0; i < count; i++) {
        FileEntry e;
        uint32_t plen = get32(buf.data() + pos); pos += 4;
        e.path.assign(buf.begin() + pos, buf.begin() + pos + plen); pos += plen;
        e.size  = get64(buf.data() + pos); pos += 8;
        e.isDir = buf[pos] != 0; pos += 1;
        e.iv.assign(buf.begin() + pos, buf.begin() + pos + crypto::IV_LEN);
        pos += crypto::IV_LEN;
        e.tag.assign(buf.begin() + pos, buf.begin() + pos + crypto::TAG_LEN);
        pos += crypto::TAG_LEN;
        e.dataOffset = get64(buf.data() + pos); pos += 8;
        entries.push_back(std::move(e));
    }
    return entries;
}

struct ScanResult {
    std::vector<std::string> dirs;
    std::vector<std::pair<std::string, uint64_t>> files;
    uint64_t totalBytes = 0;
};

ScanResult scanDrive(const QString& root) {
    ScanResult out;
    QDir base(root);

    QDirIterator it(root,
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString rel = base.relativeFilePath(it.filePath());

        if (rel == VAULT_FILE) continue;
        if (rel.startsWith("System Volume Information")) continue;
        if (rel.startsWith("$RECYCLE.BIN")) continue;
        if (rel.startsWith(".Trash")) continue;

        QFileInfo fi = it.fileInfo();
        if (fi.isDir()) {
            out.dirs.push_back(rel.toStdString());
        } else {
            auto sz = static_cast<uint64_t>(fi.size());
            out.files.push_back({rel.toStdString(), sz});
            out.totalBytes += sz;
        }
    }
    return out;
}

} // anon

VaultWorker::VaultWorker(QObject* parent) : QObject(parent) {}

void VaultWorker::encrypt(const QString& drivePath, const QString& password) {
    try {
        emit progress(0, "Scanning files...");
        auto scan = scanDrive(drivePath);

        if (scan.files.empty() && scan.dirs.empty()) {
            emit finished(false, "No files found on the drive.");
            return;
        }

        emit progress(5, "Deriving encryption key...");
        Bytes salt = crypto::randomBytes(crypto::SALT_LEN);
        Bytes key  = crypto::deriveKey(password.toStdString(), salt);
        Bytes kfp  = crypto::keyFingerprint(key);

        // build entry list (dirs first, then files)
        std::vector<FileEntry> entries;
        uint64_t offset = HEADER_SIZE;

        for (auto& d : scan.dirs) {
            FileEntry e;
            e.path  = d;
            e.isDir = true;
            e.iv.resize(crypto::IV_LEN, 0);
            e.tag.resize(crypto::TAG_LEN, 0);
            entries.push_back(std::move(e));
        }

        for (auto& [path, sz] : scan.files) {
            FileEntry e;
            e.path       = path;
            e.size       = sz;
            e.iv         = crypto::randomBytes(crypto::IV_LEN);
            e.tag.resize(crypto::TAG_LEN, 0);
            e.dataOffset = offset;
            offset += sz; // GCM ciphertext is same length as plaintext
            entries.push_back(std::move(e));
        }

        QString vaultPath = QDir(drivePath).filePath(VAULT_FILE);
        QFile vault(vaultPath);
        if (!vault.open(QIODevice::WriteOnly)) {
            emit finished(false, "Cannot create vault file: " + vault.errorString());
            return;
        }

        // placeholder header, we come back to fill it in
        Bytes hdrBuf(HEADER_SIZE, 0);
        vault.write(reinterpret_cast<const char*>(hdrBuf.data()), HEADER_SIZE);

        // encrypt each file and stream to vault
        uint64_t done = 0;
        for (auto& entry : entries) {
            if (entry.isDir) continue;

            QString srcPath = QDir(drivePath).filePath(
                QString::fromStdString(entry.path));
            QFile src(srcPath);
            if (!src.open(QIODevice::ReadOnly)) {
                vault.close();
                QFile::remove(vaultPath);
                emit finished(false,
                    "Cannot read: " + QString::fromStdString(entry.path));
                return;
            }

            int pct = 10 + static_cast<int>(
                78.0 * done / std::max(scan.totalBytes, uint64_t(1)));
            emit progress(pct,
                QString("Encrypting %1").arg(
                    QString::fromStdString(entry.path)));

            crypto::StreamEncryptor enc(key, entry.iv);

            while (!src.atEnd()) {
                QByteArray chunk = src.read(CHUNK_SIZE);
                Bytes ct = enc.update(
                    reinterpret_cast<const uint8_t*>(chunk.constData()),
                    static_cast<int>(chunk.size()));
                vault.write(reinterpret_cast<const char*>(ct.data()),
                            ct.size());
                done += chunk.size();

                pct = 10 + static_cast<int>(
                    78.0 * done / std::max(scan.totalBytes, uint64_t(1)));
                emit progress(pct,
                    QString("Encrypting %1").arg(
                        QString::fromStdString(entry.path)));
            }

            auto tail = enc.finish();
            if (!tail.ciphertext.empty())
                vault.write(reinterpret_cast<const char*>(
                    tail.ciphertext.data()), tail.ciphertext.size());
            entry.tag = tail.tag;
            src.close();
        }

        // write the encrypted file table at current position
        emit progress(90, "Writing file table...");
        uint64_t tableOff = vault.pos();

        Bytes rawTable = packTable(entries);
        Bytes tableIV  = crypto::randomBytes(crypto::IV_LEN);
        auto  sealed   = crypto::encrypt(rawTable, key, tableIV);

        vault.write(reinterpret_cast<const char*>(sealed.ciphertext.data()),
                    sealed.ciphertext.size());

        // go back and write the real header
        VaultHeader hdr;
        hdr.salt      = salt;
        hdr.keyCheck  = kfp;
        hdr.fileCount = entries.size();
        hdr.tableOff  = tableOff;
        hdr.tableSize = sealed.ciphertext.size();
        hdr.tableIV   = tableIV;
        hdr.tableTag  = sealed.tag;

        Bytes packedHdr = packHeader(hdr);
        vault.seek(0);
        vault.write(reinterpret_cast<const char*>(packedHdr.data()),
                    packedHdr.size());
        vault.close();

        // wipe originals
        emit progress(94, "Removing original files...");
        for (auto& entry : entries) {
            if (!entry.isDir) {
                QFile::remove(QDir(drivePath).filePath(
                    QString::fromStdString(entry.path)));
            }
        }
        // dirs deepest first
        std::vector<std::string> dirList;
        for (auto& entry : entries)
            if (entry.isDir) dirList.push_back(entry.path);
        std::sort(dirList.begin(), dirList.end(),
                  [](const std::string& a, const std::string& b) {
                      return a.size() > b.size();
                  });
        QDir root(drivePath);
        for (auto& d : dirList)
            root.rmdir(QString::fromStdString(d));

        emit progress(100, "Done.");
        emit finished(true, "Drive encrypted successfully.");

    } catch (const std::exception& ex) {
        emit finished(false, QString("Encryption failed: %1").arg(ex.what()));
    }
}

void VaultWorker::decrypt(const QString& drivePath, const QString& password) {
    try {
        QString vaultPath = QDir(drivePath).filePath(VAULT_FILE);
        QFile vault(vaultPath);
        if (!vault.open(QIODevice::ReadOnly)) {
            emit finished(false, "Cannot open vault file.");
            return;
        }

        emit progress(5, "Reading vault...");
        Bytes hdrBuf(HEADER_SIZE);
        vault.read(reinterpret_cast<char*>(hdrBuf.data()), HEADER_SIZE);
        VaultHeader hdr = unpackHeader(hdrBuf);

        if (hdr.version != FORMAT_VERSION) {
            emit finished(false, "Unsupported vault version.");
            return;
        }

        emit progress(8, "Verifying password...");
        Bytes key = crypto::deriveKey(password.toStdString(), hdr.salt);
        Bytes kfp = crypto::keyFingerprint(key);

        if (kfp != hdr.keyCheck) {
            emit finished(false, "Wrong password.");
            return;
        }

        // read and decrypt file table
        emit progress(14, "Reading file table...");
        vault.seek(hdr.tableOff);
        Bytes tableCt(hdr.tableSize);
        vault.read(reinterpret_cast<char*>(tableCt.data()), hdr.tableSize);

        Bytes tableRaw = crypto::decrypt(tableCt, key, hdr.tableIV, hdr.tableTag);
        auto entries = unpackTable(tableRaw, hdr.fileCount);

        uint64_t totalBytes = 0;
        for (auto& e : entries)
            if (!e.isDir) totalBytes += e.size;

        // restore directories
        QDir root(drivePath);
        for (auto& e : entries)
            if (e.isDir) root.mkpath(QString::fromStdString(e.path));

        // decrypt files
        uint64_t done = 0;
        for (auto& entry : entries) {
            if (entry.isDir) continue;

            int pct = 18 + static_cast<int>(
                78.0 * done / std::max(totalBytes, uint64_t(1)));
            emit progress(pct,
                QString("Decrypting %1").arg(
                    QString::fromStdString(entry.path)));

            QString outPath = root.filePath(
                QString::fromStdString(entry.path));
            QFileInfo(outPath).dir().mkpath(".");

            QFile out(outPath);
            if (!out.open(QIODevice::WriteOnly)) {
                emit finished(false, "Cannot write: " + outPath);
                return;
            }

            vault.seek(entry.dataOffset);
            crypto::StreamDecryptor dec(key, entry.iv, entry.tag);

            uint64_t remaining = entry.size;
            while (remaining > 0) {
                int chunk = static_cast<int>(
                    std::min(remaining, uint64_t(CHUNK_SIZE)));
                QByteArray raw = vault.read(chunk);
                Bytes plain = dec.update(
                    reinterpret_cast<const uint8_t*>(raw.constData()),
                    static_cast<int>(raw.size()));
                out.write(reinterpret_cast<const char*>(plain.data()),
                          plain.size());
                remaining -= raw.size();
                done += raw.size();

                pct = 18 + static_cast<int>(
                    78.0 * done / std::max(totalBytes, uint64_t(1)));
                emit progress(pct,
                    QString("Decrypting %1").arg(
                        QString::fromStdString(entry.path)));
            }

            Bytes tail = dec.finish();
            if (!tail.empty())
                out.write(reinterpret_cast<const char*>(tail.data()),
                          tail.size());
            out.close();
        }

        vault.close();

        emit progress(98, "Cleaning up...");
        QFile::remove(vaultPath);

        emit progress(100, "Done.");
        emit finished(true, "Drive decrypted successfully.");

    } catch (const std::exception& ex) {
        emit finished(false, QString("Decryption failed: %1").arg(ex.what()));
    }
}

}
