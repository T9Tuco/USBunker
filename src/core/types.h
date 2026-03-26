#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace bunker {

using Bytes = std::vector<uint8_t>;

constexpr const char* APP_NAME    = "USBunker";
constexpr const char* VAULT_FILE  = "vault.bunker";
constexpr uint32_t FORMAT_VERSION = 1;

}
