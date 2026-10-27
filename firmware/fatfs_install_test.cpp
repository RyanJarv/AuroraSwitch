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
    // Normal installation leaves the root selector BIN. Use patterned bytes
    // here; production compares against the installed selector's exact BIN.
    std::vector<std::uint8_t> selector(10003, 0x5a);
    std::vector<std::uint8_t> readback(StagingCapacity());
    assert(WriteNewFile(SelectorPath, selector.data(), selector.size(), readback.data(),
                        readback.size()));
    assert(f_open(&usb_file, "0:/another.bin.disabled", FA_WRITE | FA_CREATE_NEW) == FR_OK);
    assert(f_close(&usb_file) == FR_OK);
    assert(!CheckInstallFiles(dirt));
    assert(f_unlink("0:/another.bin.disabled") == FR_OK);
    assert(CheckInstallFiles(dirt));
    assert(CheckReturnFiles(selector.data(), selector.size(), readback.data(), readback.size()));
    assert(PrepareReturnFiles(selector.data(), selector.size(), readback.data(), readback.size()));
    assert(MissingFile(SelectorPath));
    assert(MatchesFile(SelectorBackupPath, selector.data(), selector.size(), readback.data(),
                       readback.size()));
    assert(MatchesFile(ReturnPath, ReturnBytes(), sizeof(ReturnInstructions) - 1, readback.data(),
                       readback.size()));
    assert(ReadOriginal(dirt, readback.data(), readback.size()));
    assert(WriteNewFile(InstallPath, readback.data(), original.size(), readback.data(),
                        readback.size()));
    assert(f_mount(nullptr, "0:/", 0) == FR_OK);
    assert(f_mount(&filesystem, "0:/", 1) == FR_OK);
    assert(CheckInstallRoot(InstallRoot::DirtOnly));
    assert(ReadImageFile(InstallPath, dirt, readback.data(), readback.size()));
    assert(std::equal(original.begin(), original.end(), readback.begin()));
    assert(ReadOriginal(dirt, readback.data(), readback.size()));
    assert(std::equal(original.begin(), original.end(), readback.begin()));
    // The computer only needs to delete Dirt's copy and re-enable the selector.
    assert(f_unlink(InstallPath) == FR_OK);
    assert(f_rename(SelectorBackupPath, SelectorPath) == FR_OK);
    assert(MatchesFile(SelectorPath, selector.data(), selector.size(), readback.data(),
                       readback.size()));
    assert(CheckInstallFiles(dirt));
    assert(f_mount(nullptr, "0:/", 0) == FR_OK);
    assert(std::fclose(disk) == 0);
}
