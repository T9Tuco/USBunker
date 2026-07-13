#pragma once

#include "types.h"
#include <memory>
#include <stdexcept>

namespace bunker::crypto {

constexpr int SALT_LEN       = 32;
constexpr int IV_LEN         = 12;
constexpr int KEY_LEN        = 32;
constexpr int TAG_LEN        = 16;
constexpr int KDF_ITERATIONS = 600000;

struct CryptoError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Sealed {
    Bytes ciphertext;
    Bytes tag;
};

Bytes randomBytes(int count);
Bytes deriveKey(const std::string& password, const Bytes& salt);
Bytes keyFingerprint(const Bytes& key);

// constant-time comparison for secret-derived values (avoids timing side channels)
bool constantTimeEqual(const Bytes& a, const Bytes& b);

Sealed encrypt(const Bytes& data, const Bytes& key, const Bytes& iv);
Bytes  decrypt(const Bytes& ciphertext, const Bytes& key,
               const Bytes& iv, const Bytes& tag);

void wipe(void* ptr, size_t len);
void wipe(Bytes& data);
void wipe(std::string& str);

class StreamEncryptor {
public:
    StreamEncryptor(const Bytes& key, const Bytes& iv);
    ~StreamEncryptor();
    StreamEncryptor(const StreamEncryptor&) = delete;
    StreamEncryptor& operator=(const StreamEncryptor&) = delete;

    Bytes  update(const uint8_t* data, int len);
    Sealed finish();

private:
    struct Ctx;
    std::unique_ptr<Ctx> m;
};

class StreamDecryptor {
public:
    StreamDecryptor(const Bytes& key, const Bytes& iv, const Bytes& tag);
    ~StreamDecryptor();
    StreamDecryptor(const StreamDecryptor&) = delete;
    StreamDecryptor& operator=(const StreamDecryptor&) = delete;

    Bytes update(const uint8_t* data, int len);
    Bytes finish();

private:
    struct Ctx;
    std::unique_ptr<Ctx> m;
};

}
