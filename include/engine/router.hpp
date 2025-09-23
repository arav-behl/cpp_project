#pragma once
#include "md/tick.hpp"
#include "engine/signal_rules.hpp"
#include "util/latency.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <string>

// Callback for signal events
using SignalCallback = std::function<void(const SignalEvent&)>;

// Main routing and signal detection engine
class Router {
private:
    // Per-symbol signal rules
    std::unordered_map<std::string, std::unique_ptr<ZScoreRule>> zscore_rules_;
    std::unordered_map<std::string, std::unique_ptr<VolumeRule>> volume_rules_;

    // Cross-symbol rules (pairs trading)
    std::unordered_map<std::string, std::unique_ptr<CorrelationBreakRule>> correlation_rules_;
    std::unordered_map<std::string, std::unique_ptr<MeanReversionRule>> mean_reversion_rules_;

    // Symbol pair mappings for correlation analysis
    std::vector<std::pair<std::string, std::string>> watched_pairs_;

    // Latest tick data for each symbol
    std::unordered_map<std::string, Tick> latest_ticks_;

    // Signal generation
    SignalCallback signal_callback_;
    std::atomic<uint64_t> signal_counter_{0};

    // Performance tracking
    LatencyHistogram latency_hist_;
    std::atomic<uint64_t> ticks_processed_{0};

    // Configuration
    double zscore_threshold_{2.5};
    double correlation_threshold_{0.3};
    double volume_threshold_{3.0};

public:
    Router() = default;

    // Configuration
    void set_zscore_threshold(double threshold) noexcept {
        zscore_threshold_ = threshold;
    }

    void set_correlation_threshold(double threshold) noexcept {
        correlation_threshold_ = threshold;
    }

    void set_volume_threshold(double threshold) noexcept {
        volume_threshold_ = threshold;
    }

    void set_signal_callback(SignalCallback callback) {
        signal_callback_ = std::move(callback);
    }

    void add_watched_pair(const std::string& symbol1, const std::string& symbol2) {
        const std::string pair_key = make_pair_key(symbol1, symbol2);
        watched_pairs_.emplace_back(symbol1, symbol2);

        // Initialize correlation rule for this pair
        correlation_rules_[pair_key] = std::make_unique<CorrelationBreakRule>(
            correlation_threshold_, 50);
    }

    // Main tick processing function
    void process_tick(const Tick& tick) {
        const auto start_time = std::chrono::steady_clock::now();

        // Update latest tick data
        const std::string symbol{tick.symbol};
        latest_ticks_[symbol] = tick;

        // Ensure rules exist for this symbol
        ensure_rules_exist(symbol);

        // Process single-symbol signals
        process_single_symbol_signals(tick);

        // Process cross-symbol signals
        process_cross_symbol_signals(tick);

        // Update latency statistics
        latency_hist_.add_sample(tick.timestamp, start_time);
        ticks_processed_.fetch_add(1, std::memory_order_relaxed);
    }

    // Statistics and monitoring
    [[nodiscard]] uint64_t ticks_processed() const noexcept {
        return ticks_processed_.load(std::memory_order_acquire);
    }

    [[nodiscard]] uint64_t signals_generated() const noexcept {
        return signal_counter_.load(std::memory_order_acquire);
    }

    [[nodiscard]] const LatencyHistogram& latency_histogram() const noexcept {
        return latency_hist_;
    }

    [[nodiscard]] double processing_rate() const noexcept {
        // Returns ticks per second processed
        return latency_hist_.sample_rate_per_second();
    }

    // Reset all statistics
    void reset_stats() {
        ticks_processed_.store(0, std::memory_order_release);
        signal_counter_.store(0, std::memory_order_release);
        latency_hist_.reset();

        // Reset all rules
        for (auto& [symbol, rule] : zscore_rules_) {
            rule->reset();
        }
        for (auto& [symbol, rule] : volume_rules_) {
            rule->reset();
        }
        for (auto& [pair, rule] : correlation_rules_) {
            rule->reset();
        }
        for (auto& [symbol, rule] : mean_reversion_rules_) {
            rule->reset();
        }
    }

    // Get current correlation for a pair
    [[nodiscard]] double get_correlation(const std::string& symbol1,
                                        const std::string& symbol2) const {
        const std::string pair_key = make_pair_key(symbol1, symbol2);
        auto it = correlation_rules_.find(pair_key);
        return (it != correlation_rules_.end()) ? it->second->correlation() : 0.0;
    }

private:
    void ensure_rules_exist(const std::string& symbol) {
        if (zscore_rules_.find(symbol) == zscore_rules_.end()) {
            zscore_rules_[symbol] = std::make_unique<ZScoreRule>(zscore_threshold_);
        }
        if (volume_rules_.find(symbol) == volume_rules_.end()) {
            volume_rules_[symbol] = std::make_unique<VolumeRule>(volume_threshold_);
        }
        if (mean_reversion_rules_.find(symbol) == mean_reversion_rules_.end()) {
            mean_reversion_rules_[symbol] = std::make_unique<MeanReversionRule>();
        }
    }

    void process_single_symbol_signals(const Tick& tick) {
        const std::string symbol{tick.symbol};

        // Z-Score analysis on last price
        auto& zscore_rule = zscore_rules_[symbol];
        zscore_rule->add_observation(tick.last_price);

        double zscore_strength;
        if (zscore_rule->evaluate(zscore_strength)) {
            emit_signal(SignalEvent::Type::Z_SCORE_BREAK, symbol, "",
                       zscore_strength, 0.95);
        }

        // Volume spike analysis
        auto& volume_rule = volume_rules_[symbol];
        volume_rule->add_volume(tick.last_size);

        double volume_strength;
        if (volume_rule->evaluate(volume_strength)) {
            emit_signal(SignalEvent::Type::VOLUME_SPIKE, symbol, "",
                       volume_strength, 0.90);
        }

        // Mean reversion analysis
        auto& mean_rev_rule = mean_reversion_rules_[symbol];
        mean_rev_rule->add_observation(tick.last_price);

        double mean_rev_strength;
        if (mean_rev_rule->evaluate(mean_rev_strength)) {
            emit_signal(SignalEvent::Type::PAIR_TRADE_ENTRY, symbol, "",
                       mean_rev_strength, 0.85);
        }
    }

    void process_cross_symbol_signals(const Tick& tick) {
        const std::string current_symbol{tick.symbol};

        // Check all pairs involving this symbol
        for (const auto& [symbol1, symbol2] : watched_pairs_) {
            if (symbol1 != current_symbol && symbol2 != current_symbol) {
                continue;
            }

            // Check if we have recent data for both symbols
            auto it1 = latest_ticks_.find(symbol1);
            auto it2 = latest_ticks_.find(symbol2);

            if (it1 == latest_ticks_.end() || it2 == latest_ticks_.end()) {
                continue;
            }

            const std::string pair_key = make_pair_key(symbol1, symbol2);
            auto& corr_rule = correlation_rules_[pair_key];

            // Add the pair observation
            corr_rule->add_pair(it1->second.last_price, it2->second.last_price);

            // Check for correlation breakdown
            double corr_strength;
            if (corr_rule->evaluate(corr_strength)) {
                emit_signal(SignalEvent::Type::CORRELATION_BREAK,
                           symbol1, symbol2, corr_strength, 0.88);
            }
        }
    }

    void emit_signal(SignalEvent::Type type, const std::string& primary,
                    const std::string& secondary, double strength, double confidence) {
        if (!signal_callback_) return;

        SignalEvent event{
            type,
            SymbolTable::intern(primary),
            secondary.empty() ? std::string_view{} : SymbolTable::intern(secondary),
            strength,
            confidence
        };

        event.signal_id = signal_counter_.fetch_add(1, std::memory_order_acq_rel);
        event.generation_time = std::chrono::steady_clock::now();

        signal_callback_(event);
    }

    [[nodiscard]] static std::string make_pair_key(const std::string& s1,
                                                  const std::string& s2) {
        return s1 < s2 ? (s1 + "|" + s2) : (s2 + "|" + s1);
    }
};