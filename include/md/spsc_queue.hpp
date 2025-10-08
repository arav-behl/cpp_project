#pragma once
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T, std::size_t N>
class SPSCQueue {
    static_assert((N & (N - 1)) == 0, "N must be power of two for efficient modulo");
    static_assert(N >= 2, "Queue size must be at least 2");
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

private:
    // Cache line alignment to prevent false sharing
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

    // Ring buffer storage
    alignas(64) T buffer_[N];

    static constexpr size_t MASK = N - 1;

public:
    SPSCQueue() = default;

    // Non-copyable, non-movable for thread safety
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    // Producer side - single thread only
    [[nodiscard]] bool push(const T& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        const size_t next_head = head + 1;

        // Check if queue is full (need to leave one slot empty to distinguish full from empty)
        if ((next_head & MASK) == (tail & MASK)) {
            return false; // Queue full
        }

        // Place item in buffer
        buffer_[head & MASK] = item;

        // Publish the new head (release semantics ensure all writes are visible)
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Move version for better performance
    [[nodiscard]] bool push(T&& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        const size_t next_head = head + 1;

        if ((next_head & MASK) == (tail & MASK)) {
            return false;
        }

        buffer_[head & MASK] = std::move(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Consumer side - single thread only
    [[nodiscard]] bool pop(T& item) noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);

        // Check if queue is empty (compare masked indices)
        if ((tail & MASK) == (head & MASK)) {
            return false; // Queue empty
        }

        // Extract item from buffer
        item = std::move(buffer_[tail & MASK]);

        // Publish the new tail
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // Utility functions (approximate, not thread-safe)
    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & MASK;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] constexpr size_t capacity() const noexcept {
        return N - 1; // One slot reserved to distinguish full from empty
    }

    // Get fill percentage (0.0 to 1.0)
    [[nodiscard]] double fill_ratio() const noexcept {
        return static_cast<double>(size()) / capacity();
    }
};