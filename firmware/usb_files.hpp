#pragma once
#include "images.hpp"
#include "../support/usb_choice.hpp"
#include "ff.h"
#include <cstdio>
#include <algorithm>

namespace aurora_selector {
// Concrete FatFs operations shared by production and native failure tests.
// All paths are on the USB volume; nothing here initializes on-module storage.
// USB DMA cannot access the DTCM stack/default data. FatFs sector windows
// and partial-sector choice transfers therefore use the existing DMA arena.
inline __attribute__((section(".sram1_bss.usb_file"), aligned(32))) FIL usb_file;
inline __attribute__((section(".sram1_bss.usb_choice"), aligned(32))) ChoiceRecord usb_choice;
constexpr const char* ChoicePath = "0:/aurora/choice.dat";
inline void FirmwarePath(char* path, std::size_t size, unsigned index) {
    std::snprintf(path, size, "0:/aurora/%s", Images[index].filename);
}
inline void DiscoverFiles(std::array<bool, ImageCount>& present) {
    for (unsigned index = 0; index < ImageCount; ++index) {
        char path[128];
        FirmwarePath(path, sizeof(path), index);
        FILINFO info{};
        present[index] = f_stat(path, &info) == FR_OK && !(info.fattrib & AM_DIR) &&
                         info.fsize == Images[index].size;
    }
}
inline bool ReadImageFile(const char* path, unsigned index, std::uint8_t* staged,
                          std::size_t capacity) {
    if (index >= ImageCount || staged == nullptr || capacity < Images[index].size)
        return false;
    auto& file = usb_file;
    if (f_open(&file, path, FA_READ) != FR_OK)
        return false;
    bool valid = f_size(&file) == Images[index].size;
    std::size_t offset = 0;
    while (valid && offset < Images[index].size) {
        // Bounded reads keep the SDK's USB transfer size within its limits.
        const auto count =
            static_cast<UINT>(std::min<std::size_t>(4096, Images[index].size - offset));
        UINT read = 0;
        valid = f_read(&file, staged + offset, count, &read) == FR_OK && read == count;
        offset += read;
    }
    valid = f_close(&file) == FR_OK && valid;
    return valid;
}

constexpr const char* InstallPath = "0:/DIRTVERB.BIN";
constexpr const char* SelectorPath = "0:/AuroraSwitch.bin";
constexpr const char* SelectorBackupPath = "0:/AuroraSwitch.disabled";
constexpr const char* ReturnPath = "0:/RETURN TO AURORASWITCH.txt";
constexpr char ReturnInstructions[] =
    "Delete DIRTVERB.BIN, then rename AuroraSwitch.disabled to AuroraSwitch.bin.\r\n"
    "Safely eject and restart.\r\n";

inline bool SameFilename(const char* left, const char* right) {
    for (; *left && *right; ++left, ++right) {
        const auto fold = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
        if (fold(*left) != fold(*right))
            return false;
    }
    return *left == *right;
}
enum class InstallRoot { Empty, SelectorAllowed, DirtOnly };
// The stock loader matches substrings, not just filename extensions. Be more
// conservative about case and hidden files, and inspect 8.3 aliases as well.
inline bool IsBinName(const char* name) {
    for (; *name; ++name) {
        if (name[0] == '.' && (name[1] == 'b' || name[1] == 'B') &&
            (name[2] == 'i' || name[2] == 'I') && (name[3] == 'n' || name[3] == 'N'))
            return true;
    }
    return false;
}
inline bool CheckInstallRoot(InstallRoot state = InstallRoot::Empty) {
    DIR directory{};
    if (f_opendir(&directory, "0:/") != FR_OK)
        return false;
    bool valid = true;
    unsigned matches = 0;
    // Bound a scan even on unexpectedly large or damaged directories.
    for (unsigned count = 0;; ++count) {
        FILINFO info{};
        if (count == 1024 || f_readdir(&directory, &info) != FR_OK) {
            valid = false;
            break;
        }
        if (!info.fname[0])
            break;
        if (!(info.fattrib & AM_DIR) && (IsBinName(info.fname) || IsBinName(info.altname))) {
            ++matches;
            const char* allowed =
                state == InstallRoot::DirtOnly ? "DIRTVERB.BIN" : "AuroraSwitch.bin";
            if (state == InstallRoot::Empty || (info.fattrib & AM_HID) ||
                !SameFilename(info.fname, allowed)) {
                valid = false;
                break;
            }
        }
    }
    return f_closedir(&directory) == FR_OK && valid &&
           (state == InstallRoot::DirtOnly ? matches == 1U : matches <= 1U);
}
inline bool CheckInstallFiles(unsigned index) {
    if (index >= ImageCount || Images[index].launch != LaunchMethod::StockUsbInstall ||
        !CheckInstallRoot(InstallRoot::SelectorAllowed))
        return false;
    char source[128];
    FirmwarePath(source, sizeof(source), index);
    FILINFO info{};
    // Keep the supported source visible and refuse directories before any
    // confirmed operation changes the drive.
    return f_stat(source, &info) == FR_OK && !(info.fattrib & (AM_DIR | AM_HID));
}
// Read a complete file and compare it byte-for-byte with a known local copy.
// The caller supplies DMA-accessible scratch; no filesystem transfer uses DTCM.
inline bool MatchesFile(const char* path, const std::uint8_t* expected, std::size_t size,
                        std::uint8_t* scratch, std::size_t capacity) {
    if (!expected || !scratch || !size || !capacity || f_open(&usb_file, path, FA_READ) != FR_OK)
        return false;
    bool valid = f_size(&usb_file) == size;
    for (std::size_t offset = 0; valid && offset < size;) {
        const auto count =
            static_cast<UINT>(std::min({size - offset, capacity, std::size_t(4096)}));
        UINT read = 0;
        valid = f_read(&usb_file, scratch, count, &read) == FR_OK && read == count &&
                std::memcmp(scratch, expected + offset, count) == 0;
        offset += count;
    }
    return f_close(&usb_file) == FR_OK && valid;
}
inline bool VisibleFile(const char* path) {
    FILINFO info{};
    return f_stat(path, &info) == FR_OK && !(info.fattrib & (AM_DIR | AM_HID));
}
inline bool MissingFile(const char* path) {
    FILINFO info{};
    return f_stat(path, &info) == FR_NO_FILE;
}
// Never replace an existing file. Sync and close before any subsequent readback.
inline bool WriteNewFile(const char* path, const std::uint8_t* bytes, std::size_t size,
                         std::uint8_t* scratch, std::size_t capacity) {
    if (!bytes || !scratch || !size || !capacity ||
        f_open(&usb_file, path, FA_WRITE | FA_CREATE_NEW) != FR_OK)
        return false;
    bool valid = true;
    for (std::size_t offset = 0; valid && offset < size;) {
        const auto count =
            static_cast<UINT>(std::min({size - offset, capacity, std::size_t(4096)}));
        if (scratch != bytes + offset)
            std::memmove(scratch, bytes + offset, count);
        UINT written = 0;
        valid = f_write(&usb_file, scratch, count, &written) == FR_OK && written == count;
        offset += count;
    }
    valid = valid && f_sync(&usb_file) == FR_OK;
    return f_close(&usb_file) == FR_OK && valid;
}
inline const std::uint8_t* ReturnBytes() {
    return reinterpret_cast<const std::uint8_t*>(ReturnInstructions);
}
// Only our exact installed selector may be renamed. A different backup or text
// file is a conflict; nothing is overwritten, even if its name looks familiar.
inline bool CheckReturnFiles(const std::uint8_t* selector, std::size_t size, std::uint8_t* scratch,
                             std::size_t capacity) {
    if (!CheckInstallRoot(InstallRoot::SelectorAllowed))
        return false;
    if (!MissingFile(SelectorPath)) {
        if (!MissingFile(SelectorBackupPath) ||
            !MatchesFile(SelectorPath, selector, size, scratch, capacity))
            return false;
    } else if (!MissingFile(SelectorBackupPath) &&
               (!VisibleFile(SelectorBackupPath) ||
                !MatchesFile(SelectorBackupPath, selector, size, scratch, capacity))) {
        return false;
    }
    return MissingFile(ReturnPath) ||
           (VisibleFile(ReturnPath) &&
            MatchesFile(ReturnPath, ReturnBytes(), sizeof(ReturnInstructions) - 1, scratch,
                        capacity));
}
// Called only after confirmation and the controller's terminal-write boundary.
// If the installer was removed, save a copy of the installed image instead.
// Any failed rename/write/readback stops the operation without rollback/reset.
inline bool PrepareReturnFiles(const std::uint8_t* selector, std::size_t size,
                               std::uint8_t* scratch, std::size_t capacity) {
    if (!CheckReturnFiles(selector, size, scratch, capacity))
        return false;
    if (!MissingFile(SelectorPath)) {
        if (f_rename(SelectorPath, SelectorBackupPath) != FR_OK || !MissingFile(SelectorPath))
            return false;
    } else if (MissingFile(SelectorBackupPath) &&
               !WriteNewFile(SelectorBackupPath, selector, size, scratch, capacity)) {
        return false;
    }
    if (!MatchesFile(SelectorBackupPath, selector, size, scratch, capacity))
        return false;
    if (MissingFile(ReturnPath) &&
        !WriteNewFile(ReturnPath, ReturnBytes(), sizeof(ReturnInstructions) - 1, scratch, capacity))
        return false;
    return MatchesFile(ReturnPath, ReturnBytes(), sizeof(ReturnInstructions) - 1, scratch,
                       capacity) &&
           CheckInstallRoot();
}

inline int ReadChoice() {
    auto& file = usb_file;
    if (f_open(&file, ChoicePath, FA_READ) != FR_OK)
        return -1;
    auto& record = usb_choice;
    UINT read = 0;
    const bool valid = f_size(&file) == record.size() &&
                       f_read(&file, record.data(), record.size(), &read) == FR_OK &&
                       read == record.size();
    const bool closed = f_close(&file) == FR_OK;
    return valid && closed ? DecodeChoice(record) : -1;
}
inline bool WriteChoice(unsigned index) {
    if (index >= ImageCount || Images[index].launch != LaunchMethod::Ram)
        return false;
    if (ReadChoice() == static_cast<int>(index))
        return true;
    // One replace-in-place record. A torn save can lose the remembered choice,
    // which only opens the menu next boot. No journal or recovery copy needed.
    auto& record = usb_choice;
    record = EncodeChoice(index);
    auto& file = usb_file;
    if (f_open(&file, ChoicePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;
    UINT written = 0;
    const bool saved = f_write(&file, record.data(), record.size(), &written) == FR_OK &&
                       written == record.size() && f_sync(&file) == FR_OK;
    const bool closed = f_close(&file) == FR_OK;
    return saved && closed && ReadChoice() == static_cast<int>(index);
}
} // namespace aurora_selector
