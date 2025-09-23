#include "md/spsc_queue.hpp"
#include "md/tick.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <cassert>

// Simple test structure
struct TestItem {
    int value;
    TestItem(int v = 0) : value(v) {}
    TestItem(const TestItem&) = default;
    TestItem(TestItem&&) = default;
    TestItem& operator=(const TestItem&) = default;
    TestItem& operator=(TestItem&&) = default;
};

void test_spsc_basic() {
    std::cout << "Testing SPSC queue basic operations...\n";

    SPSCQueue<TestItem, 8> queue;

    // Test empty queue
    assert(queue.empty());
    assert(queue.size() == 0);
    assert(queue.capacity() == 7); // N-1 for ring buffer

    TestItem item;
    assert(!queue.pop(item)); // Empty queue

    // Test single push/pop
    assert(queue.push(TestItem{42}));
    assert(!queue.empty());
    assert(queue.size() == 1);

    assert(queue.pop(item));
    assert(item.value == 42);
    assert(queue.empty());

    // Test filling to capacity
    for (int i = 0; i < 7; ++i) {
        assert(queue.push(TestItem{i}));
    }
    assert(!queue.push(TestItem{999})); // Should fail (full)

    // Test draining
    for (int i = 0; i < 7; ++i) {
        assert(queue.pop(item));
        assert(item.value == i);
    }
    assert(queue.empty());

    std::cout << "✅ SPSC basic tests passed\n";
}

void test_spsc_move_semantics() {
    std::cout << "Testing SPSC queue move semantics...\n";

    SPSCQueue<std::unique_ptr<int>, 4> queue;

    // Test move-only types
    auto ptr = std::make_unique<int>(123);
    assert(queue.push(std::move(ptr)));
    assert(ptr == nullptr); // Should be moved

    std::unique_ptr<int> result;
    assert(queue.pop(result));
    assert(result != nullptr);
    assert(*result == 123);

    std::cout << "✅ SPSC move semantics tests passed\n";
}

void test_spsc_concurrency() {
    std::cout << "Testing SPSC queue concurrency...\n";

    constexpr size_t QUEUE_SIZE = 1024;
    constexpr size_t NUM_ITEMS = 100000;

    SPSCQueue<int, QUEUE_SIZE> queue;
    std::atomic<size_t> items_produced{0};
    std::atomic<size_t> items_consumed{0};
    std::atomic<bool> producer_done{false};

    // Producer thread
    std::thread producer([&queue, &items_produced, &producer_done]() {
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            while (!queue.push(static_cast<int>(i))) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            items_produced.fetch_add(1, std::memory_order_relaxed);
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer thread
    std::thread consumer([&queue, &items_consumed, &producer_done]() {
        int item;
        size_t expected = 0;

        while (true) {
            if (queue.pop(item)) {
                assert(item == static_cast<int>(expected));
                expected++;
                items_consumed.fetch_add(1, std::memory_order_relaxed);
            } else if (producer_done.load(std::memory_order_acquire)) {
                // Producer is done, drain remaining items
                while (queue.pop(item)) {
                    assert(item == static_cast<int>(expected));
                    expected++;
                    items_consumed.fetch_add(1, std::memory_order_relaxed);
                }
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });

    // Wait for completion
    producer.join();
    consumer.join();

    assert(items_produced.load() == NUM_ITEMS);
    assert(items_consumed.load() == NUM_ITEMS);
    assert(queue.empty());

    std::cout << "✅ SPSC concurrency tests passed\n";
}

void test_spsc_with_ticks() {
    std::cout << "Testing SPSC queue with Tick objects...\n";

    SPSCQueue<Tick, 16> queue;

    // Create test ticks
    std::vector<Tick> test_ticks;
    for (int i = 0; i < 10; ++i) {
        test_ticks.emplace_back(
            SymbolTable::intern("TEST"),
            100.0 + i,
            99.0 + i,
            101.0 + i,
            1000.0,
            i
        );
    }

    // Push all ticks
    for (const auto& tick : test_ticks) {
        assert(queue.push(tick));
    }

    // Pop and verify
    Tick received_tick;
    for (size_t i = 0; i < test_ticks.size(); ++i) {
        assert(queue.pop(received_tick));
        assert(received_tick.symbol == test_ticks[i].symbol);
        assert(received_tick.last_price == test_ticks[i].last_price);
        assert(received_tick.sequence_id == test_ticks[i].sequence_id);
    }

    assert(queue.empty());

    std::cout << "✅ SPSC Tick tests passed\n";
}

void test_spsc_performance() {
    std::cout << "Testing SPSC queue performance...\n";

    constexpr size_t QUEUE_SIZE = 65536;
    constexpr size_t NUM_OPERATIONS = 50000; // Fit in queue capacity

    SPSCQueue<int, QUEUE_SIZE> queue;

    const auto start_time = std::chrono::high_resolution_clock::now();

    // Single-threaded performance test
    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        [[maybe_unused]] bool result = queue.push(static_cast<int>(i));
        assert(result);
    }

    int item;
    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        assert(queue.pop(item));
        assert(item == static_cast<int>(i));
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count();

    const double ops_per_second = (2.0 * NUM_OPERATIONS * 1000000.0) / duration;

    std::cout << "  Operations: " << (2 * NUM_OPERATIONS) << "\n";
    std::cout << "  Time: " << duration << " μs\n";
    std::cout << "  Rate: " << std::fixed << std::setprecision(0) << ops_per_second << " ops/sec\n";

    assert(ops_per_second > 1000000); // At least 1M ops/sec

    std::cout << "✅ SPSC performance tests passed\n";
}

void run_queue_tests() {
    std::cout << "🧪 Running Queue Tests\n";
    std::cout << "======================\n";

    test_spsc_basic();
    test_spsc_move_semantics();
    test_spsc_concurrency();
    test_spsc_with_ticks();
    test_spsc_performance();

    std::cout << "\n✅ All queue tests passed!\n\n";
}

// Main test runner
int main() {
    std::cout << "🚀 Running C++ Real-Time Trading System Tests\n";
    std::cout << "==============================================\n\n";

    // Run statistics tests
    void run_stats_tests();
    run_stats_tests();

    // Run queue tests
    run_queue_tests();

    std::cout << "🎉 All tests completed successfully!\n";
    std::cout << "Your C++ skills are looking solid! 💪\n";

    return 0;
}