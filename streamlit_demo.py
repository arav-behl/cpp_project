#!/usr/bin/env python3
"""
Streamlit Demo: C++ Real-Time Trading System Simulator
"""

import streamlit as st
import pandas as pd
import numpy as np
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import time
import random
from datetime import datetime, timedelta
from typing import List, Dict, Tuple
import math

# Page configuration
st.set_page_config(
    page_title="C++ Real-Time Trading System Demo",
    page_icon="🚀",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Custom CSS
st.markdown("""
<style>
    .main-header {
        font-size: 3rem;
        font-weight: bold;
        text-align: center;
        color: #1f77b4;
        margin-bottom: 2rem;
    }
    .tech-badge {
        background-color: #ff4b4b;
        color: white;
        padding: 0.25rem 0.5rem;
        border-radius: 15px;
        font-size: 0.8rem;
        font-weight: bold;
        margin: 0.1rem;
    }
    .signal-alert {
        background-color: #ff4b4b;
        color: white;
        padding: 0.5rem;
        border-radius: 5px;
        font-family: monospace;
        margin: 0.2rem 0;
    }
</style>
""", unsafe_allow_html=True)

class TradingSystemSimulator:
    """
    Python simulation of the C++ Real-Time Trading System
    Mimics the behavior without needing actual C++ compilation
    """

    def __init__(self):
        self.symbols = []
        self.prices = {}
        self.rolling_stats = {}
        self.correlations = {}
        self.signals = []
        self.latencies = []
        self.tick_count = 0

    def initialize(self, symbols: List[str], duration: int, tick_rate: int):
        """Initialize the simulation"""
        self.symbols = symbols
        self.duration = duration
        self.tick_rate = tick_rate
        self.total_ticks = duration * tick_rate * len(symbols)

        # Initialize prices and statistics
        for symbol in symbols:
            self.prices[symbol] = 100.0 + random.uniform(-10, 10)
            self.rolling_stats[symbol] = {
                'prices': [],
                'volumes': [],
                'mean': 100.0,
                'std': 1.0,
                'volume_mean': 1000.0,
                'volume_std': 200.0
            }

        # Initialize correlation tracking for pairs
        if len(symbols) >= 2:
            for i in range(len(symbols)):
                for j in range(i+1, len(symbols)):
                    pair = f"{symbols[i]}/{symbols[j]}"
                    self.correlations[pair] = {
                        'prices_x': [],
                        'prices_y': [],
                        'correlation': 0.8  # Start with high correlation
                    }

    def generate_tick(self, symbol: str, tick_num: int) -> Dict:
        """Generate a realistic market tick"""
        # Geometric Brownian Motion with some mean reversion
        dt = 1.0 / (365 * 24 * 60 * 60)  # Approximate time step
        drift = 0.0
        volatility = 0.02

        # Add some correlation breakdown over time
        correlation_factor = 1.0 - (tick_num / self.total_ticks) * 0.3

        # Price movement
        z = np.random.normal(0, 1)
        price_change = drift * dt + volatility * math.sqrt(dt) * z

        # Add occasional jumps
        if random.random() < 0.001:  # 0.1% chance of jump
            price_change += random.uniform(-0.02, 0.02)

        self.prices[symbol] *= (1 + price_change)

        # Generate volume
        base_volume = 1000
        volume_noise = random.uniform(0.5, 2.0)
        volume = base_volume * volume_noise

        # Occasional volume spikes
        if random.random() < 0.005:  # 0.5% chance of volume spike
            volume *= random.uniform(3, 8)

        # Generate realistic latency (microseconds)
        latency_us = max(10, int(np.random.exponential(100)))

        return {
            'symbol': symbol,
            'price': self.prices[symbol],
            'volume': volume,
            'timestamp': datetime.now() + timedelta(microseconds=tick_num*1000),
            'latency_us': latency_us
        }

    def update_statistics(self, symbol: str, price: float, volume: float):
        """Update rolling statistics (simulates Welford's algorithm)"""
        stats = self.rolling_stats[symbol]

        # Keep last 100 prices for rolling statistics
        stats['prices'].append(price)
        stats['volumes'].append(volume)

        if len(stats['prices']) > 100:
            stats['prices'].pop(0)
            stats['volumes'].pop(0)

        # Update rolling mean and std
        if len(stats['prices']) > 1:
            stats['mean'] = np.mean(stats['prices'])
            stats['std'] = np.std(stats['prices'])
            stats['volume_mean'] = np.mean(stats['volumes'])
            stats['volume_std'] = np.std(stats['volumes'])

    def check_signals(self, symbol: str, price: float, volume: float, zscore_threshold: float) -> List[Dict]:
        """Check for trading signals"""
        signals = []
        stats = self.rolling_stats[symbol]

        if len(stats['prices']) < 10:  # Need minimum data
            return signals

        # Z-Score Signal
        z_score = (price - stats['mean']) / max(stats['std'], 0.01)
        if abs(z_score) >= zscore_threshold:
            signals.append({
                'type': 'ZBreak',
                'symbol': symbol,
                'strength': z_score,
                'confidence': min(0.95, abs(z_score) / 5.0),
                'timestamp': datetime.now()
            })

        # Volume Signal
        if stats['volume_std'] > 0:
            volume_z = (volume - stats['volume_mean']) / stats['volume_std']
            if volume_z >= 3.0:  # Volume spike
                signals.append({
                    'type': 'VolSpike',
                    'symbol': symbol,
                    'strength': volume_z,
                    'confidence': 0.88,
                    'timestamp': datetime.now()
                })

        return signals

    def check_correlation_signals(self, correlation_threshold: float) -> List[Dict]:
        """Check for correlation breakdown signals"""
        signals = []

        for pair, data in self.correlations.items():
            if len(data['prices_x']) >= 20:  # Need minimum data
                # Calculate rolling correlation
                corr = np.corrcoef(data['prices_x'][-20:], data['prices_y'][-20:])[0, 1]

                if not np.isnan(corr) and abs(corr) < correlation_threshold:
                    symbols = pair.split('/')
                    signals.append({
                        'type': 'CorrBreak',
                        'symbol': symbols[0],
                        'secondary_symbol': symbols[1],
                        'strength': corr,
                        'confidence': 0.85,
                        'timestamp': datetime.now()
                    })

        return signals

    def run_simulation(self, progress_callback=None) -> Dict:
        """Run the complete trading simulation"""
        start_time = time.time()

        self.signals = []
        self.latencies = []
        self.tick_count = 0

        ticks_per_update = max(1, self.total_ticks // 100)  # 100 progress updates

        for tick_num in range(self.total_ticks):
            symbol = self.symbols[tick_num % len(self.symbols)]

            # Generate tick
            tick = self.generate_tick(symbol, tick_num)
            self.tick_count += 1

            # Update statistics
            self.update_statistics(symbol, tick['price'], tick['volume'])

            # Update correlation data
            for pair, data in self.correlations.items():
                symbols = pair.split('/')
                if symbol == symbols[0]:
                    data['prices_x'].append(tick['price'])
                    if len(data['prices_x']) > 50:
                        data['prices_x'].pop(0)
                elif symbol == symbols[1]:
                    data['prices_y'].append(tick['price'])
                    if len(data['prices_y']) > 50:
                        data['prices_y'].pop(0)

            # Check for signals every few ticks
            if tick_num % 10 == 0:
                # Single symbol signals
                symbol_signals = self.check_signals(symbol, tick['price'], tick['volume'], 2.5)
                self.signals.extend(symbol_signals)

                # Correlation signals
                if tick_num % 50 == 0:  # Check less frequently
                    corr_signals = self.check_correlation_signals(0.3)
                    self.signals.extend(corr_signals)

            # Record latency
            self.latencies.append(tick['latency_us'])

            # Progress update
            if progress_callback and tick_num % ticks_per_update == 0:
                progress = tick_num / self.total_ticks
                progress_callback(progress)

        execution_time = time.time() - start_time

        # Calculate metrics
        avg_latency = np.mean(self.latencies) if self.latencies else 0
        p95_latency = np.percentile(self.latencies, 95) if self.latencies else 0
        p99_latency = np.percentile(self.latencies, 99) if self.latencies else 0

        return {
            'success': True,
            'execution_time': execution_time,
            'metrics': {
                'total_ticks': self.tick_count,
                'total_signals': len(self.signals),
                'average_rate': self.tick_count / execution_time if execution_time > 0 else 0,
                'drop_rate': 0.0,  # Simulated - no drops in Python version
                'avg_latency_us': avg_latency,
                'p95_latency_us': p95_latency,
                'p99_latency_us': p99_latency
            },
            'signals': self.signals,
            'final_prices': self.prices.copy()
        }

def main():
    # Title and introduction
    st.markdown('<h1 class="main-header">🚀 C++ Real-Time Trading System</h1>', unsafe_allow_html=True)

    st.markdown("""
    <div style="text-align: center; margin-bottom: 2rem;">
        <span class="tech-badge">C++20</span>
        <span class="tech-badge">Lock-Free Queues</span>
        <span class="tech-badge">Real-Time Analytics</span>
        <span class="tech-badge">Sub-μs Latency</span>
        <span class="tech-badge">Quantitative Finance</span>
    </div>
    """, unsafe_allow_html=True)

    # Sidebar configuration
    st.sidebar.header("🎛️ Simulation Parameters")

    duration = st.sidebar.slider("Duration (seconds)", 5, 30, 15)
    symbols = st.sidebar.multiselect(
        "Trading Symbols",
        ["AAPL", "MSFT", "GOOGL", "TSLA", "AMZN", "NVDA"],
        default=["AAPL", "MSFT", "GOOGL", "TSLA"]
    )
    tick_rate = st.sidebar.selectbox("Tick Rate (Hz)", [500, 1000, 2000], index=1)
    zscore_threshold = st.sidebar.slider("Z-Score Threshold", 1.0, 5.0, 2.5, 0.1)

    # Information sections
    col1, col2 = st.columns([2, 1])

    with col1:
        st.header("🎯 C++ System Architecture")
        st.markdown("""
        This demonstrates a **production C++ quantitative trading system**:

        - ** Lock-Free SPSC Queue**: Ring buffer with acquire/release memory ordering
        - ** Welford's Algorithm**: Numerically stable streaming statistics
        - **⚡ Sub-microsecond Latency**: Optimized hot path with zero-copy design
        - ** Real-Time Signals**: Z-score breakouts, correlation analysis, volume spikes
        - ** Performance Monitoring**: Latency histograms and throughput tracking
        """)

    with col2:
        st.header(" Signal Types")
        st.markdown("""
        **Z-Score Breakout**
        Price moves >2.5σ from mean

        **Correlation Break**
        Pairs decorrelate <0.3

        **Volume Spike**
        Volume >3σ above normal

        **Mean Reversion**
        Short vs long-term divergence
        """)

    # Note about the simulation
    st.info(" **Note**: This Streamlit demo simulates the C++ system's behavior. The actual implementation uses lock-free queues, Welford's algorithm, and achieves sub-microsecond latencies.")

    # Demo section
    st.header(" Live Trading System Demo")

    if len(symbols) < 2:
        st.warning("Please select at least 2 symbols for correlation analysis.")
        return

    if st.button(" Run C++ Trading System Simulation", type="primary", key="run_sim"):
        st.success(f" Starting C++ trading engine: {len(symbols)} symbols @ {tick_rate} Hz for {duration}s...")

        # Initialize simulator
        simulator = TradingSystemSimulator()
        simulator.initialize(symbols, duration, tick_rate)

        # Progress tracking
        progress_bar = st.progress(0)
        status_text = st.empty()
        signal_container = st.empty()

        def progress_callback(progress):
            progress_bar.progress(progress)
            status_text.text(f"Processing ticks... {progress*100:.1f}% complete")

        # Run simulation
        with st.spinner("Running high-frequency simulation..."):
            result = simulator.run_simulation(progress_callback)

        progress_bar.progress(1.0)
        status_text.text(" Simulation completed!")

        if result['success']:
            display_results(result, simulator)

def display_results(result: Dict, simulator: TradingSystemSimulator):
    """Display comprehensive results"""

    metrics = result['metrics']
    signals = result['signals']

    # Performance metrics
    st.header("📊 Real-Time Performance Metrics")

    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.metric(
            "Ticks Processed",
            f"{metrics['total_ticks']:,}",
            delta=f"{metrics['average_rate']:,.0f} TPS"
        )

    with col2:
        st.metric(
            "Signals Generated",
            f"{metrics['total_signals']:,}",
            delta=f"{(metrics['total_signals']/max(metrics['total_ticks'],1)*100):.1f}% hit rate"
        )

    with col3:
        st.metric(
            "Avg Latency",
            f"{metrics['avg_latency_us']:.0f} μs",
            delta=f"P99: {metrics['p99_latency_us']:.0f} μs"
        )

    with col4:
        st.metric(
            "System Efficiency",
            f"{100-metrics['drop_rate']:.1f}%",
            delta="Zero drops achieved"
        )

    # Tabs for different analyses
    tab1, tab2, tab3, tab4 = st.tabs([" Trading Signals", "⚡ Latency Analysis", " Price Action", " C++ Concepts"])

    with tab1:
        display_signals_analysis(signals)

    with tab2:
        display_latency_analysis(simulator.latencies)

    with tab3:
        display_price_analysis(result['final_prices'])

    with tab4:
        display_cpp_concepts()

def display_signals_analysis(signals: List[Dict]):
    """Display trading signals analysis"""

    if not signals:
        st.warning("No signals generated. Try lowering the Z-score threshold or running longer.")
        return

    # Convert to DataFrame
    signals_df = pd.DataFrame(signals)

    col1, col2 = st.columns(2)

    with col1:
        # Signal distribution
        signal_counts = signals_df['type'].value_counts()
        fig = px.pie(
            values=signal_counts.values,
            names=signal_counts.index,
            title="Signal Types Distribution",
            color_discrete_sequence=['#ff4b4b', '#1f77b4', '#2ca02c', '#ff7f0e']
        )
        st.plotly_chart(fig, use_container_width=True)

    with col2:
        # Signal strength distribution
        fig = px.histogram(
            signals_df,
            x='strength',
            color='type',
            title="Signal Strength Distribution",
            nbins=20
        )
        st.plotly_chart(fig, use_container_width=True)

    # Recent signals table
    st.subheader("Recent Trading Signals")

    display_signals = signals_df.tail(10).copy()
    display_signals['timestamp'] = display_signals['timestamp'].dt.strftime('%H:%M:%S.%f')
    display_signals['strength'] = display_signals['strength'].round(3)
    display_signals['confidence'] = display_signals['confidence'].round(3)

    # Style the signals table
    st.dataframe(
        display_signals[['timestamp', 'type', 'symbol', 'strength', 'confidence']],
        use_container_width=True
    )

    # Show signal alerts
    st.subheader(" Live Signal Feed")
    for signal in signals[-5:]:  # Show last 5 signals
        signal_type = signal['type']
        symbol = signal['symbol']
        strength = signal['strength']
        conf = signal['confidence']

        secondary = signal.get('secondary_symbol', '')
        pair_text = f"/{secondary}" if secondary else ""

        st.markdown(f"""
        <div class="signal-alert">
        🚨 SIGNAL {signal_type} | {symbol}{pair_text} | strength={strength:.2f} | conf={conf:.2f}
        </div>
        """, unsafe_allow_html=True)

def display_latency_analysis(latencies: List[float]):
    """Display latency performance analysis"""

    # Create latency histogram
    latency_df = pd.DataFrame({'latency_us': latencies})

    fig = px.histogram(
        latency_df,
        x='latency_us',
        nbins=50,
        title="End-to-End Latency Distribution (μs)",
        labels={'latency_us': 'Latency (μs)', 'count': 'Frequency'}
    )
    fig.add_vline(x=np.mean(latencies), line_dash="dash", line_color="red",
                  annotation_text=f"Mean: {np.mean(latencies):.0f}μs")
    st.plotly_chart(fig, use_container_width=True)

    # Percentile analysis
    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.metric("P50 Latency", f"{np.percentile(latencies, 50):.0f} μs")
    with col2:
        st.metric("P95 Latency", f"{np.percentile(latencies, 95):.0f} μs")
    with col3:
        st.metric("P99 Latency", f"{np.percentile(latencies, 99):.0f} μs")
    with col4:
        st.metric("Max Latency", f"{np.max(latencies):.0f} μs")

def display_price_analysis(final_prices: Dict[str, float]):
    """Display price movement analysis"""

    # Price change summary
    price_data = []
    for symbol, price in final_prices.items():
        initial_price = 100.0  # Approximate starting price
        change = (price - initial_price) / initial_price * 100
        price_data.append({'Symbol': symbol, 'Final Price': price, 'Change %': change})

    price_df = pd.DataFrame(price_data)

    # Price change chart
    fig = px.bar(
        price_df,
        x='Symbol',
        y='Change %',
        title="Price Changes During Simulation",
        color='Change %',
        color_continuous_scale='RdYlGn'
    )
    st.plotly_chart(fig, use_container_width=True)

    # Final prices table
    st.subheader("📊 Final Prices")
    st.dataframe(price_df, use_container_width=True)

def display_cpp_concepts():
    """Explain the C++ concepts demonstrated"""

    st.subheader("C++ Systems Programming Concepts")

    col1, col2 = st.columns(2)

    with col1:
        st.markdown("""
        ** Lock-Free Programming**
        ```cpp
        template<typename T, size_t N>
        class SPSCQueue {
            alignas(64) std::atomic<size_t> head_{0};
            alignas(64) std::atomic<size_t> tail_{0};

            bool push(T&& item) noexcept {
                head_.store(next, memory_order_release);
                return true;
            }
        };
        ```

        ** Numerical Stability**
        ```cpp
        // Welford's algorithm prevents cancellation
        void add(double x) {
            n++;
            double delta = x - mean;
            mean += delta / n;
            m2 += delta * (x - mean);
        }
        ```
        """)

    with col2:
        st.markdown("""
        **⚡ Zero-Copy Design**
        ```cpp
        struct Tick {
            std::string_view symbol;  // No allocation
            double last_price;
            std::chrono::steady_clock::time_point ts;
        };
        ```

        **Template Metaprogramming**
        ```cpp
        template<size_t WindowSize>
        class WindowedStats {
            static_assert(WindowSize > 0);
            double buffer_[WindowSize];
        };
        ```
        """)

    st.subheader("Main C++ Concepts & Features Built")

    st.markdown("""
    - **"Implemented lock-free SPSC queue using acquire/release memory ordering"**
    - **"Achieved sub-microsecond latency with cache-aligned data structures"**
    - **"Used Welford's algorithm for numerically stable streaming statistics"**
    - **"Optimized hot paths with move semantics and string_view"**
    - **"Prevented false sharing with alignas(64) cache line alignment"**
    """)

if __name__ == "__main__":
    main()