#include "md/spsc_queue.hpp"
#include "md/feed_sim.hpp"
#include "md/tick.hpp"
#include "engine/router.hpp"
#include "util/latency.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <vector>
#include <sstream>
#include <mutex>

// Global shutdown signal
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully...\n";
    g_running.store(false, std::memory_order_release);
}

// Configuration structure
struct DemoConfig {
    std::vector<std::string> symbols{"AAPL", "MSFT", "GOOGL", "TSLA"};
    double tick_rate_ms = 0.5;  // 2000 Hz
    double zscore_threshold = 2.5;
    double correlation_threshold = 0.3;
    double volume_threshold = 3.0;
    std::chrono::seconds duration{30};
    bool enable_csv_output = true;
    bool enable_live_display = true;
};

// Signal event logger
class SignalLogger {
private:
    std::vector<SignalEvent> events_;
    std::mutex events_mutex_;
    std::atomic<uint64_t> signal_count_{0};

public:
    void log_signal(const SignalEvent& event) {
        signal_count_.fetch_add(1, std::memory_order_relaxed);

        if (g_running.load(std::memory_order_acquire)) {
            // Print to console
            std::cout << "\n🚨 SIGNAL " << std::setfill('0') << std::setw(6)
                      << event.signal_id << " | " << event.type_name()
                      << " | " << event.primary_symbol;

            if (!event.secondary_symbol.empty()) {
                std::cout << "/" << event.secondary_symbol;
            }

            std::cout << " | strength=" << std::fixed << std::setprecision(2)
                      << event.signal_strength << " | conf=" << event.confidence;

            const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                event.generation_time - event.event_time).count();
            std::cout << " | lat=" << latency_us << "μs";

            std::cout << std::endl;
        }

        // Store for CSV export
        std::lock_guard<std::mutex> lock(events_mutex_);
        events_.push_back(event);
    }

    void export_csv(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(events_mutex_));
        std::ofstream file(filename);

        file << "timestamp,signal_id,type,primary_symbol,secondary_symbol,"
             << "signal_strength,confidence,latency_us\n";

        for (const auto& event : events_) {
            const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                event.event_time.time_since_epoch()).count();
            const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                event.generation_time - event.event_time).count();

            file << timestamp << "," << event.signal_id << "," << event.type_name()
                 << "," << event.primary_symbol << "," << event.secondary_symbol
                 << "," << event.signal_strength << "," << event.confidence
                 << "," << latency_us << "\n";
        }
    }

    [[nodiscard]] uint64_t signal_count() const {
        return signal_count_.load(std::memory_order_acquire);
    }
};

// Live performance dashboard
class Dashboard {
private:
    std::chrono::steady_clock::time_point start_time_;

public:
    Dashboard() : start_time_(std::chrono::steady_clock::now()) {}

    void print_status(const Router& router, const FeedSimulator& feed_sim,
                     const SignalLogger& signal_logger,
                     const SPSCQueue<Tick, 65536>& queue) const {

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - start_time_).count();

        // Clear screen and move cursor to top
        std::cout << "\033[2J\033[H";

        // Header
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              🚀 REAL-TIME TRADING SYSTEM 🚀                  ║\n";
        std::cout << "║                    C++20 Low-Latency Engine                  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";

        // Runtime info
        std::cout << "║ Runtime: " << std::setw(3) << elapsed << "s"
                  << "                                                   ║\n";

        // Feed simulator stats
        const auto ticks_generated = feed_sim.ticks_generated();
        const auto ticks_dropped = feed_sim.ticks_dropped();
        const auto drop_rate = feed_sim.drop_rate() * 100.0;

        std::cout << "║ Feed: " << std::setw(8) << ticks_generated << " ticks"
                  << " | Dropped: " << std::setw(6) << ticks_dropped
                  << " (" << std::fixed << std::setprecision(2) << drop_rate << "%)"
                  << "        ║\n";

        // Queue status
        const auto queue_fill = queue.fill_ratio() * 100.0;
        std::cout << "║ Queue: " << std::fixed << std::setprecision(1) << queue_fill << "% full"
                  << "                                                ║\n";

        // Router stats
        const auto ticks_processed = router.ticks_processed();
        const auto processing_rate = router.processing_rate();

        std::cout << "║ Processed: " << std::setw(8) << ticks_processed << " ticks"
                  << " | Rate: " << std::fixed << std::setprecision(0)
                  << processing_rate << " TPS"
                  << "              ║\n";

        // Signal stats
        const auto signals_generated = signal_logger.signal_count();
        std::cout << "║ Signals: " << std::setw(6) << signals_generated
                  << "                                                   ║\n";

        // Latency stats
        const auto& latency_hist = router.latency_histogram();
        std::cout << "║ Latency: P50=" << std::fixed << std::setprecision(0)
                  << latency_hist.p50_us() << "μs | P95=" << latency_hist.p95_us()
                  << "μs | P99=" << latency_hist.p99_us() << "μs"
                  << "          ║\n";

        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        std::cout << std::flush;
    }
};

int main(int argc, char* argv[]) {
    // Setup signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Configuration
    DemoConfig config;

    // Parse command line arguments (simplified)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --duration N     Run for N seconds (default: 30)\n"
                      << "  --rate N         Tick rate in Hz (default: 2000)\n"
                      << "  --zscore N       Z-score threshold (default: 2.5)\n"
                      << "  --help           Show this help\n";
            return 0;
        } else if (arg == "--duration" && i + 1 < argc) {
            config.duration = std::chrono::seconds(std::stoi(argv[++i]));
        } else if (arg == "--rate" && i + 1 < argc) {
            config.tick_rate_ms = 1000.0 / std::stod(argv[++i]);
        } else if (arg == "--zscore" && i + 1 < argc) {
            config.zscore_threshold = std::stod(argv[++i]);
        }
    }

    std::cout << "🚀 Starting Real-Time Trading System Demo...\n";
    std::cout << "Press Ctrl+C to stop gracefully\n\n";

    // Initialize components
    SPSCQueue<Tick, 65536> tick_queue;  // 64K tick buffer
    SignalLogger signal_logger;
    Router router;
    Dashboard dashboard;

    // Configure router
    router.set_zscore_threshold(config.zscore_threshold);
    router.set_correlation_threshold(config.correlation_threshold);
    router.set_volume_threshold(config.volume_threshold);
    router.set_signal_callback([&signal_logger](const SignalEvent& event) {
        signal_logger.log_signal(event);
    });

    // Add correlation pairs
    router.add_watched_pair("AAPL", "MSFT");
    router.add_watched_pair("GOOGL", "TSLA");

    // Create symbol configurations
    std::vector<SymbolConfig> symbol_configs;
    for (const auto& symbol : config.symbols) {
        symbol_configs.emplace_back(symbol, 100.0 + std::rand() % 100, 0.02);
    }

    // Initialize feed simulator
    FeedSimulator feed_sim(std::move(symbol_configs),
                          PriceModel::GEOMETRIC_BROWNIAN_MOTION,
                          config.tick_rate_ms);

    // Start feed simulator thread
    std::thread feed_thread([&feed_sim, &tick_queue]() {
        while (g_running.load(std::memory_order_acquire)) {
            feed_sim.generate_ticks(tick_queue);
            std::this_thread::sleep_for(std::chrono::microseconds(
                static_cast<int>(feed_sim.symbols()[0].tick_size * 1000)));
        }
    });

    // Start consumer thread
    std::thread consumer_thread([&router, &tick_queue]() {
        Tick tick;
        while (g_running.load(std::memory_order_acquire)) {
            if (tick_queue.pop(tick)) {
                router.process_tick(tick);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });

    // Live dashboard display
    std::thread dashboard_thread([&]() {
        while (g_running.load(std::memory_order_acquire)) {
            if (config.enable_live_display) {
                dashboard.print_status(router, feed_sim, signal_logger, tick_queue);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    // Run for specified duration or until interrupted
    const auto start_time = std::chrono::steady_clock::now();
    while (g_running.load(std::memory_order_acquire)) {
        const auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed >= config.duration) {
            std::cout << "\n⏰ Time limit reached, shutting down...\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Graceful shutdown
    g_running.store(false, std::memory_order_release);

    // Wait for threads to finish
    if (feed_thread.joinable()) feed_thread.join();
    if (consumer_thread.joinable()) consumer_thread.join();
    if (dashboard_thread.joinable()) dashboard_thread.join();

    // Final statistics
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        FINAL RESULTS                        ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";

    const auto& latency_hist = router.latency_histogram();
    std::cout << "║ Total Ticks Processed: " << std::setw(10) << router.ticks_processed() << "                    ║\n";
    std::cout << "║ Total Signals:         " << std::setw(10) << signal_logger.signal_count() << "                    ║\n";
    std::cout << "║ Average Rate:           " << std::setw(8) << std::fixed << std::setprecision(0)
              << router.processing_rate() << " TPS               ║\n";
    std::cout << "║ Queue Drop Rate:        " << std::setw(8) << std::fixed << std::setprecision(2)
              << feed_sim.drop_rate() * 100.0 << "%                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    // Print latency histogram
    std::cout << "\n";
    latency_hist.print_histogram(std::cout);

    // Export CSV files
    if (config.enable_csv_output) {
        std::cout << "\n📊 Exporting data...\n";
        signal_logger.export_csv("data/signals.csv");

        // Export latency histogram
        std::ofstream latency_file("data/latency_histogram.csv");
        latency_file << "lower_bound_us,upper_bound_us,count,percentage\n";
        for (const auto& bucket : latency_hist.get_histogram()) {
            latency_file << bucket.lower_bound_us << "," << bucket.upper_bound_us
                        << "," << bucket.count << "," << bucket.percentage << "\n";
        }

        std::cout << "✅ Data exported to data/ directory\n";
    }

    std::cout << "\n🎉 Demo completed successfully!\n";
    return 0;
}