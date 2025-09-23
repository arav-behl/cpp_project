#pragma once
#include <chrono>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iomanip>

// High-performance latency measurement and histogram
class LatencyHistogram {
private:
    // Histogram buckets (in microseconds)
    static constexpr std::size_t NUM_BUCKETS = 10;
    static constexpr int BUCKET_EDGES[NUM_BUCKETS + 1] = {
        0, 50, 100, 250, 500, 1000, 2000, 5000, 10000, 50000, 1000000
    };

    std::atomic<uint64_t> buckets_[NUM_BUCKETS];
    std::atomic<uint64_t> total_samples_{0};
    std::atomic<uint64_t> total_latency_us_{0};
    std::atomic<uint64_t> min_latency_us_{UINT64_MAX};
    std::atomic<uint64_t> max_latency_us_{0};

    // For rate calculation
    mutable std::chrono::steady_clock::time_point start_time_;
    mutable std::atomic<bool> timing_started_{false};

public:
    LatencyHistogram() {
        reset();
    }

    void add_sample(std::chrono::steady_clock::time_point start,
                   std::chrono::steady_clock::time_point end) {
        const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        add_sample_us(static_cast<uint64_t>(latency_us));
    }

    void add_sample_us(uint64_t latency_us) {
        // Update timing for rate calculation
        if (!timing_started_.exchange(true, std::memory_order_acq_rel)) {
            start_time_ = std::chrono::steady_clock::now();
        }

        // Find appropriate bucket
        std::size_t bucket = 0;
        for (std::size_t i = 0; i < NUM_BUCKETS; ++i) {
            if (latency_us < static_cast<uint64_t>(BUCKET_EDGES[i + 1])) {
                bucket = i;
                break;
            }
        }
        if (bucket >= NUM_BUCKETS) bucket = NUM_BUCKETS - 1;

        // Update atomically
        buckets_[bucket].fetch_add(1, std::memory_order_relaxed);
        total_samples_.fetch_add(1, std::memory_order_relaxed);
        total_latency_us_.fetch_add(latency_us, std::memory_order_relaxed);

        // Update min/max
        uint64_t current_min = min_latency_us_.load(std::memory_order_relaxed);
        while (latency_us < current_min &&
               !min_latency_us_.compare_exchange_weak(current_min, latency_us,
                                                     std::memory_order_relaxed)) {}

        uint64_t current_max = max_latency_us_.load(std::memory_order_relaxed);
        while (latency_us > current_max &&
               !max_latency_us_.compare_exchange_weak(current_max, latency_us,
                                                     std::memory_order_relaxed)) {}
    }

    void reset() {
        for (auto& bucket : buckets_) {
            bucket.store(0, std::memory_order_relaxed);
        }
        total_samples_.store(0, std::memory_order_relaxed);
        total_latency_us_.store(0, std::memory_order_relaxed);
        min_latency_us_.store(UINT64_MAX, std::memory_order_relaxed);
        max_latency_us_.store(0, std::memory_order_relaxed);
        timing_started_.store(false, std::memory_order_relaxed);
    }

    // Statistics
    [[nodiscard]] uint64_t total_samples() const {
        return total_samples_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double mean_latency_us() const {
        const uint64_t total = total_samples();
        return total > 0 ? static_cast<double>(total_latency_us_.load(std::memory_order_acquire)) / total : 0.0;
    }

    [[nodiscard]] uint64_t min_latency_us() const {
        const uint64_t min_val = min_latency_us_.load(std::memory_order_acquire);
        return min_val == UINT64_MAX ? 0 : min_val;
    }

    [[nodiscard]] uint64_t max_latency_us() const {
        return max_latency_us_.load(std::memory_order_acquire);
    }

    // Percentile calculation (approximate)
    [[nodiscard]] double percentile_us(double p) const {
        const uint64_t total = total_samples();
        if (total == 0) return 0.0;

        const uint64_t target_count = static_cast<uint64_t>(total * p / 100.0);
        uint64_t cumulative = 0;

        for (std::size_t i = 0; i < NUM_BUCKETS; ++i) {
            cumulative += buckets_[i].load(std::memory_order_acquire);
            if (cumulative >= target_count) {
                // Linear interpolation within bucket
                const double bucket_start = BUCKET_EDGES[i];
                const double bucket_end = BUCKET_EDGES[i + 1];
                const uint64_t bucket_count = buckets_[i].load(std::memory_order_acquire);

                if (bucket_count == 0) return bucket_start;

                const uint64_t prev_cumulative = cumulative - bucket_count;
                const double position = static_cast<double>(target_count - prev_cumulative) / bucket_count;
                return bucket_start + position * (bucket_end - bucket_start);
            }
        }

        return BUCKET_EDGES[NUM_BUCKETS];
    }

    [[nodiscard]] double p50_us() const { return percentile_us(50.0); }
    [[nodiscard]] double p95_us() const { return percentile_us(95.0); }
    [[nodiscard]] double p99_us() const { return percentile_us(99.0); }

    // Throughput calculation
    [[nodiscard]] double sample_rate_per_second() const {
        if (!timing_started_.load(std::memory_order_acquire)) return 0.0;

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            now - start_time_).count();

        if (elapsed <= 0) return 0.0;

        const uint64_t total = total_samples();
        return static_cast<double>(total) * 1000000.0 / elapsed;
    }

    // Histogram data for visualization
    struct BucketInfo {
        int lower_bound_us;
        int upper_bound_us;
        uint64_t count;
        double percentage;
    };

    [[nodiscard]] std::vector<BucketInfo> get_histogram() const {
        std::vector<BucketInfo> result;
        const uint64_t total = total_samples();

        for (std::size_t i = 0; i < NUM_BUCKETS; ++i) {
            const uint64_t count = buckets_[i].load(std::memory_order_acquire);
            const double percentage = total > 0 ? static_cast<double>(count) * 100.0 / total : 0.0;

            result.push_back({
                BUCKET_EDGES[i],
                BUCKET_EDGES[i + 1],
                count,
                percentage
            });
        }

        return result;
    }

    // Print histogram to stream
    void print_histogram(std::ostream& os) const {
        const auto histogram = get_histogram();
        const uint64_t total = total_samples();

        os << "Latency Histogram (total samples: " << total << ")\n";
        os << "Range (μs)     | Count    | Percentage\n";
        os << "---------------|----------|----------\n";

        for (const auto& bucket : histogram) {
            os << std::setw(6) << bucket.lower_bound_us << "-"
               << std::setw(6) << bucket.upper_bound_us << " | "
               << std::setw(8) << bucket.count << " | "
               << std::setw(6) << std::fixed << std::setprecision(2)
               << bucket.percentage << "%\n";
        }

        os << "\nStatistics:\n";
        os << "  Mean: " << std::fixed << std::setprecision(1) << mean_latency_us() << " μs\n";
        os << "  Min:  " << min_latency_us() << " μs\n";
        os << "  Max:  " << max_latency_us() << " μs\n";
        os << "  P50:  " << std::fixed << std::setprecision(1) << p50_us() << " μs\n";
        os << "  P95:  " << std::fixed << std::setprecision(1) << p95_us() << " μs\n";
        os << "  P99:  " << std::fixed << std::setprecision(1) << p99_us() << " μs\n";
        os << "  Rate: " << std::fixed << std::setprecision(0) << sample_rate_per_second() << " samples/sec\n";
    }
};