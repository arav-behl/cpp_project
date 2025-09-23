#pragma once
#include "stats/rolling_stats.hpp"
#include "stats/rolling_covar.hpp"
#include "md/tick.hpp"
#include <cmath>
#include <algorithm>
#include <functional>

// Base interface for signal rules
class SignalRule {
public:
    virtual ~SignalRule() = default;
    virtual bool evaluate(double& signal_strength) const = 0;
    virtual void reset() = 0;
    virtual const char* name() const noexcept = 0;
};

// Z-Score breakout rule - classic momentum strategy
class ZScoreRule : public SignalRule {
private:
    RollingStats stats_;
    double threshold_;
    double last_value_{0.0};
    bool has_value_{false};

public:
    explicit ZScoreRule(double threshold = 2.0) noexcept
        : threshold_(threshold) {}

    void add_observation(double value) noexcept {
        stats_.add(value);
        last_value_ = value;
        has_value_ = true;
    }

    bool evaluate(double& signal_strength) const override {
        if (!has_value_ || stats_.count() < 10) {
            signal_strength = 0.0;
            return false;
        }

        signal_strength = stats_.z_score(last_value_);
        return std::abs(signal_strength) >= threshold_;
    }

    void reset() override {
        stats_.reset();
        last_value_ = 0.0;
        has_value_ = false;
    }

    const char* name() const noexcept override { return "ZScore"; }

    [[nodiscard]] double threshold() const noexcept { return threshold_; }
    void set_threshold(double thresh) noexcept { threshold_ = thresh; }
};

// Correlation breakdown rule - pairs trading signal
class CorrelationBreakRule : public SignalRule {
private:
    RollingCovar covar_;
    double correlation_threshold_;
    double min_observations_;
    bool below_threshold_{false};

public:
    explicit CorrelationBreakRule(double corr_threshold = 0.3,
                                 double min_obs = 50) noexcept
        : correlation_threshold_(corr_threshold)
        , min_observations_(min_obs) {}

    void add_pair(double x, double y) noexcept {
        covar_.add(x, y);
    }

    bool evaluate(double& signal_strength) const override {
        if (covar_.count() < min_observations_) {
            signal_strength = 0.0;
            return false;
        }

        const double corr = covar_.correlation();
        signal_strength = corr;

        // Signal when correlation drops below threshold
        return std::abs(corr) < correlation_threshold_;
    }

    void reset() override {
        covar_.reset();
        below_threshold_ = false;
    }

    const char* name() const noexcept override { return "CorrBreak"; }

    [[nodiscard]] double correlation() const noexcept {
        return covar_.correlation();
    }

    [[nodiscard]] double beta() const noexcept {
        return covar_.beta();
    }
};

// Mean reversion rule - classic reversion strategy
class MeanReversionRule : public SignalRule {
private:
    EMAStats fast_ema_;
    EMAStats slow_ema_;
    double threshold_;
    double last_value_{0.0};
    bool has_value_{false};

public:
    explicit MeanReversionRule(std::size_t fast_window = 10,
                              std::size_t slow_window = 50,
                              double threshold = 2.0) noexcept
        : fast_ema_(fast_window)
        , slow_ema_(slow_window)
        , threshold_(threshold) {}

    void add_observation(double value) noexcept {
        fast_ema_.add(value);
        slow_ema_.add(value);
        last_value_ = value;
        has_value_ = true;
    }

    bool evaluate(double& signal_strength) const override {
        if (!has_value_ || !fast_ema_.is_initialized() || !slow_ema_.is_initialized()) {
            signal_strength = 0.0;
            return false;
        }

        const double fast_mean = fast_ema_.mean();
        const double slow_mean = slow_ema_.mean();
        const double fast_std = fast_ema_.std_dev();

        if (fast_std <= 0.0) {
            signal_strength = 0.0;
            return false;
        }

        // Signal strength is how far fast EMA is from slow EMA in standard deviations
        signal_strength = (fast_mean - slow_mean) / fast_std;
        return std::abs(signal_strength) >= threshold_;
    }

    void reset() override {
        fast_ema_.reset();
        slow_ema_.reset();
        last_value_ = 0.0;
        has_value_ = false;
    }

    const char* name() const noexcept override { return "MeanRev"; }
};

// Volume spike detection
class VolumeRule : public SignalRule {
private:
    RollingStats volume_stats_;
    double threshold_;
    double last_volume_{0.0};
    bool has_volume_{false};

public:
    explicit VolumeRule(double threshold = 3.0) noexcept
        : threshold_(threshold) {}

    void add_volume(double volume) noexcept {
        volume_stats_.add(volume);
        last_volume_ = volume;
        has_volume_ = true;
    }

    bool evaluate(double& signal_strength) const override {
        if (!has_volume_ || volume_stats_.count() < 20) {
            signal_strength = 0.0;
            return false;
        }

        signal_strength = volume_stats_.z_score(last_volume_);
        return signal_strength >= threshold_; // Only positive spikes
    }

    void reset() override {
        volume_stats_.reset();
        last_volume_ = 0.0;
        has_volume_ = false;
    }

    const char* name() const noexcept override { return "Volume"; }
};

// Composite rule engine - combines multiple signals
class CompositeSignalEngine {
private:
    std::vector<std::unique_ptr<SignalRule>> rules_;
    std::vector<double> weights_;
    double composite_threshold_{1.0};

public:
    CompositeSignalEngine() = default;

    void add_rule(std::unique_ptr<SignalRule> rule, double weight = 1.0) {
        rules_.emplace_back(std::move(rule));
        weights_.push_back(weight);
    }

    bool evaluate(double& composite_strength) const {
        if (rules_.empty()) {
            composite_strength = 0.0;
            return false;
        }

        composite_strength = 0.0;
        double total_weight = 0.0;
        std::size_t active_rules = 0;

        for (std::size_t i = 0; i < rules_.size(); ++i) {
            double strength;
            if (rules_[i]->evaluate(strength)) {
                composite_strength += strength * weights_[i];
                total_weight += weights_[i];
                active_rules++;
            }
        }

        if (active_rules == 0) {
            composite_strength = 0.0;
            return false;
        }

        // Normalize by active weight
        composite_strength /= total_weight;
        return std::abs(composite_strength) >= composite_threshold_;
    }

    void reset_all() {
        for (auto& rule : rules_) {
            rule->reset();
        }
    }

    void set_composite_threshold(double threshold) noexcept {
        composite_threshold_ = threshold;
    }

    [[nodiscard]] std::size_t rule_count() const noexcept {
        return rules_.size();
    }
};