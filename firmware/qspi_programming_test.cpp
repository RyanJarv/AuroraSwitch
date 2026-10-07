// Synthetic fault injection for the writer's fixed bounds and complete readback.
#include "../support/qspi_image_programming.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <vector>
using namespace aurora_selector;

struct Driver
{
    std::array<std::uint8_t, QspiApplicationEnd + 4096U> nor{};
    unsigned erase_fail = 0, page_fail = 0, erase_count = 0, page_count = 0;
    bool init_fail = false, corrupt = false, unmapped = false;
    std::vector<std::uint32_t> erased;
    Driver() { nor.fill(0x5a); }
    bool Initialize() { return !init_fail; }
    bool EraseSector(std::uint32_t offset)
    {
        assert(offset >= QspiApplicationOffset && offset + 4096 <= QspiApplicationEnd);
        erased.push_back(offset);
        if(++erase_count == erase_fail) return false;
        std::memset(nor.data() + offset, 255, 4096);
        return true;
    }
    bool WritePage(std::uint32_t offset, std::size_t size, const std::uint8_t* data)
    {
        assert((offset & 255U) == 0 && size > 0 && size <= 256);
        assert(offset >= QspiApplicationOffset && offset + size <= QspiApplicationOffset + DirtImageSize);
        if(++page_count == page_fail) return false;
        std::memcpy(nor.data() + offset, data, size);
        return true;
    }
    const std::uint8_t* MappedData()
    {
        if(corrupt) nor[QspiApplicationOffset + DirtImageSize - 1] ^= 1;
        return unmapped ? nullptr : nor.data() + QspiApplicationOffset;
    }
};

int main()
{
    std::array<std::uint8_t, DirtImageSize> bytes{};
    bytes.fill(0x35);
    volatile QspiDiagnostic diagnostic{};
    const auto good = [] { return true; };
    Driver driver;
    assert(ProgramQspiImage(driver, bytes.data(), bytes.size(), good, diagnostic) == QspiResult::Ready);
    assert(diagnostic.erase_attempts == 24 && diagnostic.pages == 372);
    assert(diagnostic.verified == bytes.size() && diagnostic.stage == 5);
    for(unsigned i = 0; i < QspiApplicationOffset; ++i) assert(driver.nor[i] == 0x5a);
    for(unsigned i = QspiApplicationEnd; i < driver.nor.size(); ++i) assert(driver.nor[i] == 0x5a);
    for(unsigned i = QspiApplicationOffset + bytes.size(); i < QspiApplicationEnd; ++i)
        assert(driver.nor[i] == 0xff);
    for(unsigned i = 0; i < 5; ++i)
    {
        Driver failed;
        if(i == 0) failed.init_fail = true;
        if(i == 1) failed.erase_fail = 2;
        if(i == 2) failed.page_fail = 2;
        if(i == 3) failed.corrupt = true;
        if(i == 4) failed.unmapped = true;
        assert(ProgramQspiImage(failed, bytes.data(), bytes.size(), good, diagnostic) != QspiResult::Ready);
        assert(diagnostic.verified == 0);
    }
    Driver rejected;
    assert(ProgramQspiImage(rejected, bytes.data(), bytes.size() - 1, good, diagnostic) == QspiResult::Rejected);
    assert(ProgramQspiImage(rejected, bytes.data(), bytes.size(), [] { return false; }, diagnostic) == QspiResult::Rejected);
    assert(rejected.erase_count == 0 && rejected.page_count == 0);
    unsigned validations = 0;
    Driver changed;
    assert(ProgramQspiImage(changed, bytes.data(), bytes.size(), [&] { return ++validations == 1; }, diagnostic)
           == QspiResult::ReadbackFailed);
}
