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
inline bool ReadOriginal(unsigned index, std::uint8_t* staged, std::size_t capacity) {
    if (index >= ImageCount)
        return false;
    char path[128];
    FirmwarePath(path, sizeof(path), index);
    return ReadImageFile(path, index, staged, capacity);
}

constexpr const char* InstallPath = "0:/DIRTVERB.BIN";
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
inline bool CheckInstallRoot(bool installed = false) {
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
            if (!installed || std::strcmp(info.fname, "DIRTVERB.BIN") != 0) {
                valid = false;
                break;
            }
        }
    }
    return f_closedir(&directory) == FR_OK && valid && matches == (installed ? 1U : 0U);
}
// Once rename is attempted, its failure can leave directory changes behind.
// The controller latches that boundary; never roll back or retry automatically.
inline bool MoveToInstallRoot(unsigned index) {
    if (index >= ImageCount || Images[index].launch != LaunchMethod::StockUsbInstall)
        return false;
    char source[128];
    FirmwarePath(source, sizeof(source), index);
    if (f_rename(source, InstallPath) != FR_OK)
        return false;
    FILINFO info{};
    return f_stat(source, &info) == FR_NO_FILE && CheckInstallRoot(true);
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
