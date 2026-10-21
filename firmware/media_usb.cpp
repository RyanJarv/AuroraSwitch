// Hardware I/O only. File policy is in file_media.hpp and staging helpers.
#include "daisy_core.h"
#include "hid/usb_host.h"
#include "fatfs.h"
#include "media_transport.hpp"

namespace aurora {
extern daisy::USBHostHandle usb;
}

namespace {
// BOOT_SRAM globals are otherwise in DTCM, which USB DMA cannot access.
DMA_BUFFER_MEM_SECTION __attribute__((aligned(32))) daisy::FatFSInterface filesystem;
DMA_BUFFER_MEM_SECTION __attribute__((aligned(32))) FIL file;
FRESULT result = FR_OK;
} // namespace

namespace aurora_selector {
daisy_development::MediaInitialization MediaTransport::Initialize() {
    daisy::USBHostHandle::Config usb_config;
    daisy::FatFSInterface::Config fs_config;
    fs_config.media = daisy::FatFSInterface::Config::MEDIA_USB;
    return daisy_development::InitializeMedia(
        [&] { return aurora::usb.Init(usb_config) == daisy::USBHostHandle::Result::OK; },
        [&] { return filesystem.Init(fs_config) == daisy::FatFSInterface::Result::OK; },
        [&] {
            return f_mount(&filesystem.GetUSBFileSystem(), filesystem.GetUSBPath(), 0) == FR_OK;
        });
}

bool MediaTransport::Ready() {
    return aurora::usb.GetReady();
}

void MediaTransport::Pump() {
    aurora::usb.Process();
}

FileStatus MediaTransport::Open(const char* path, bool write) {
    result = f_open(&file, path, write ? FA_WRITE | FA_CREATE_ALWAYS : FA_READ);
    return result == FR_OK                                ? FileStatus::Ok
           : result == FR_NO_FILE || result == FR_NO_PATH ? FileStatus::Missing
                                                          : FileStatus::Error;
}

std::size_t MediaTransport::Size() {
    return f_size(&file);
}

bool MediaTransport::Read(std::uint8_t* bytes, std::size_t amount, std::size_t& count) {
    UINT transferred = 0;
    result = f_read(&file, bytes, static_cast<UINT>(amount), &transferred);
    count = transferred;
    return result == FR_OK;
}

bool MediaTransport::Write(const std::uint8_t* bytes, std::size_t amount, std::size_t& count) {
    UINT transferred = 0;
    result = f_write(&file, bytes, static_cast<UINT>(amount), &transferred);
    count = transferred;
    return result == FR_OK;
}

bool MediaTransport::Sync() {
    result = f_sync(&file);
    return result == FR_OK;
}

bool MediaTransport::Close() {
    result = f_close(&file);
    return result == FR_OK;
}

std::uint32_t MediaTransport::Result() {
    return result;
}

std::uint32_t MediaTransport::SuccessResult() {
    return FR_OK;
}

bool MediaTransport::Unmount() {
    return f_mount(nullptr, filesystem.GetUSBPath(), 0) == FR_OK;
}

void MediaTransport::Stop() {
    aurora::usb.Deinit();
}
} // namespace aurora_selector
