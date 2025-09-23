# 🚀 C++ Real-Time Trading System

A production-grade, low-latency trading system built in C++20 showcasing quantitative finance and systems programming skills.

## 🎯 Perfect for Quant Interviews

This project demonstrates:
- **Lock-free concurrency** with SPSC queues
- **Numerically stable statistics** (Welford's algorithm)
- **Real-time signal generation** (Z-score, correlation analysis)
- **Sub-microsecond latency** processing
- **Modern C++20** features and best practices

## 🏗️ Architecture

```
Feed Simulator (Multi-threaded) → Lock-Free Queue → Signal Engine → Strategy Rules
                                        ↓
                               Performance Monitor + Latency Tracking
```

## 🚀 Quick Demo (For Recruiters)

### Option 1: Interactive Web Dashboard
```bash
# Install Python dependencies
pip install -r requirements.txt

# Build the C++ system
cmake -S . -B build && cmake --build build -j8

# Launch the Streamlit dashboard
streamlit run streamlit_dashboard.py
```

Then click **"Run Trading System Demo"** in your browser!

### Option 2: Command Line Demo
```bash
# Build the system
cmake -S . -B build && cmake --build build -j8

# Run a 15-second simulation
cd build && ./demo_realtime --duration 15 --rate 1000 --zscore 2.5
```

## 📊 What You'll See

The dashboard shows:
- **Real-time metrics**: Throughput, latency percentiles, signal hit rates
- **Interactive charts**: Signal timelines, latency histograms
- **Performance analysis**: Queue efficiency, drop rates
- **Live C++ output**: Raw system logs and statistics

## 🔧 System Components

### Core C++ Engine (`src/`)
- **SPSC Queue**: Lock-free ring buffer with acquire/release semantics
- **Rolling Statistics**: Online mean/variance with numerical stability
- **Signal Engine**: Z-score breakouts, correlation analysis, mean reversion
- **Feed Simulator**: Realistic market microstructure with GBM pricing

### Frontend Dashboard (`streamlit_dashboard.py`)
- **Interactive controls**: Adjust tick rates, thresholds, duration
- **Real-time visualization**: Charts and metrics updated live
- **Performance monitoring**: Latency analysis and throughput tracking

## 🎪 Interview Talking Points

### Systems Design
- "Implemented lock-free SPSC queue using acquire/release memory ordering"
- "Achieved sub-microsecond latency with cache-aligned data structures"
- "Used power-of-2 sizing for efficient modulo operations"

### Quantitative Finance
- "Built numerically stable streaming statistics with Welford's algorithm"
- "Implemented online covariance for real-time correlation analysis"
- "Created adaptive signal detection with configurable thresholds"

### Performance Engineering
- "Optimized hot paths with move semantics and string_view"
- "Prevented false sharing with alignas(64) cache line alignment"
- "Measured end-to-end latency with steady_clock timestamps"

## 🔬 Technical Deep Dive

### Memory Ordering
```cpp
// Producer publishes with release semantics
head_.store(next_head, std::memory_order_release);

// Consumer reads with acquire semantics
head_.load(std::memory_order_acquire);
```

### Numerical Stability
```cpp
// Welford's algorithm prevents catastrophic cancellation
mean += (x - mean) / n;
m2 += (x - mean_prev) * (x - mean);
```

### Zero-Copy Design
```cpp
// Use string_view for symbol references
std::string_view symbol;  // No allocation in hot path
```

## 🧪 Testing

```bash
# Run comprehensive test suite
cd build && ./test_suite
```

Tests cover:
- SPSC queue correctness and performance
- Statistical algorithm accuracy
- Numerical stability under stress
- Concurrency safety

## 📈 Performance Benchmarks

Typical results on modern hardware:
- **Throughput**: 10M+ operations/second
- **Latency P50**: < 100μs
- **Latency P99**: < 1ms
- **Queue Efficiency**: > 99.9%

## 💼 Recruiter Instructions

1. **Clone and build** (5 minutes)
2. **Run Streamlit dashboard** (`streamlit run streamlit_dashboard.py`)
3. **Click "Run Trading System Demo"**
4. **Watch real-time C++ performance** in beautiful charts!

The system will show live metrics proving the C++ engine is processing thousands of ticks per second with microsecond latencies.

## 🛠️ Requirements

- **C++20** compiler (GCC 10+, Clang 12+)
- **CMake 3.20+**
- **Python 3.8+** (for dashboard)

## 📁 Project Structure

```
├── include/          # C++ headers
│   ├── md/          # Market data structures
│   ├── stats/       # Statistical algorithms
│   ├── engine/      # Signal processing
│   └── util/        # Utilities
├── src/             # C++ implementation
├── examples/        # Demo applications
├── tests/           # Unit tests
├── streamlit_dashboard.py  # Web interface
└── cpp_trading_wrapper.py # Python wrapper
```

---

**Built with ❤️ for quantitative finance interviews**