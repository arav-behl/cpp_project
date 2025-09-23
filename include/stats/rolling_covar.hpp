#pragma once
#include "stats/rolling_stats.hpp"
#include <cmath>
#include <algorithm>

// Online covariance computation using parallel to Welford's algorithm
class RollingCovar {
private:
    double mean_x_{0.0};
    double mean_y_{0.0};
    double c_{0.0};        // Comoment (sum of cross-deviations)
    double m2_x_{0.0};     // Sum of squared deviations for X
    double m2_y_{0.0};     // Sum of squared deviations for Y
    std::size_t count_{0};

public:
    RollingCovar() = default;

    // Add a pair of observations
    void add(double x, double y) noexcept {
        count_++;
        const double n = static_cast<double>(count_);

        // Update means and compute deltas
        const double dx = x - mean_x_;
        const double dy = y - mean_y_;

        mean_x_ += dx / n;
        mean_y_ += dy / n;

        // Update comoment and sum of squared deviations
        c_ += dx * (y - mean_y_);
        m2_x_ += dx * (x - mean_x_);
        m2_y_ += dy * (y - mean_y_);
    }

    void reset() noexcept {
        mean_x_ = mean_y_ = 0.0;
        c_ = m2_x_ = m2_y_ = 0.0;
        count_ = 0;
    }

    // Accessors
    [[nodiscard]] double mean_x() const noexcept { return mean_x_; }
    [[nodiscard]] double mean_y() const noexcept { return mean_y_; }
    [[nodiscard]] std::size_t count() const noexcept { return count_; }

    // Sample covariance
    [[nodiscard]] double covariance() const noexcept {
        return count_ > 1 ? c_ / static_cast<double>(count_ - 1) : 0.0;
    }

    // Population covariance
    [[nodiscard]] double population_covariance() const noexcept {
        return count_ > 0 ? c_ / static_cast<double>(count_) : 0.0;
    }

    // Individual variances
    [[nodiscard]] double variance_x() const noexcept {
        return count_ > 1 ? m2_x_ / static_cast<double>(count_ - 1) : 0.0;
    }

    [[nodiscard]] double variance_y() const noexcept {
        return count_ > 1 ? m2_y_ / static_cast<double>(count_ - 1) : 0.0;
    }

    // Standard deviations
    [[nodiscard]] double std_dev_x() const noexcept {
        return std::sqrt(variance_x());
    }

    [[nodiscard]] double std_dev_y() const noexcept {
        return std::sqrt(variance_y());
    }

    // Correlation coefficient
    [[nodiscard]] double correlation() const noexcept {
        const double var_x = variance_x();
        const double var_y = variance_y();

        if (var_x <= 0.0 || var_y <= 0.0) return 0.0;

        return covariance() / std::sqrt(var_x * var_y);
    }

    // Beta coefficient (slope of regression y on x)
    [[nodiscard]] double beta() const noexcept {
        const double var_x = variance_x();
        return var_x > 0.0 ? covariance() / var_x : 0.0;
    }

    // R-squared (coefficient of determination)
    [[nodiscard]] double r_squared() const noexcept {
        const double corr = correlation();
        return corr * corr;
    }

    // Check if statistics are valid
    [[nodiscard]] bool is_valid() const noexcept {
        return count_ > 0 &&
               std::isfinite(mean_x_) && std::isfinite(mean_y_) &&
               std::isfinite(c_) && std::isfinite(m2_x_) && std::isfinite(m2_y_);
    }
};

// EMA-based covariance for faster decay
class EMACovar {
private:
    double alpha_;
    double mean_x_{0.0};
    double mean_y_{0.0};
    double cov_{0.0};
    double var_x_{0.0};
    double var_y_{0.0};
    bool initialized_{false};

public:
    explicit EMACovar(double alpha) noexcept : alpha_(alpha) {
        assert(alpha > 0.0 && alpha <= 1.0);
    }

    explicit EMACovar(std::size_t window) noexcept
        : alpha_(2.0 / (static_cast<double>(window) + 1.0)) {}

    void add(double x, double y) noexcept {
        if (!initialized_) {
            mean_x_ = x;
            mean_y_ = y;
            cov_ = var_x_ = var_y_ = 0.0;
            initialized_ = true;
        } else {
            const double dx = x - mean_x_;
            const double dy = y - mean_y_;

            mean_x_ += alpha_ * dx;
            mean_y_ += alpha_ * dy;

            cov_ = (1.0 - alpha_) * cov_ + alpha_ * dx * dy;
            var_x_ = (1.0 - alpha_) * var_x_ + alpha_ * dx * dx;
            var_y_ = (1.0 - alpha_) * var_y_ + alpha_ * dy * dy;
        }
    }

    void reset() noexcept {
        mean_x_ = mean_y_ = 0.0;
        cov_ = var_x_ = var_y_ = 0.0;
        initialized_ = false;
    }

    [[nodiscard]] double mean_x() const noexcept { return mean_x_; }
    [[nodiscard]] double mean_y() const noexcept { return mean_y_; }
    [[nodiscard]] double covariance() const noexcept { return cov_; }
    [[nodiscard]] double variance_x() const noexcept { return var_x_; }
    [[nodiscard]] double variance_y() const noexcept { return var_y_; }

    [[nodiscard]] double correlation() const noexcept {
        if (var_x_ <= 0.0 || var_y_ <= 0.0) return 0.0;
        return cov_ / std::sqrt(var_x_ * var_y_);
    }

    [[nodiscard]] double beta() const noexcept {
        return var_x_ > 0.0 ? cov_ / var_x_ : 0.0;
    }

    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
};