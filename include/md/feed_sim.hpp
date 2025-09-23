#pragma once
#include "md/tick.hpp"
#include "md/spsc_queue.hpp"
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <atomic>
#include <thread>
#include <algorithm>

enum class PriceModel {
    GEOMETRIC_BROWNIAN_MOTION,
    ORNSTEIN_UHLENBECK,
    JUMP_DIFFUSION,
    MICROSTRUCTURE_NOISE
};

// Configuration for a single symbol
struct SymbolConfig {
    std::string symbol;
    double initial_price{100.0};
    double volatility{0.02};        // Annual volatility (e.g., 0.02 = 2%)
    double drift{0.0};              // Annual drift
    double mean_reversion{0.0};     // For OU process
    double jump_intensity{0.0};     // Jumps per year
    double jump_mean{0.0};          // Mean jump size
    double jump_std{0.01};          // Jump volatility
    double bid_ask_spread{0.01};    // Relative spread (e.g., 0.01 = 1%)
    double tick_size{0.01};         // Minimum price increment

    SymbolConfig(std::string sym, double price = 100.0, double vol = 0.02)
        : symbol(std::move(sym)), initial_price(price), volatility(vol) {}
};

// Market microstructure simulator
class FeedSimulator {
private:
    std::vector<SymbolConfig> symbols_;
    std::vector<double> current_prices_;
    std::vector<uint64_t> sequence_ids_;

    // Random number generation
    mutable std::mt19937_64 rng_;
    mutable std::normal_distribution<double> normal_dist_{0.0, 1.0};
    mutable std::exponential_distribution<double> exp_dist_{1.0};
    mutable std::uniform_real_distribution<double> uniform_dist_{0.0, 1.0};

    // Simulation parameters
    PriceModel model_{PriceModel::GEOMETRIC_BROWNIAN_MOTION};
    double time_step_ms_{1.0};  // Time between ticks in milliseconds
    std::atomic<uint64_t> global_sequence_{0};

    // Performance tracking
    std::atomic<uint64_t> ticks_generated_{0};
    std::atomic<uint64_t> ticks_dropped_{0};

public:
    explicit FeedSimulator(std::vector<SymbolConfig> symbols,
                          PriceModel model = PriceModel::GEOMETRIC_BROWNIAN_MOTION,
                          double tick_interval_ms = 1.0)
        : symbols_(std::move(symbols))
        , current_prices_(symbols_.size())
        , sequence_ids_(symbols_.size(), 0)
        , rng_(std::random_device{}())
        , model_(model)
        , time_step_ms_(tick_interval_ms) {

        // Initialize current prices
        for (size_t i = 0; i < symbols_.size(); ++i) {
            current_prices_[i] = symbols_[i].initial_price;
        }
    }

    // Generate next tick for all symbols
    template<typename Queue>
    void generate_ticks(Queue& queue) {
        const auto now = std::chrono::steady_clock::now();

        for (size_t i = 0; i < symbols_.size(); ++i) {
            auto tick = generate_tick(i, now);
            if (!queue.push(std::move(tick))) {
                ticks_dropped_.fetch_add(1, std::memory_order_relaxed);
            } else {
                ticks_generated_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    // Main simulation loop
    template<typename Queue>
    void run(Queue& queue, std::atomic<bool>& running,
             std::chrono::milliseconds duration = std::chrono::milliseconds::zero()) {

        const auto start_time = std::chrono::steady_clock::now();
        const auto tick_interval = std::chrono::microseconds(
            static_cast<int64_t>(time_step_ms_ * 1000));

        auto next_tick_time = start_time;

        while (running.load(std::memory_order_acquire)) {
            // Check duration limit
            if (duration.count() > 0) {
                const auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= duration) {
                    break;
                }
            }

            // Generate ticks for all symbols
            generate_ticks(queue);

            // Wait for next tick time
            next_tick_time += tick_interval;
            std::this_thread::sleep_until(next_tick_time);
        }
    }

    // Statistics
    [[nodiscard]] uint64_t ticks_generated() const noexcept {
        return ticks_generated_.load(std::memory_order_acquire);
    }

    [[nodiscard]] uint64_t ticks_dropped() const noexcept {
        return ticks_dropped_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double drop_rate() const noexcept {
        const auto generated = ticks_generated();
        const auto dropped = ticks_dropped();
        return generated > 0 ? static_cast<double>(dropped) / generated : 0.0;
    }

    [[nodiscard]] const std::vector<SymbolConfig>& symbols() const noexcept {
        return symbols_;
    }

    void reset_stats() noexcept {
        ticks_generated_.store(0, std::memory_order_release);
        ticks_dropped_.store(0, std::memory_order_release);
    }

private:
    Tick generate_tick(size_t symbol_idx, std::chrono::steady_clock::time_point timestamp) {
        const auto& config = symbols_[symbol_idx];

        // Update price based on model
        double& price = current_prices_[symbol_idx];
        update_price(price, config);

        // Round to tick size
        price = round_to_tick_size(price, config.tick_size);

        // Generate bid/ask around mid
        const auto [bid, ask] = generate_bid_ask(price, config);

        // Generate synthetic volume
        const double volume = generate_volume();

        return Tick{
            SymbolTable::intern(config.symbol),
            price,
            bid,
            ask,
            volume,
            ++sequence_ids_[symbol_idx]
        };
    }

    void update_price(double& price, const SymbolConfig& config) {
        const double dt = time_step_ms_ / (365.25 * 24 * 60 * 60 * 1000); // Convert to years
        const double z = normal_dist_(rng_);

        switch (model_) {
            case PriceModel::GEOMETRIC_BROWNIAN_MOTION: {
                // dS = μS dt + σS dW
                const double drift_term = config.drift * price * dt;
                const double diffusion_term = config.volatility * price * std::sqrt(dt) * z;
                price += drift_term + diffusion_term;
                break;
            }

            case PriceModel::ORNSTEIN_UHLENBECK: {
                // dS = θ(μ - S) dt + σ dW
                const double mean_rev_term = config.mean_reversion *
                    (config.initial_price - price) * dt;
                const double diffusion_term = config.volatility * std::sqrt(dt) * z;
                price += mean_rev_term + diffusion_term;
                break;
            }

            case PriceModel::JUMP_DIFFUSION: {
                // GBM + Poisson jumps
                const double drift_term = config.drift * price * dt;
                const double diffusion_term = config.volatility * price * std::sqrt(dt) * z;
                price += drift_term + diffusion_term;

                // Add jumps
                if (config.jump_intensity > 0.0) {
                    const double jump_prob = config.jump_intensity * dt;
                    if (uniform_dist_(rng_) < jump_prob) {
                        const double jump_size = config.jump_mean +
                            config.jump_std * normal_dist_(rng_);
                        price *= std::exp(jump_size);
                    }
                }
                break;
            }

            case PriceModel::MICROSTRUCTURE_NOISE: {
                // High-frequency noise model
                const double base_move = config.volatility * std::sqrt(dt) * z * price;
                const double noise = config.tick_size * normal_dist_(rng_) * 0.1;
                price += base_move + noise;
                break;
            }
        }

        // Ensure price stays positive
        price = std::max(price, config.tick_size);
    }

    [[nodiscard]] double round_to_tick_size(double price, double tick_size) const noexcept {
        return std::round(price / tick_size) * tick_size;
    }

    [[nodiscard]] std::pair<double, double> generate_bid_ask(
        double mid_price, const SymbolConfig& config) const {

        const double half_spread = mid_price * config.bid_ask_spread * 0.5;
        const double bid = round_to_tick_size(mid_price - half_spread, config.tick_size);
        const double ask = round_to_tick_size(mid_price + half_spread, config.tick_size);

        return {bid, ask};
    }

    [[nodiscard]] double generate_volume() const {
        // Simple exponential distribution for volume
        return std::max(1.0, exp_dist_(rng_) * 100.0);
    }
};