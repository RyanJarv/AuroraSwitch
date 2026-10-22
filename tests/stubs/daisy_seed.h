#pragma once

// Host recorder for the SDK boundary used by flash_driver.hpp. This does not
// simulate flash commands; production-command checks belong to execution tests.
#include <cstddef>
#include <cstdint>
#include <vector>

enum { DSY_GPIOF, DSY_GPIOG };
struct dsy_pin {
    int port;
    unsigned pin;
    dsy_pin(int port_value = 0, unsigned pin_value = 0) : port(port_value), pin(pin_value) {}
};
struct QspiRegisters {
    std::uint32_t CR = 0;
    std::uint32_t CCR = 0;
};
inline QspiRegisters registers;
inline auto* QUADSPI = &registers;
constexpr std::uint32_t QUADSPI_CR_EN = 1;
constexpr std::uint32_t QUADSPI_CCR_FMODE = 3U << 26;
inline void __DSB() {}

namespace daisy {
struct QSPIHandle {
    enum Result { OK, ERR };
    enum Status { GOOD, BAD };
    struct Config {
        enum Device { IS25LP064A } device;
        enum Mode { INDIRECT_POLLING, MEMORY_MAPPED } mode;
        struct {
            dsy_pin io0, io1, io2, io3, clk, ncs;
        } pin_config;
    };
    struct Call {
        bool erase;
        std::uint32_t offset;
        std::vector<std::uint8_t> bytes;
    };
    Config configured{};
    Result result = OK;
    Status status = GOOD;
    unsigned initializations = 0;
    std::vector<Call> calls;

    Result Init(const Config& config) {
        ++initializations;
        configured = config;
        return result;
    }
    Status GetStatus() { return status; }
    Result EraseSector(std::uint32_t offset) {
        calls.push_back({true, offset, {}});
        return result;
    }
    Result WritePage(std::uint32_t offset, std::size_t size, std::uint8_t* bytes) {
        calls.push_back({false, offset, {bytes, bytes + size}});
        return result;
    }
};
} // namespace daisy
