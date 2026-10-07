#pragma once

// Narrow application writer: driver operations retain their real return values.
// The adapter must authenticate staging and expose actual mapped NOR readback.
#include <cstddef>
#include <cstdint>

namespace aurora_selector
{
    constexpr std::uint32_t QspiApplicationOffset = 0x40000U;
    constexpr std::uint32_t DirtImageSize = 95196U;
    constexpr std::uint32_t QspiApplicationEnd = 0x58000U;
    // Exact catalog admission belongs to the caller. The shared writer still
    // limits the destination to the application and reviewed staging maximum.
    struct QspiExtent { std::size_t size; std::uint32_t erase_end; };
    constexpr QspiExtent DirtExtent{DirtImageSize, QspiApplicationEnd};
    constexpr std::uint32_t QspiMaximumEnd = 0x78000U;
    enum class QspiResult : std::uint32_t
    { Rejected, InitializeFailed, EraseFailed, WriteFailed, ReadbackFailed, Ready };

    // Observational counters; these do not replace bytes/driver-result checks.
    struct QspiDiagnostic
    {
        std::uint32_t stage, erase_attempts, pages, verified, result;
    };

    // Fixed extent avoids accepting a caller-selected address or erase length.
    template<class Driver, class Authenticate>
    QspiResult ProgramQspiImage(Driver& driver, const std::uint8_t* staged,
                               std::size_t size, Authenticate authenticate,
                               volatile QspiDiagnostic& diagnostic,
                               QspiExtent extent = DirtExtent)
    {
        diagnostic.stage = diagnostic.erase_attempts = diagnostic.pages = 0;
        diagnostic.verified = diagnostic.result = 0;
        const auto finish = [&diagnostic](QspiResult result) {
            diagnostic.result = static_cast<std::uint32_t>(result);
            return result;
        };
        if(staged == nullptr || size < 8 || size > 0x38000U || size != extent.size
           || extent.erase_end != QspiApplicationOffset + ((size + 4095U) & ~std::size_t(4095U))
           || extent.erase_end > QspiMaximumEnd || !authenticate())
            return finish(QspiResult::Rejected);
        diagnostic.stage = 1;
        if(!driver.Initialize())
            return finish(QspiResult::InitializeFailed);
        diagnostic.stage = 2;
        for(auto offset = QspiApplicationOffset; offset < extent.erase_end; offset += 4096U)
        {
            // An attempted erase is already indeterminate on failure: no UI retry.
            diagnostic.erase_attempts = diagnostic.erase_attempts + 1U;
            if(!driver.EraseSector(offset))
                return finish(QspiResult::EraseFailed);
        }
        diagnostic.stage = 3;
        for(std::size_t offset = 0; offset < size; offset += 256U)
        {
            const auto length = size - offset < 256U ? size - offset : 256U;
            if(!driver.WritePage(QspiApplicationOffset + offset, length, staged + offset))
                return finish(QspiResult::WriteFailed);
            diagnostic.pages = diagnostic.pages + 1U;
        }
        diagnostic.stage = 4;
        const auto* mapped = driver.MappedData();
        if(mapped == nullptr || !authenticate())
            return finish(QspiResult::ReadbackFailed);
        for(std::size_t offset = 0; offset < size; ++offset)
            if(mapped[offset] != staged[offset])
                return finish(QspiResult::ReadbackFailed);
        diagnostic.verified = size;
        diagnostic.stage = 5;
        return finish(QspiResult::Ready);
    }
}
