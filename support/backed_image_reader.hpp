#pragma once

// Synthetic transport for host/virtual tests; never substitutes for real USB.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace daisy_development
{
    // Synthetic, read-only transport. Payload memory is supplied separately by
    // the caller, never by an untrusted guest pointer in this descriptor.
    // Publish ready last; bump generation whenever the backing bytes change.
    struct BackedImageDescriptor
    {
        std::uint32_t version, ready, generation, size;
        char path[64];
        std::uint32_t disconnect_after;
    };

    // Freeze size/generation at Open so replacement cannot silently splice a read.
    class BackedImageReader
    {
      public:
        BackedImageReader(volatile BackedImageDescriptor& descriptor,
                          const std::uint8_t* bytes, std::size_t capacity)
            : descriptor_(descriptor), bytes_(bytes), capacity_(capacity) {}

        bool Ready() const
        {
            return descriptor_.version == 1 && descriptor_.ready == 1
                && bytes_ != nullptr && descriptor_.size <= capacity_
                && (!opened_ || descriptor_.generation == generation_);
        }
        bool Open(const char* path)
        {
            if(opened_ || !Ready() || path == nullptr)
                return false;
            bool matched = false;
            for(std::size_t i = 0; i < sizeof(descriptor_.path); ++i)
            {
                if(path[i] != descriptor_.path[i])
                    return false;
                if(path[i] == '\0') { matched = true; break; }
            }
            if(!matched)
                return false;
            generation_ = descriptor_.generation;
            size_ = descriptor_.size;
            offset_ = 0;
            opened_ = true;
            return Ready();
        }
        std::size_t Size() const { return size_; }
        std::size_t BytesRead() const { return offset_; }
        bool Read(std::uint8_t* destination, std::size_t amount, std::size_t& read)
        {
            read = 0;
            if(!opened_ || !Ready() || descriptor_.size != size_
               || destination == nullptr || offset_ > size_ || amount > size_ - offset_)
                return false;
            std::memcpy(destination, bytes_ + offset_, amount);
            offset_ += amount;
            read = amount;
            return true;
        }
        void Pump()
        {
            // Only an active transfer owns disconnect injection. Idle pumping
            // must not clear freshly published media. Snapshot the volatile
            // threshold once: replacement can occur at any guest instruction.
            if(!opened_ || !Ready())
                return;
            const auto threshold = descriptor_.disconnect_after;
            if(threshold != 0 && offset_ >= threshold)
                descriptor_.ready = 0;
        }
        bool Close()
        {
            const bool valid = opened_ && Ready() && descriptor_.size == size_;
            opened_ = false;
            return valid;
        }

      private:
        volatile BackedImageDescriptor& descriptor_;
        const std::uint8_t* bytes_;
        std::size_t capacity_, size_ = 0, offset_ = 0;
        std::uint32_t generation_ = 0;
        bool opened_ = false;
    };
}
