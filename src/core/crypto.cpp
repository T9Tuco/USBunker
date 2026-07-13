#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>
#include <cstring>

namespace bunker::crypto {

Bytes randomBytes(int count) {
    Bytes buf(count);
    if (RAND_bytes(buf.data(), count) != 1)
        throw CryptoError("random generation failed");
    return buf;
}

Bytes deriveKey(const std::string& password, const Bytes& salt) {
    Bytes key(KEY_LEN);
    if (PKCS5_PBKDF2_HMAC(
            password.c_str(), static_cast<int>(password.size()),
            salt.data(), static_cast<int>(salt.size()),
            KDF_ITERATIONS, EVP_sha512(),
            KEY_LEN, key.data()) != 1) {
        throw CryptoError("key derivation failed");
    }
    return key;
}

Bytes keyFingerprint(const Bytes& key) {
    unsigned int len = 0;
    Bytes out(32);
    const char* label = "USBunker-verify";
    if (!HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
              reinterpret_cast<const uint8_t*>(label),
              static_cast<int>(std::strlen(label)),
              out.data(), &len)) {
        throw CryptoError("HMAC computation failed");
    }
    out.resize(len);
    return out;
}

bool constantTimeEqual(const Bytes& a, const Bytes& b) {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// -- secure memory wipe --

void wipe(void* ptr, size_t len) {
    OPENSSL_cleanse(ptr, len);
}

void wipe(Bytes& data) {
    if (!data.empty()) OPENSSL_cleanse(data.data(), data.size());
    data.clear();
}

void wipe(std::string& str) {
    if (!str.empty()) OPENSSL_cleanse(str.data(), str.size());
    str.clear();
}

// -- one-shot encrypt/decrypt --

Sealed encrypt(const Bytes& data, const Bytes& key, const Bytes& iv) {
    if (key.size() != KEY_LEN) throw CryptoError("invalid key size");
    if (iv.size()  != IV_LEN)  throw CryptoError("invalid IV size");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw CryptoError("cipher context allocation failed");

    Sealed result;
    result.ciphertext.resize(data.size() + 32);
    result.tag.resize(TAG_LEN);

    int len = 0, total = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("encrypt init failed");
    }

    if (EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len,
                          data.data(), static_cast<int>(data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("encrypt update failed");
    }
    total = len;

    if (EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("encrypt finalize failed");
    }
    total += len;
    result.ciphertext.resize(total);

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN,
                            result.tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("failed to retrieve auth tag");
    }

    EVP_CIPHER_CTX_free(ctx);
    return result;
}

Bytes decrypt(const Bytes& ciphertext, const Bytes& key,
              const Bytes& iv, const Bytes& tag) {
    if (key.size() != KEY_LEN) throw CryptoError("invalid key size");
    if (iv.size()  != IV_LEN)  throw CryptoError("invalid IV size");
    if (tag.size() != TAG_LEN) throw CryptoError("invalid auth tag size");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw CryptoError("cipher context allocation failed");

    Bytes plain(ciphertext.size() + 32);
    int len = 0, total = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("decrypt init failed");
    }

    if (EVP_DecryptUpdate(ctx, plain.data(), &len,
                          ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("decrypt update failed");
    }
    total = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN,
                            const_cast<uint8_t*>(tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("failed to set expected auth tag");
    }

    if (EVP_DecryptFinal_ex(ctx, plain.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw CryptoError("authentication failed - wrong password or corrupted data");
    }
    total += len;
    plain.resize(total);

    EVP_CIPHER_CTX_free(ctx);
    return plain;
}

// -- streaming encryptor --

struct StreamEncryptor::Ctx {
    EVP_CIPHER_CTX* handle = nullptr;
    ~Ctx() { if (handle) EVP_CIPHER_CTX_free(handle); }
};

StreamEncryptor::StreamEncryptor(const Bytes& key, const Bytes& iv)
    : m(std::make_unique<Ctx>())
{
    if (key.size() != KEY_LEN) throw CryptoError("invalid key size");
    if (iv.size()  != IV_LEN)  throw CryptoError("invalid IV size");

    m->handle = EVP_CIPHER_CTX_new();
    if (!m->handle) throw CryptoError("cipher context allocation failed");

    if (EVP_EncryptInit_ex(m->handle, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(m->handle, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1 ||
        EVP_EncryptInit_ex(m->handle, nullptr, nullptr, key.data(), iv.data()) != 1) {
        throw CryptoError("stream encrypt init failed");
    }
}

StreamEncryptor::~StreamEncryptor() = default;

Bytes StreamEncryptor::update(const uint8_t* data, int len) {
    Bytes out(len + 32);
    int written = 0;
    if (EVP_EncryptUpdate(m->handle, out.data(), &written, data, len) != 1)
        throw CryptoError("stream encrypt failed");
    out.resize(written);
    return out;
}

Sealed StreamEncryptor::finish() {
    Sealed result;
    result.ciphertext.resize(32);
    int written = 0;

    if (EVP_EncryptFinal_ex(m->handle, result.ciphertext.data(), &written) != 1)
        throw CryptoError("stream encrypt finalize failed");
    result.ciphertext.resize(written);

    result.tag.resize(TAG_LEN);
    if (EVP_CIPHER_CTX_ctrl(m->handle, EVP_CTRL_GCM_GET_TAG, TAG_LEN,
                            result.tag.data()) != 1) {
        throw CryptoError("failed to retrieve auth tag");
    }
    return result;
}

// -- streaming decryptor --

struct StreamDecryptor::Ctx {
    EVP_CIPHER_CTX* handle = nullptr;
    Bytes expectedTag;
    ~Ctx() { if (handle) EVP_CIPHER_CTX_free(handle); }
};

StreamDecryptor::StreamDecryptor(const Bytes& key, const Bytes& iv, const Bytes& tag)
    : m(std::make_unique<Ctx>())
{
    if (key.size() != KEY_LEN) throw CryptoError("invalid key size");
    if (iv.size()  != IV_LEN)  throw CryptoError("invalid IV size");
    if (tag.size() != TAG_LEN) throw CryptoError("invalid auth tag size");

    m->handle = EVP_CIPHER_CTX_new();
    m->expectedTag = tag;
    if (!m->handle) throw CryptoError("cipher context allocation failed");

    if (EVP_DecryptInit_ex(m->handle, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(m->handle, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1 ||
        EVP_DecryptInit_ex(m->handle, nullptr, nullptr, key.data(), iv.data()) != 1) {
        throw CryptoError("stream decrypt init failed");
    }
}

StreamDecryptor::~StreamDecryptor() = default;

Bytes StreamDecryptor::update(const uint8_t* data, int len) {
    Bytes out(len + 32);
    int written = 0;
    if (EVP_DecryptUpdate(m->handle, out.data(), &written, data, len) != 1)
        throw CryptoError("stream decrypt failed");
    out.resize(written);
    return out;
}

Bytes StreamDecryptor::finish() {
    if (EVP_CIPHER_CTX_ctrl(m->handle, EVP_CTRL_GCM_SET_TAG, TAG_LEN,
                            const_cast<uint8_t*>(m->expectedTag.data())) != 1) {
        throw CryptoError("failed to set expected auth tag");
    }

    Bytes out(32);
    int written = 0;
    if (EVP_DecryptFinal_ex(m->handle, out.data(), &written) != 1)
        throw CryptoError("authentication failed - wrong password or corrupted data");
    out.resize(written);
    return out;
}

}
