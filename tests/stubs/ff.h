#pragma once
// In-memory FatFs boundary for native tests of the production file operations.
#include <cstddef>
#include <cstdint>
using UINT = unsigned;
using FRESULT = int;
constexpr int FR_OK = 0, FR_DISK_ERR = 1, FR_NO_FILE = 4;
constexpr unsigned FA_READ = 1, FA_WRITE = 2, FA_CREATE_ALWAYS = 8, AM_DIR = 16;
struct FIL {
    const char* path;
    std::size_t position;
    std::size_t size;
};
struct FILINFO {
    std::size_t fsize;
    unsigned fattrib;
};
inline std::size_t f_size(const FIL* file) {
    return file->size;
}
FRESULT f_stat(const char*, FILINFO*);
FRESULT f_open(FIL*, const char*, unsigned);
FRESULT f_read(FIL*, void*, UINT, UINT*);
FRESULT f_write(FIL*, const void*, UINT, UINT*);
FRESULT f_sync(FIL*);
FRESULT f_close(FIL*);
