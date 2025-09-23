#pragma once
#include <chrono>
#include <string_view>
#include <cstdint>

// Symbol interning for zero-allocation string handling
struct SymbolTable {
    static constexpr size_t MAX_SYMBOLS = 256;
    static inline std::string_view intern(const std::string& sym) {
        // In production, this would use a hash table
        // For demo, just return string_view (assumes static lifetime)
        return std::string_view{sym};
    }
};

// Optimized tick structure - carefully ordered for cache efficiency
struct alignas(64) Tick {
    // Most frequently accessed fields first
    double last_price{0.0};        // 8 bytes
    double bid_price{0.0};         // 8 bytes
    double ask_price{0.0};         // 8 bytes
    double last_size{0.0};         // 8 bytes - Total: 32 bytes

    // Timestamp (critical for latency measurement)
    std::chrono::steady_clock::time_point timestamp; // 8 bytes

    // Symbol reference (zero-copy)
    std::string_view symbol;       // 16 bytes (ptr + size)

    // Sequence number for ordering/gap detection
    uint64_t sequence_id{0};       // 8 bytes - Total so far: 64 bytes

    // Default constructor
    Tick() = default;

    // Constructor with all fields
    Tick(std::string_view sym, double last, double bid, double ask,
         double size, uint64_t seq) noexcept
        : last_price(last)
        , bid_price(bid)
        , ask_price(ask)
        , last_size(size)
        , timestamp(std::chrono::steady_clock::now())
        , symbol(sym)
        , sequence_id(seq) {}

    // Move constructor and assignment (efficient)
    Tick(Tick&&) noexcept = default;
    Tick& operator=(Tick&&) noexcept = default;

    // Copy operations (for cases where needed)
    Tick(const Tick&) = default;
    Tick& operator=(const Tick&) = default;

    // Utility methods
    [[nodiscard]] double mid_price() const noexcept {
        return (bid_price + ask_price) * 0.5;
    }

    [[nodiscard]] double spread() const noexcept {
        return ask_price - bid_price;
    }

    [[nodiscard]] double spread_bps() const noexcept {
        const double mid = mid_price();
        return mid > 0.0 ? (spread() / mid) * 10000.0 : 0.0;
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return last_price > 0.0 && bid_price > 0.0 && ask_price > 0.0
               && bid_price <= ask_price && !symbol.empty();
    }
};

// Signal event structure
struct alignas(32) SignalEvent {
    enum class Type : uint8_t {
        Z_SCORE_BREAK,
        CORRELATION_BREAK,
        PAIR_TRADE_ENTRY,
        PAIR_TRADE_EXIT,
        VOLUME_SPIKE
    };

    // Event details
    Type event_type{Type::Z_SCORE_BREAK};
    std::string_view primary_symbol;
    std::string_view secondary_symbol; // For pair signals

    // Signal strength/score
    double signal_strength{0.0};
    double confidence{0.0};

    // Timing
    std::chrono::steady_clock::time_point event_time;
    std::chrono::steady_clock::time_point generation_time;

    // Sequence for ordering
    uint64_t signal_id{0};

    SignalEvent() = default;

    SignalEvent(Type type, std::string_view primary, double strength) noexcept
        : event_type(type)
        , primary_symbol(primary)
        , signal_strength(strength)
        , confidence(1.0)
        , event_time(std::chrono::steady_clock::now())
        , generation_time(event_time) {}

    SignalEvent(Type type, std::string_view primary, std::string_view secondary,
                double strength, double conf = 1.0) noexcept
        : event_type(type)
        , primary_symbol(primary)
        , secondary_symbol(secondary)
        , signal_strength(strength)
        , confidence(conf)
        , event_time(std::chrono::steady_clock::now())
        , generation_time(event_time) {}

    [[nodiscard]] std::chrono::microseconds latency() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            generation_time - event_time);
    }

    [[nodiscard]] const char* type_name() const noexcept {
        switch (event_type) {
            case Type::Z_SCORE_BREAK: return "ZBreak";
            case Type::CORRELATION_BREAK: return "CorrBreak";
            case Type::PAIR_TRADE_ENTRY: return "PairEntry";
            case Type::PAIR_TRADE_EXIT: return "PairExit";
            case Type::VOLUME_SPIKE: return "VolSpike";
            default: return "Unknown";
        }
    }
};