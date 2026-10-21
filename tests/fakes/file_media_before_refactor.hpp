#pragma once
// Frozen physical method bodies from AuroraSwitch 9972d5d:firmware/selector.cpp.
// Only the surrounding includes and hardware globals are supplied by this fixture.
// Do not update this oracle when changing the implementation under test.
#include "aurora.h"
#include "../../firmware/file_media.hpp"
namespace before_refactor
{
    alignas(32) FIL selector_file;
    volatile aurora_selector::LoadDiagnostic load_diagnostic{};
    bool MediaReady() { return aurora::usb.GetReady(); }
    struct BootStore
    {
        bool healthy = true;
        bool Healthy() const { return healthy; }
        bool Read(unsigned slot, aurora_selector::BootRecord& record)
        {

            if(!MediaReady()) { healthy = false; return false; }
            const char* path = slot == 0 ? "0:/aurora/boot-a.dat" : "0:/aurora/boot-b.dat";
            const auto opened = f_open(&selector_file, path, FA_READ);
            if(opened != FR_OK)
            {
                if(opened != FR_NO_FILE && opened != FR_NO_PATH) healthy = false;
                return false;
            }
            UINT count = 0;
            const bool size_valid = f_size(&selector_file) == record.size();
            if(size_valid && (f_read(&selector_file, record.data(), record.size(), &count) != FR_OK
                             || count != record.size())) healthy = false;
            if(f_close(&selector_file) != FR_OK || !MediaReady()) healthy = false;
            return healthy && size_valid;

        }
        bool Write(unsigned slot, const aurora_selector::BootRecord& record)
        {

            if(!MediaReady()) return false;
            const char* path = slot == 0 ? "0:/aurora/boot-a.dat" : "0:/aurora/boot-b.dat";
            if(f_open(&selector_file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return false;
            UINT count = 0;
            const bool valid = f_write(&selector_file, record.data(), record.size(), &count) == FR_OK
                && count == record.size() && f_sync(&selector_file) == FR_OK;
            return f_close(&selector_file) == FR_OK && valid && MediaReady();

        }
    };

    struct UsbFileReader
    {
        bool Ready() { return aurora::usb.GetReady(); }
        bool Open(const char* path)
        {
            const auto result = f_open(&selector_file, path, FA_READ);
            load_diagnostic.fatfs_result = result;
            return result == FR_OK;
        }
        std::size_t Size()
        {
            load_diagnostic.file_size = f_size(&selector_file);
            return load_diagnostic.file_size;
        }
        bool Read(std::uint8_t* buffer, std::size_t amount, std::size_t& read)
        {
            UINT count = 0;
            const auto result = f_read(&selector_file, buffer, static_cast<UINT>(amount), &count);
            load_diagnostic.fatfs_result = result;
            load_diagnostic.bytes_read = load_diagnostic.bytes_read + count;
            read = count;
            return result == FR_OK;
        }
        void Pump() { aurora::usb.Process(); }
        bool Close()
        {
            const auto result = f_close(&selector_file);
            // Preserve the first operation failure rather than hide it by close.
            if(load_diagnostic.fatfs_result == FR_OK)
                load_diagnostic.fatfs_result = result;
            return result == FR_OK;
        }
    };
}
