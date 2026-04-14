//2. Zero-Copy Ring Buffer for Market Data
// In market data, copying a large OrderBookUpdate or Trade struct twice (once into the queue and once out) adds nanoseconds of latency. A Zero-Copy Ring Buffer solves this by letting the producer write directly into the buffer's memory.
// The Strategy
// Pre-allocate: Allocate a fixed-size array of your data structures at startup.
// Sequence Numbers: Use two atomic indices: head (where the consumer reads) and tail (where the producer writes).
// Claim & Commit: Instead of "pushing," the producer "claims" a slot, writes to it, and then "commits" it by updating the tail.
// Cache Line Padding: Use alignas(64) to ensure the head and tail reside on different cache lines to prevent false sharing.

#include <atomic>
#include <vector>
#include <cassert>
// The Strategy
// Pre-allocate: Allocate a fixed-size array of your data structures at startup.
// Sequence Numbers: Use two atomic indices: head (where the consumer reads) and tail (where the producer writes).
// Claim & Commit: Instead of "pushing," the producer "claims" a slot, writes to it, and then "commits" it by updating the tail.
template <typename T, size_t Size>
class ZeroCopyRingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

private:
    struct Slot {
        T data;
    };

    alignas(64) std::atomic<size_t> write_idx_{0};
    alignas(64) std::atomic<size_t> read_idx_{0};
    std::vector<Slot> buffer_;

    const size_t mask_ = Size - 1;

public:
    ZeroCopyRingBuffer() : buffer_(Size) {} //Pre-allocate

    // --- PRODUCER SIDE ---
    //Claim & Commit: Instead of "pushing," the producer "claims" a slot, writes to it, and then "commits" it by updating the tail.
    // Returns a pointer to the next available slot for writing
    T* reserve_slot() {
        size_t w = write_idx_.load(std::memory_order_relaxed);
        size_t r = read_idx_.load(std::memory_order_acquire);

        if (w - r >= Size) return nullptr; // Buffer full
        return &buffer_[w & mask_].data;
    }
    void commit_slot() {
        // Incrementing the write index makes the data visible to consumer
        write_idx_.fetch_add(1, std::memory_order_release);
    }

    // --- CONSUMER SIDE ---
    T* peek_slot() {
        size_t r = read_idx_.load(std::memory_order_relaxed);
        size_t w = write_idx_.load(std::memory_order_acquire);

        if (r == w) return nullptr; // Buffer empty
        return &buffer_[r & mask_].data;
    }

    void release_slot() {
        read_idx_.fetch_add(1, std::memory_order_release);
    }
};