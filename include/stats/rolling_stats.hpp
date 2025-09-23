#pragma once
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <cassert>

// Numerically stable online statistics using Welford's algorithm
class RollingStats {
private:
    double mean_{0.0};
    double m2_{0.0};    // Sum of squared deviations
    std::size_t count_{0};

public:
    RollingStats() = default;

    // Add a new observation using Welford's algorithm
    void add(double value) noexcept {
        count_++;
        const double delta = value - mean_;
        mean_ += delta / static_cast<double>(count_);
        const double delta2 = value - mean_;
        m2_ += delta * delta2;
    }

    // Reset statistics
    void reset() noexcept {
        mean_ = 0.0;
        m2_ = 0.0;
        count_ = 0;
    }

    // Accessors
    [[nodiscard]] double mean() const noexcept { return mean_; }
    [[nodiscard]] std::size_t count() const noexcept { return count_; }

    // Sample variance (Bessel's correction: divide by n-1)
    [[nodiscard]] double variance() const noexcept {
        return count_ > 1 ? m2_ / static_cast<double>(count_ - 1) : 0.0;
    }

    // Population variance (divide by n)
    [[nodiscard]] double population_variance() const noexcept {
        return count_ > 0 ? m2_ / static_cast<double>(count_) : 0.0;
    }

    // Standard deviation
    [[nodiscard]] double std_dev() const noexcept {
        return std::sqrt(variance());
    }

    [[nodiscard]] double population_std_dev() const noexcept {
        return std::sqrt(population_variance());
    }

    // Coefficient of variation (relative volatility)
    [[nodiscard]] double cv() const noexcept {
        return (mean_ != 0.0) ? std_dev() / std::abs(mean_) : 0.0;
    }

    // Z-score for a new value
    [[nodiscard]] double z_score(double value) const noexcept {
        const double sd = std_dev();
        return (sd > 0.0) ? (value - mean_) / sd : 0.0;
    }

    // Check if statistics are valid
    [[nodiscard]] bool is_valid() const noexcept {
        return count_ > 0 && std::isfinite(mean_) && std::isfinite(m2_);
    }
};

// Exponential Moving Average version for fixed-decay rolling statistics
class EMAStats {
private:
    double alpha_;      // Smoothing factor
    double mean_{0.0};
    double var_{0.0};   // EMA of variance
    bool initialized_{false};

public:
    // alpha = 2/(window + 1), e.g., window=20 -> alpha=0.095
    explicit EMAStats(double alpha) noexcept : alpha_(alpha) {
        assert(alpha > 0.0 && alpha <= 1.0);
    }

    explicit EMAStats(std::size_t window) noexcept
        : alpha_(2.0 / (static_cast<double>(window) + 1.0)) {}

    void add(double value) noexcept {
        if (!initialized_) {
            mean_ = value;
            var_ = 0.0;
            initialized_ = true;
        } else {
            const double delta = value - mean_;
            mean_ += alpha_ * delta;
            var_ = (1.0 - alpha_) * (var_ + alpha_ * delta * delta);
        }
    }

    void reset() noexcept {
        mean_ = 0.0;
        var_ = 0.0;
        initialized_ = false;
    }

    [[nodiscard]] double mean() const noexcept { return mean_; }
    [[nodiscard]] double variance() const noexcept { return var_; }
    [[nodiscard]] double std_dev() const noexcept { return std::sqrt(var_); }

    [[nodiscard]] double z_score(double value) const noexcept {
        const double sd = std_dev();
        return (sd > 0.0) ? (value - mean_) / sd : 0.0;
    }

    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
};

// Fixed-window rolling statistics with circular buffer
template<std::size_t WindowSize>
class WindowedStats {
private:
    static_assert(WindowSize > 0, "Window size must be positive");

    double buffer_[WindowSize];
    std::size_t index_{0};
    std::size_t count_{0};
    double sum_{0.0};
    double sum_sq_{0.0};

    void update_sums() noexcept {
        sum_ = 0.0;
        sum_sq_ = 0.0;
        const std::size_t n = std::min(count_, WindowSize);
        for (std::size_t i = 0; i < n; ++i) {
            const double val = buffer_[i];
            sum_ += val;
            sum_sq_ += val * val;
        }
    }

public:
    WindowedStats() { std::fill(buffer_, buffer_ + WindowSize, 0.0); }

    void add(double value) noexcept {
        if (count_ >= WindowSize) {
            // Remove old value from sums
            const double old_val = buffer_[index_];
            sum_ -= old_val;
            sum_sq_ -= old_val * old_val;
        }

        // Add new value
        buffer_[index_] = value;
        sum_ += value;
        sum_sq_ += value * value;

        index_ = (index_ + 1) % WindowSize;
        if (count_ < WindowSize) count_++;
    }

    void reset() noexcept {
        std::fill(buffer_, buffer_ + WindowSize, 0.0);
        index_ = 0;
        count_ = 0;
        sum_ = 0.0;
        sum_sq_ = 0.0;
    }

    [[nodiscard]] double mean() const noexcept {
        return count_ > 0 ? sum_ / static_cast<double>(count_) : 0.0;
    }

    [[nodiscard]] double variance() const noexcept {
        if (count_ <= 1) return 0.0;
        const double n = static_cast<double>(count_);
        const double mean_val = mean();
        return (sum_sq_ - n * mean_val * mean_val) / (n - 1.0);
    }

    [[nodiscard]] double std_dev() const noexcept {
        return std::sqrt(variance());
    }

    [[nodiscard]] double z_score(double value) const noexcept {
        const double sd = std_dev();
        return (sd > 0.0) ? (value - mean()) / sd : 0.0;
    }

    [[nodiscard]] std::size_t count() const noexcept { return count_; }
    [[nodiscard]] bool is_full() const noexcept { return count_ >= WindowSize; }
};