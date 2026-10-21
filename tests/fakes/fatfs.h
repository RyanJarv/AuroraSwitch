#pragma once
// Host declarations for the small FatFs boundary used by media_usb.cpp.
#include <cstddef>
#include <cstdint>
using UINT = unsigned;
using BYTE = unsigned char;
enum FRESULT { FR_OK, FR_DISK_ERR, FR_INT_ERR, FR_NOT_READY,
               FR_NO_FILE, FR_NO_PATH, FR_INVALID_NAME, FR_DENIED };
constexpr BYTE FA_READ = 1, FA_WRITE = 2, FA_CREATE_ALWAYS = 8;
struct FIL { std::size_t size = 0; };
struct FATFS {};
inline std::size_t f_size(FIL* file) { return file->size; }
FRESULT f_open(FIL*, const char*, BYTE);
FRESULT f_read(FIL*, void*, UINT, UINT*);
FRESULT f_write(FIL*, const void*, UINT, UINT*);
FRESULT f_sync(FIL*);
FRESULT f_close(FIL*);
FRESULT f_mount(FATFS*, const char*, BYTE);

namespace daisy
{
    struct FatFSInterface
    {
        struct Config { enum Media { MEDIA_USB }; Media media; };
        enum class Result { OK, ERR };
        Result Init(Config);
        FATFS& GetUSBFileSystem() { return filesystem; }
        const char* GetUSBPath() { return "0:"; }
        FATFS filesystem;
    };
}
