#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <array>
#include <optional>

template<typename T, size_t Size>
class RingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");

public:
    RingBuffer() : head_(0), tail_(0) {}

    bool push(const T& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) & (Size - 1);
        
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;  // Buffer full
        }
        
        buffer_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // Buffer empty
        }
        
        T item = buffer_[tail];
        tail_.store((tail + 1) & (Size - 1), std::memory_order_release);
        return item;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & (Size - 1);
    }

private:
    std::array<T, Size> buffer_;
    std::atomic<size_t> head_;  // Write index
    std::atomic<size_t> tail_;  // Read index
    
    // Padding to prevent false sharing
    char padding_[64 - sizeof(std::atomic<size_t>) * 2 % 64];
};

#endif // RING_BUFFER_H