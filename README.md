<p align="center">
  <img src="https://github.com/user-attachments/assets/02dc5dc8-37f5-4744-95a1-2dca28496e38" 
       alt="USBunker Screenshot" 
       width="800" 
       height="auto">
</p>

# USBunker

Your USB drive's personal bodyguard. Because even flash drives deserve some privacy.

USBunker is a desktop application that encrypts the entire contents of a USB drive into a single AES-256-GCM vault file. No cloud, no account, no subscription -- just a password and your data stays yours.

Think of it as putting your USB stick into a bunker. A *really* small bunker. That fits in your pocket.

---

## What It Does

You plug in a USB stick, click **Encrypt**, pick a password, and USBunker packs every file on the drive into an encrypted vault. The original files are removed. To get them back, open USBunker, plug in the drive, and hit **Decrypt**.

That's it. No PhD required. (Though if you have one, it still works the same way.)

## Features

- **AES-256-GCM encryption** -- the same algorithm banks and governments trust, but for your vacation photos and questionable meme collection
- **Built-in file explorer** -- browse your drive's contents before encrypting, with color-coded file type icons so you can spot that one important PDF among 400 screenshots
- **Cross-platform file opening** -- double-click any file in the explorer to open it with your system's default application, works on both Linux and Windows
- **Dark mode UI** -- because it's 2026 and light mode at 2 AM is a war crime
- **Animated progress tracking** -- a smooth circular progress ring so you have something to stare at while your 64 GB of "totally important files" gets encrypted
- **Password strength meter** -- gently judges your password choices in real time
- **Custom vault format** -- `.bunker` files can only be opened with USBunker, so even if someone finds your drive, they'll just see one mysterious file and wonder what life choices led you here
- **Zero network access** -- USBunker never phones home, never uploads anything, never even looks at the internet

## How The Encryption Works

For the security-minded among you (and anyone who just likes reading about crypto at 3 AM):

1. **Key derivation**: Your password goes through PBKDF2-HMAC-SHA512 with 600,000 iterations and a random 32-byte salt. This turns "password123" into something actually useful. (Please don't use "password123".)

2. **File encryption**: Each file is encrypted individually using AES-256-GCM with a unique random IV. GCM mode provides both confidentiality and authenticity -- meaning the data is not only unreadable but also tamper-proof.

3. **Vault structure**:
   ```
   [Header - 256 bytes]        Magic, version, salt, key check, table metadata
   [Encrypted file data]       Each file encrypted with its own IV + auth tag
   [Encrypted file table]      File paths, sizes, offsets -- also encrypted
   ```

4. **Key verification**: A HMAC-SHA256 fingerprint of the derived key is stored in the header. This lets USBunker check if your password is correct *before* attempting to decrypt 50 GB of data. You're welcome.

5. **Cleanup**: After encryption, original files are deleted from the drive. After decryption, the vault file is removed. The drive always contains either your files or the vault -- never both at the same time.

## Building

### Prerequisites

- **CMake** 3.20 or newer
- **Qt6** (Widgets and Svg modules)
- **OpenSSL** (libcrypto)
- A C++17 compiler (GCC, Clang, MSVC -- dealer's choice)

### Linux

```bash
# install dependencies (Debian/Ubuntu)
sudo apt install cmake qt6-base-dev libqt6svg6-dev libssl-dev

# install dependencies (Arch)
sudo pacman -S cmake qt6-base qt6-svg openssl

# build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# run
./usbunker
```

### Windows

```powershell
# assuming Qt6 and OpenSSL are installed and in your PATH
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# run
.\Release\usbunker.exe
```

Or open the project in Qt Creator and hit the big green play button. We don't judge.

## Usage

1. **Plug in** a USB drive
2. **Launch** USBunker
3. **Click** "Scan for Drives" -- your drive should appear with its name, size, and current status
4. **Browse** your files in the built-in explorer (double-click to open, "Back" to navigate up)
5. **Encrypt**: Click "Encrypt Drive", set a password (8+ characters, mix it up a bit), confirm, and wait
6. **Decrypt**: Click "Decrypt Drive", enter your password, and wait

The progress ring will keep you company. It's not *fast* company, but it's honest company.

## File Type Icons

The explorer uses color-coded icons to help you identify files at a glance:

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
    crypto.h/cpp     -- AES-256-GCM, PBKDF2, streaming encrypt/decrypt
    vault.h/cpp      -- vault format, threaded encrypt/decrypt worker
    usb.h/cpp        -- cross-platform removable drive detection
    types.h          -- shared types and constants
  ui/
    main_window.h/cpp   -- application shell, page management, transitions
    file_explorer.h/cpp -- file browser with custom icon provider
    widgets.h/cpp       -- ProgressRing, DriveCard, StrengthMeter
    style.h             -- dark theme stylesheet
  main.cpp           -- entry point
resources/
  icons/             -- hand-crafted SVG icons
  resources.qrc      -- Qt resource index
```

## Security Notes

- USBunker is designed for personal use. It protects against casual snooping and lost-drive scenarios. It is *not* a replacement for full-disk encryption solutions like LUKS or BitLocker if you're defending against state-level adversaries. (If you are, you probably have bigger problems than USB encryption.)
- The vault format includes no file metadata in plaintext -- file names, paths, and sizes are all encrypted in the file table.
- Key derivation uses 600,000 PBKDF2 iterations, which makes brute-force attacks slow and expensive. But no amount of iterations can save "1234" as a password. Please try harder.
- USBunker deletes original files after encryption, but does not perform secure wipe (multiple overwrites). On flash storage, secure wipe is unreliable due to wear leveling anyway. If you need that level of assurance, physically destroy the drive. A hammer works. So does a blender, but that voids the warranty.

## License

MIT -- see [LICENSE](LICENSE) for details.

Do whatever you want with it. Encrypt responsibly.
