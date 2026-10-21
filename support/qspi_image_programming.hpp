#pragma once
#include "selector_constants.h"

// Write approved application bytes to external flash, preserving other regions.
// The caller verifies the staged image; this helper checks driver results and
// reads the flash back. Identical installed bytes skip erasing and programming.
#include <cstddef>
#include <cstdint>

namespace aurora_selector {
constexpr std::uint32_t QspiApplicationOffset = SELECTOR_QSPI_APPLICATION_OFFSET;

// Exact catalog admission belongs to the caller. The shared writer still
// limits the destination to the application and reviewed staging maximum.
struct QspiExtent {
    std::size_t size;
    std::uint32_t erase_end;
};

constexpr std::uint32_t QspiMaximumEnd = SELECTOR_QSPI_MAXIMUM_END;
enum class QspiResult : std::uint32_t {
    Rejected,
    InitializeFailed,
    EraseFailed,
    WriteFailed,
    ReadbackFailed,
    Ready
};

// Debug counters help explain flash activity. They do not replace checking
// the actual bytes or whether each driver operation succeeded.
struct QspiDiagnostic {
    std::uint32_t stage, erase_attempts, pages, verified, result;
};

// The caller supplies the exact image size and sector-rounded end offset.
// Check both against the fixed application region before touching flash.
template <class Driver, class Authenticate>
QspiResult ProgramQspiImage(Driver& driver, const std::uint8_t* staged, std::size_t size,
                            Authenticate authenticate, volatile QspiDiagnostic& diagnostic,
                            QspiExtent extent) {
    diagnostic.stage = diagnostic.erase_attempts = diagnostic.pages = 0;
    diagnostic.verified = diagnostic.result = 0;
    const auto finish = [&diagnostic](QspiResult result) {
        diagnostic.result = static_cast<std::uint32_t>(result);
        return result;
    };
    if (staged == nullptr || size < 8 || size > SELECTOR_QSPI_MAXIMUM_IMAGE_SIZE ||
        size != extent.size ||
        extent.erase_end != QspiApplicationOffset + ((size + 4095U) & ~std::size_t(4095U)) ||
        extent.erase_end > QspiMaximumEnd || !authenticate()) {
        return finish(QspiResult::Rejected);
    }
    diagnostic.stage = 1;
    if (!driver.Initialize()) {
        return finish(QspiResult::InitializeFailed);
    }
    // An identical authenticated application needs neither erase nor program.
    const auto* existing = driver.MappedData();
    if (existing == nullptr) {
        return finish(QspiResult::ReadbackFailed);
    }
    bool identical = true;
    for (std::size_t offset = 0; offset < size; ++offset) {
        if (existing[offset] != staged[offset]) {
            identical = false;
            break;
        }
    }
    if (identical) {
        if (!authenticate()) {
            return finish(QspiResult::ReadbackFailed);
        }
        diagnostic.verified = size;
        diagnostic.stage = 5;
        return finish(QspiResult::Ready);
    }
    diagnostic.stage = 2;
    for (auto offset = QspiApplicationOffset; offset < extent.erase_end; offset += 4096U) {
        // Even a failed erase may have changed flash. Count the attempt so the
        // caller knows that returning to the menu may no longer be safe.
        diagnostic.erase_attempts = diagnostic.erase_attempts + 1U;
        if (!driver.EraseSector(offset)) {
            return finish(QspiResult::EraseFailed);
        }
    }
    diagnostic.stage = 3;
    for (std::size_t offset = 0; offset < size; offset += 256U) {
        const auto length = size - offset < 256U ? size - offset : 256U;
        if (!driver.WritePage(QspiApplicationOffset + offset, length, staged + offset)) {
            return finish(QspiResult::WriteFailed);
        }
        diagnostic.pages = diagnostic.pages + 1U;
    }
    diagnostic.stage = 4;
    const auto* mapped = driver.MappedData();
    if (mapped == nullptr || !authenticate()) {
        return finish(QspiResult::ReadbackFailed);
    }
    for (std::size_t offset = 0; offset < size; ++offset) {
        if (mapped[offset] != staged[offset]) {
            return finish(QspiResult::ReadbackFailed);
        }
    }
    diagnostic.verified = size;
    diagnostic.stage = 5;
    return finish(QspiResult::Ready);
}
} // namespace aurora_selector
