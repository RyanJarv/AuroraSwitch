#include "flash_driver.hpp"
#include <array>
#include <cassert>
#include <limits>

using namespace aurora_selector;

int main() {
    daisy::QSPIHandle sdk;
    ChoiceFlashDriver driver{sdk};
    std::array<std::uint8_t, 80> record{};
    for (unsigned i = 0; i < record.size(); ++i)
        record[i] = i;

    // Expected chip offsets/geometry are independent of production constants.
    // Check every accepted journal page, not just the first record.
    for (std::uint32_t offset = 0x7e0000; offset < 0x7e2000; offset += 256) {
        assert(driver.WritePage(offset, record.size(), record.data()));
        const auto& call = sdk.calls.back();
        assert(!call.erase && call.offset == offset);
        assert(call.bytes == std::vector<std::uint8_t>(record.begin(), record.end()));
    }
    for (auto offset : {0x7e0000U, 0x7e1000U}) {
        assert(driver.EraseSector(offset));
        assert(sdk.calls.back().erase && sdk.calls.back().offset == offset);
    }
    const auto accepted = sdk.calls.size();
    for (auto offset : {0U, 0x1000U, 0x2000U, 0x40000U, 0x7dff00U, 0x7dffffU, 0x7e0001U, 0x7e1f01U,
                        0x7e1fffU, 0x7e2000U, 0x800000U, 0x907e0000U, 0xffffffffU}) {
        assert(!driver.WritePage(offset, record.size(), record.data()));
        assert(!driver.EraseSector(offset));
    }
    // A valid page address is insufficient for an erase, or a wrong length.
    assert(!driver.EraseSector(0x7e0100));
    for (std::size_t size : {std::size_t{0}, std::size_t{79}, std::size_t{81}, std::size_t{256},
                             std::numeric_limits<std::size_t>::max()})
        assert(!driver.WritePage(0x7e1f00, size, record.data()));
    assert(sdk.calls.size() == accepted); // Rejection must precede SDK access.

    sdk.result = daisy::QSPIHandle::ERR;
    assert(!driver.WritePage(0x7e0000, record.size(), record.data()));
    assert(!driver.EraseSector(0x7e0000));
    assert(!driver.Initialize() && !driver.initialized);
    sdk.result = daisy::QSPIHandle::OK;
    assert(driver.Initialize());
    assert(sdk.configured.device == daisy::QSPIHandle::Config::IS25LP064A);
    assert(sdk.configured.mode == daisy::QSPIHandle::Config::INDIRECT_POLLING);
    const auto& pins = sdk.configured.pin_config;
    assert(pins.io0.port == DSY_GPIOF && pins.io0.pin == 8);
    assert(pins.io1.port == DSY_GPIOF && pins.io1.pin == 9);
    assert(pins.io2.port == DSY_GPIOF && pins.io2.pin == 7);
    assert(pins.io3.port == DSY_GPIOF && pins.io3.pin == 6);
    assert(pins.clk.port == DSY_GPIOF && pins.clk.pin == 10);
    assert(pins.ncs.port == DSY_GPIOG && pins.ncs.pin == 6);

    registers = {QUADSPI_CR_EN, QUADSPI_CCR_FMODE};
    assert(reinterpret_cast<std::uintptr_t>(driver.MappedData()) == 0x90000000);
    const auto inits = sdk.initializations;
    assert(driver.Initialize() && sdk.initializations == inits);
    for (auto state : {QspiRegisters{0, QUADSPI_CCR_FMODE}, QspiRegisters{QUADSPI_CR_EN, 0}}) {
        registers = state;
        assert(driver.MappedData() == nullptr);
        const auto before = sdk.initializations;
        assert(driver.Initialize() && sdk.initializations == before + 1);
    }
    registers = {QUADSPI_CR_EN, QUADSPI_CCR_FMODE};
    sdk.status = daisy::QSPIHandle::BAD;
    const auto before = sdk.initializations;
    assert(driver.Initialize() && sdk.initializations == before + 1);
}
