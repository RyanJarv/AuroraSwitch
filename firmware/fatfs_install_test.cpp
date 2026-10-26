// Run our USB move helpers against the pinned, real FatFs on a disposable image.
// Disk calls use an ordinary local file, never a USB device or mounted volume.
#include "usb_files.hpp"
#include "diskio.h"
#include <cassert>
#include <vector>

static FILE* disk;
extern "C" {
DSTATUS disk_initialize(BYTE) {
    return 0;
}
DSTATUS disk_status(BYTE) {
    return 0;
}
DRESULT disk_read(BYTE, BYTE* bytes, DWORD sector, UINT count) {
    return std::fseek(disk, long(sector) * 512, SEEK_SET) == 0 &&
                   std::fread(bytes, 512, count, disk) == count
               ? RES_OK
               : RES_ERROR;
}
DRESULT disk_write(BYTE, const BYTE* bytes, DWORD sector, UINT count) {
    return std::fseek(disk, long(sector) * 512, SEEK_SET) == 0 &&
                   std::fwrite(bytes, 512, count, disk) == count
               ? RES_OK
               : RES_ERROR;
}
DRESULT disk_ioctl(BYTE, BYTE command, void*) {
    return command == CTRL_SYNC && std::fflush(disk) == 0 ? RES_OK : RES_PARERR;
}
DWORD get_fattime() {
    return 0;
}
}
int main(int argc, char** argv) {
    using namespace aurora_selector;
    static_assert(sizeof(DWORD) == 4, "Match the embedded FAT integer width");
    assert(argc == 2);
    disk = std::fopen(argv[1], "r+b");
    assert(disk);
    FATFS filesystem{};
    assert(f_mount(&filesystem, "0:/", 1) == FR_OK);
    assert(f_mkdir("0:/aurora") == FR_OK);
    const unsigned dirt = ImageCount - 1;
    char source[128];
    FirmwarePath(source, sizeof(source), dirt);
    std::vector<std::uint8_t> original(Images[dirt].size);
    for (std::size_t i = 0; i < original.size(); ++i)
        original[i] = i % 251;
    assert(f_open(&usb_file, source, FA_WRITE | FA_CREATE_NEW) == FR_OK);
    UINT written = 0;
    assert(f_write(&usb_file, original.data(), original.size(), &written) == FR_OK);
    assert(written == original.size() && f_close(&usb_file) == FR_OK);
    // A misleading long name must also block a switch on the real filesystem.
    assert(f_open(&usb_file, "0:/another.bin.disabled", FA_WRITE | FA_CREATE_NEW) == FR_OK);
    assert(f_close(&usb_file) == FR_OK);
    assert(!CheckInstallRoot());
    assert(f_rename("0:/another.bin.disabled", "0:/AuroraSwitch.img") == FR_OK);
    assert(CheckInstallRoot());
    assert(MoveToInstallRoot(dirt));
    assert(f_mount(nullptr, "0:/", 0) == FR_OK);
    assert(f_mount(&filesystem, "0:/", 1) == FR_OK);
    assert(CheckInstallRoot(true));
    std::vector<std::uint8_t> readback(StagingCapacity());
    assert(ReadImageFile(InstallPath, dirt, readback.data(), readback.size()));
    assert(std::equal(original.begin(), original.end(), readback.begin()));
    // Exercise the documented computer restoration using the same FAT engine.
    assert(f_rename(InstallPath, source) == FR_OK);
    assert(f_rename("0:/AuroraSwitch.img", "0:/AuroraSwitch.bin") == FR_OK);
    assert(!CheckInstallRoot());
    assert(ReadOriginal(dirt, readback.data(), readback.size()));
    assert(std::equal(original.begin(), original.end(), readback.begin()));
    assert(f_mount(nullptr, "0:/", 0) == FR_OK);
    assert(std::fclose(disk) == 0);
}
