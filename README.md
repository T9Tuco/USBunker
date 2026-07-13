<p align="center">
  <img src="https://github.com/user-attachments/assets/02dc5dc8-37f5-4744-95a1-2dca28496e38" 
       alt="USBunker Screenshot" 
       width="600" 
       height="auto">
</p>

# USBunker

[![Build](https://github.com/T9Tuco/USBunker/actions/workflows/release.yml/badge.svg)](https://github.com/T9Tuco/USBunker/actions/workflows/release.yml)
[![CI](https://github.com/T9Tuco/USBunker/actions/workflows/ci.yml/badge.svg)](https://github.com/T9Tuco/USBunker/actions/workflows/ci.yml)
[![Latest Release](https://img.shields.io/github/v/release/T9Tuco/USBunker)](https://github.com/T9Tuco/USBunker/releases/latest)
[![AUR Version](https://img.shields.io/aur/version/usbunker?label=AUR%20stable&color=4f8ef7)](https://aur.archlinux.org/packages/usbunker)
[![AUR Version](https://img.shields.io/aur/version/usbunker-git?label=AUR%20git&color=6ea8fe)](https://aur.archlinux.org/packages/usbunker-git)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

USBunker is a desktop application that encrypts the entire contents of a USB drive into a single AES-256-GCM vault file. There's no cloud sync, no account, and no subscription. You set a password, and the drive's contents stay yours.

---

## Download

**Windows**: grab the latest `USBunker-windows-x64.zip` from the [Releases](https://github.com/T9Tuco/USBunker/releases) page, unzip it, and run `usbunker.exe`. No installation is needed, all dependencies are bundled.

**Linux (AppImage)**: grab the latest `USBunker-linux-x86_64.AppImage` from the [Releases](https://github.com/T9Tuco/USBunker/releases) page, mark it executable with `chmod +x`, and run it. No installation is needed.

**Linux (Arch / AUR)**: two packages are available.

```bash
yay -S usbunker      # stable release
yay -S usbunker-git  # latest commit
```

The stable package tracks tagged releases, and the git package builds from whatever is currently on `main`. Both install the binary, the desktop entry, and the icon.

**Linux (other distros)**: build from source. See [Building](#building) below.

---

## What It Does

You plug in a USB drive, click **Encrypt**, pick a password, and USBunker packs every file on the drive into an encrypted vault. The original files are removed. To get them back, open USBunker, plug in the drive, and click **Decrypt**.

## Features

- **AES-256-GCM encryption**, the same authenticated cipher used in TLS and most modern disk encryption tools
- **Built-in file explorer** to browse a drive's contents before encrypting, with color-coded file type icons
- **Cross-platform file opening**: double-click a file in the explorer to open it with your system's default application, on both Linux and Windows
- **Dark mode UI**
- **Animated progress tracking** with a circular progress indicator during encryption and decryption
- **Password strength meter** with live feedback while typing
- **Custom vault format**: `.bunker` files only open with USBunker, so a lost or found drive reveals nothing about its contents
- **No network access**: USBunker never connects to the internet

## How The Encryption Works

1. **Key derivation**: the password is run through PBKDF2-HMAC-SHA512 with 600,000 iterations and a random 32-byte salt.

2. **File encryption**: each file is encrypted individually with AES-256-GCM and a unique random IV. GCM mode provides both confidentiality and integrity, so tampered ciphertext fails to decrypt rather than decrypting to corrupted data silently.

3. **Vault structure**:
   ```
   [Header, 256 bytes]        magic, version, salt, key check, table metadata
   [Encrypted file data]      each file encrypted with its own IV and auth tag
   [Encrypted file table]     file paths, sizes, and offsets, also encrypted
   ```

4. **Key verification**: an HMAC-SHA256 fingerprint of the derived key is stored in the header, compared in constant time. This lets USBunker reject a wrong password immediately, before attempting to decrypt any file data.

5. **Cleanup**: after encryption, the original files are deleted from the drive. After decryption, the vault file is removed. The drive holds either the plaintext files or the vault, never both at once.

## Building

### Prerequisites

- **CMake** 3.20 or newer
- **Qt6** (Widgets and Svg modules)
- **OpenSSL** (libcrypto)
- A C++17 compiler (GCC, Clang, or MSVC)

### Linux

**1. Clone the repo**

```bash
git clone https://github.com/T9Tuco/USBunker.git
cd USBunker
```

**2. Install dependencies**

```bash
# Debian / Ubuntu
sudo apt install cmake qt6-base-dev libqt6svg6-dev libssl-dev

# Arch / Manjaro
sudo pacman -S cmake qt6-base qt6-svg openssl

# Fedora
sudo dnf install cmake qt6-qtbase-devel qt6-qtsvg-devel openssl-devel
```

**3. Build and run**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/usbunker
```

### Windows

**1. Install dependencies**

- **Qt6**: download the installer from [qt.io/download-open-source](https://www.qt.io/download-open-source). During installation, select the *MSVC 2022 64-bit* (or MinGW) component under Qt 6.x.
- **OpenSSL**: install via winget or download a prebuilt binary from [slproweb.com/products/Win32OpenSSL.html](https://slproweb.com/products/Win32OpenSSL.html).

```powershell
winget install ShiningLight.OpenSSL
```

- **CMake**: `winget install Kitware.CMake`
- **MSVC**: install [Visual Studio](https://visualstudio.microsoft.com/) with the *Desktop development with C++* workload, or use the Build Tools variant.

**2. Clone the repo**

```powershell
git clone https://github.com/TucoT9/USBunker.git
cd USBunker
```

**3. Build and run**

Replace `C:\Qt\6.x.x\msvc2022_64` with your actual Qt installation path:

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
.\build\Release\usbunker.exe
```

Alternatively, open the project folder in Qt Creator and click Run. It handles the Qt path automatically.

## Usage

1. **Plug in** a USB drive
2. **Launch** USBunker
3. **Click** "Scan for Drives". The drive should appear with its name, size, and current status
4. **Browse** the files in the built-in explorer (double-click to open, "Back" to navigate up)
5. **Encrypt**: click "Encrypt Drive", set a password (8+ characters), confirm it, and wait
6. **Decrypt**: click "Decrypt Drive", enter the password, and wait

## File Type Icons

The explorer uses color-coded icons to help identify files at a glance:

| Color | Type |
|-------|------|
| Blue | Folders |
| Purple | Images |
| Orange | Videos |
| Lavender | Audio |
| Light blue | Documents |
| Yellow | Archives |
| Green | Code / scripts |
| Gray | Everything else |

## Project Structure

```
src/
  core/
    crypto.h/cpp         AES-256-GCM, PBKDF2, streaming encrypt/decrypt
    vault.h/cpp           vault format, threaded encrypt/decrypt worker
    usb.h/cpp             cross-platform removable drive detection
    types.h                shared types and constants
  ui/
    main_window.h/cpp    application shell, page management, transitions
    file_explorer.h/cpp  file browser with custom icon provider
    widgets.h/cpp         ProgressRing, DriveCard, StrengthMeter
    style.h                 dark theme stylesheet
  main.cpp                entry point
resources/
  icons/                    hand-crafted SVG icons
  resources.qrc          Qt resource index
```

## Security Notes

- USBunker is built for personal use. It protects against casual snooping and a lost or stolen drive. It is not a replacement for full-disk encryption tools like LUKS or BitLocker against a more capable adversary.
- The vault format stores no file metadata in plaintext. File names, paths, and sizes are all encrypted inside the file table.
- Key derivation uses 600,000 PBKDF2 iterations, which makes brute-force password guessing slow. This does not help if the password itself is weak.
- USBunker deletes original files after encryption but does not perform a secure wipe with multiple overwrites. On flash storage, wear leveling makes secure wipe unreliable regardless. If you need that guarantee, destroy the drive physically.

## License

MIT. See [LICENSE](LICENSE) for details.
