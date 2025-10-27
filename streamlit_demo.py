#!/usr/bin/env python3
"""
Streamlit Demo: C++ Real-Time Trading System
Interactive visualization of exported CSV data
"""

import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from pathlib import Path
import numpy as np

# Page configuration
st.set_page_config(
    page_title="C++ Real-Time Trading System",
    page_icon="📊",
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
        margin-bottom: 1rem;
    }
    .tech-badge {
        background-color: #ff4b4b;
        color: white;
        padding: 0.25rem 0.5rem;
        border-radius: 15px;
        font-size: 0.8rem;
        font-weight: bold;
        margin: 0.2rem;
    }
    .metric-card {
        background-color: #f0f2f6;
        padding: 1rem;
        border-radius: 10px;
        border-left: 4px solid #ff4b4b;
    }
    .signal-card {
        background-color: #fff3cd;
        padding: 0.5rem;
        border-radius: 5px;
        border-left: 3px solid #ffc107;
        margin: 0.5rem 0;
    }
</style>
""", unsafe_allow_html=True)

def load_data():
    """Load CSV data files"""
    data_dir = Path("data")

    signals_df = None
    latency_df = None

    # Load signals
    signals_path = data_dir / "signals.csv"
    if signals_path.exists():
        try:
            # For very large files, sample the data
            file_size = signals_path.stat().st_size
            if file_size > 10_000_000:  # If larger than 10MB
                # Sample 100k rows
                signals_df = pd.read_csv(signals_path, nrows=100_000)
                st.info(f"📊 Large dataset detected ({file_size / 1_000_000:.1f}MB). Showing sample of 100k signals.")
            else:
                signals_df = pd.read_csv(signals_path)

            # Convert timestamp to seconds for better readability
            if 'timestamp' in signals_df.columns:
                signals_df['timestamp_sec'] = signals_df['timestamp'] / 1_000_000
        except Exception as e:
            st.error(f"Error loading signals.csv: {e}")

    # Load latency histogram
    latency_path = data_dir / "latency_histogram.csv"
    if latency_path.exists():
        try:
            latency_df = pd.read_csv(latency_path)
        except Exception as e:
            st.error(f"Error loading latency_histogram.csv: {e}")

    return signals_df, latency_df

def main():
    # Title
    st.markdown('<h1 class="main-header">C++ Real-Time Trading System</h1>', unsafe_allow_html=True)

    st.markdown("""
    <div style="text-align: center; margin-bottom: 2rem;">
        <span class="tech-badge">C++20</span>
        <span class="tech-badge">Lock-Free Queues</span>
        <span class="tech-badge">Real-Time Analytics</span>
        <span class="tech-badge">Sub-μs Latency</span>
        <span class="tech-badge">Welford's Algorithm</span>
    </div>
    """, unsafe_allow_html=True)

    # Load data
    signals_df, latency_df = load_data()

    # Check if data exists
    has_data = signals_df is not None and latency_df is not None

    if not has_data:
        st.error("This demo requires local C++ compilation")
        st.markdown("""
        ## This Demo Requires Local Setup

        This C++ trading system needs to be **compiled and run locally** because:
        - It's a native C++ executable (compiled binary)
        - Streamlit Cloud runs on Linux, but the code needs to be compiled for your OS
        - Live performance requires local hardware access

        ### To Run Locally:

        ```bash
        # Clone the repository
        git clone https://github.com/arav-behl/cpp_project
        cd cpp_project

        # Build the C++ system
        cmake -S . -B build && cmake --build build -j8

        # Option 1: Run in terminal (recommended)
        ./build/demo_realtime --duration 30 --rate 2000

        # Option 2: Run this Streamlit dashboard
        pip install streamlit pandas plotly
        streamlit run streamlit_demo.py
        ```

        ### What You'll See:

        - **Live terminal output** streaming in real-time
        - **Signal detections** as they fire (Z-score, correlation breaks)
        - **Latency statistics** (P50/P95/P99 in microseconds)
        - **Throughput metrics** (thousands of ticks per second)
        - **Performance charts** with interactive visualizations

        ---

        ### Architecture Overview

        ```
        ┌─────────────────┐
        │ Feed Simulator  │ ← GBM/OU pricing models
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │ SPSC Queue 64K  │ ← Lock-free, cache-aligned
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │ Signal Engine   │ ← Z-score, correlation, volume
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │ Latency Monitor │ ← P50/P95/P99 tracking
        └─────────────────┘
        ```

        ### Key Technical Features:

        - **Lock-Free Concurrency**: SPSC queue with acquire/release memory ordering
        - **Numerical Stability**: Welford's algorithm for streaming variance
        - **Cache-Aware Design**: alignas(64) to prevent false sharing
        - **Zero-Copy**: Move semantics and string_view for hot path
        - **Sub-Millisecond Latency**: Steady clock timestamps, bounded memory

        ### Performance Benchmarks:

        Typical results on modern hardware:
        - **Throughput**: 1-5M ticks/sec
        - **P50 Latency**: 100-300μs
        - **P99 Latency**: 1-3ms
        - **Queue Efficiency**: >99.9%

        ---

        **GitHub Repository**: https://github.com/arav-behl/cpp_project
        """)

        # Show example output
        st.markdown("### Example Terminal Output")
        st.code("""
╔══════════════════════════════════════════════════════════════╗
║              REAL-TIME TRADING SYSTEM                        ║
║                    C++20 Low-Latency Engine                  ║
╠══════════════════════════════════════════════════════════════╣
║ Runtime: 15s                                                 ║
║ Feed: 120000 ticks | Dropped: 0 (0.00%)                    ║
║ Queue: 12.5% full                                           ║
║ Processed: 120000 ticks | Rate: 8000 TPS                   ║
║ Signals: 23                                                 ║
║ Latency: P50=125μs | P95=380μs | P99=950μs                 ║
╚══════════════════════════════════════════════════════════════╝

SIGNAL 000001 | ZBreak | AAPL | strength=2.61 | lat=120μs
SIGNAL 000002 | CorrBreak | AAPL/MSFT | strength=0.18 | lat=135μs
SIGNAL 000003 | VolSpike | TSLA | strength=3.42 | lat=98μs
        """, language="text")

    else:
        # ========================================
        # MAIN DASHBOARD WITH DATA VISUALIZATION
        # ========================================

        st.success("Data loaded successfully!")

        # ========================================
        # SIDEBAR CONTROLS
        # ========================================
        with st.sidebar:
            st.header("Dashboard Controls")

            # Signal type filter
            signal_types = ['All'] + sorted(signals_df['type'].unique().tolist())
            selected_signal_type = st.selectbox("Signal Type", signal_types)

            # Symbol filter
            all_symbols = set()
            for col in ['primary_symbol', 'secondary_symbol']:
                if col in signals_df.columns:
                    all_symbols.update(signals_df[col].dropna().unique())
            all_symbols = ['All'] + sorted(list(all_symbols))
            selected_symbol = st.selectbox("Symbol", all_symbols)

            # Time range
            if 'timestamp_sec' in signals_df.columns:
                min_time = float(signals_df['timestamp_sec'].min())
                max_time = float(signals_df['timestamp_sec'].max())
                time_range = st.slider(
                    "Time Range (seconds)",
                    min_value=min_time,
                    max_value=max_time,
                    value=(min_time, max_time)
                )

            st.markdown("---")
            st.markdown("### About This System")
            st.markdown("""
            **Architecture:**
            - Lock-free SPSC queue
            - Welford's streaming stats
            - Multi-threaded design

            **Signals:**
            - Z-Score breakouts
            - Correlation breaks
            - Volume spikes
            """)

        # ========================================
        # FILTER DATA
        # ========================================
        filtered_signals = signals_df.copy()

        if selected_signal_type != 'All':
            filtered_signals = filtered_signals[filtered_signals['type'] == selected_signal_type]

        if selected_symbol != 'All':
            filtered_signals = filtered_signals[
                (filtered_signals['primary_symbol'] == selected_symbol) |
                (filtered_signals['secondary_symbol'] == selected_symbol)
            ]

        if 'timestamp_sec' in filtered_signals.columns:
            filtered_signals = filtered_signals[
                (filtered_signals['timestamp_sec'] >= time_range[0]) &
                (filtered_signals['timestamp_sec'] <= time_range[1])
            ]

        # ========================================
        # KEY METRICS ROW
        # ========================================
        st.markdown("## Performance Metrics")

        col1, col2, col3, col4 = st.columns(4)

        with col1:
            total_signals = len(filtered_signals)
            st.metric("Total Signals", f"{total_signals:,}")

        with col2:
            # Calculate average latency from histogram
            if latency_df is not None and 'count' in latency_df.columns:
                total_samples = latency_df['count'].sum()
                # Weighted average using bucket midpoints
                latency_df['midpoint'] = (latency_df['lower_bound_us'] + latency_df['upper_bound_us']) / 2
                avg_latency = (latency_df['midpoint'] * latency_df['count']).sum() / total_samples
                st.metric("Avg Latency", f"{avg_latency:.0f} μs")

        with col3:
            # Calculate P99 latency
            cumsum = latency_df['percentage'].cumsum()
            p99_bucket = latency_df[cumsum >= 99.0].iloc[0] if len(latency_df[cumsum >= 99.0]) > 0 else latency_df.iloc[-1]
            st.metric("P99 Latency", f"{p99_bucket['upper_bound_us']:.0f} μs")

        with col4:
            unique_pairs = filtered_signals[['primary_symbol', 'secondary_symbol']].drop_duplicates()
            st.metric("Active Pairs", len(unique_pairs))

        st.markdown("---")

        # ========================================
        # LATENCY DISTRIBUTION
        # ========================================
        st.markdown("## Latency Distribution")

        col1, col2 = st.columns([2, 1])

        with col1:
            # Latency histogram
            fig_latency = go.Figure()

            # Filter out extreme outliers for better visualization
            display_latency = latency_df[latency_df['upper_bound_us'] <= 50000].copy()

            fig_latency.add_trace(go.Bar(
                x=[f"{int(row['lower_bound_us'])}-{int(row['upper_bound_us'])}" for _, row in display_latency.iterrows()],
                y=display_latency['percentage'],
                text=[f"{p:.2f}%" for p in display_latency['percentage']],
                textposition='auto',
                marker_color='#ff4b4b',
                hovertemplate='<b>%{x} μs</b><br>%{y:.2f}%<extra></extra>'
            ))

            fig_latency.update_layout(
                title="Latency Histogram (< 50ms range)",
                xaxis_title="Latency Range (μs)",
                yaxis_title="Percentage (%)",
                height=400,
                hovermode='x unified',
                showlegend=False
            )

            st.plotly_chart(fig_latency, use_container_width=True)

        with col2:
            st.markdown("### Latency Percentiles")

            # Calculate percentiles
            percentiles = [50, 75, 90, 95, 99]
            cumsum = latency_df['percentage'].cumsum()

            for p in percentiles:
                bucket = latency_df[cumsum >= p].iloc[0] if len(latency_df[cumsum >= p]) > 0 else latency_df.iloc[-1]
                st.markdown(f"""
                <div class="metric-card">
                    <strong>P{p}</strong>: {bucket['upper_bound_us']:.0f} μs
                </div>
                """, unsafe_allow_html=True)

        st.markdown("---")

        # ========================================
        # SIGNAL ANALYSIS
        # ========================================
        st.markdown("## Signal Analysis")

        col1, col2 = st.columns(2)

        with col1:
            # Signal type distribution
            signal_counts = filtered_signals['type'].value_counts()

            fig_signal_types = go.Figure(data=[go.Pie(
                labels=signal_counts.index,
                values=signal_counts.values,
                hole=0.4,
                marker=dict(colors=['#ff4b4b', '#ffa600', '#00cc96'])
            )])

            fig_signal_types.update_layout(
                title="Signal Type Distribution",
                height=400
            )

            st.plotly_chart(fig_signal_types, use_container_width=True)

        with col2:
            # Signal strength distribution
            if 'signal_strength' in filtered_signals.columns:
                fig_strength = px.histogram(
                    filtered_signals[filtered_signals['signal_strength'] > 0],
                    x='signal_strength',
                    nbins=30,
                    title="Signal Strength Distribution",
                    color_discrete_sequence=['#ff4b4b']
                )
                fig_strength.update_layout(height=400)
                st.plotly_chart(fig_strength, use_container_width=True)

        # ========================================
        # SIGNALS OVER TIME
        # ========================================
        if 'timestamp_sec' in filtered_signals.columns:
            st.markdown("## Signals Over Time")

            # Group signals by time windows
            time_bins = pd.cut(filtered_signals['timestamp_sec'], bins=50)
            signals_over_time = filtered_signals.groupby([time_bins, 'type']).size().reset_index(name='count')
            signals_over_time['time_midpoint'] = signals_over_time['timestamp_sec'].apply(lambda x: x.mid)

            fig_timeline = px.line(
                signals_over_time,
                x='time_midpoint',
                y='count',
                color='type',
                title="Signal Frequency Over Time",
                labels={'time_midpoint': 'Time (seconds)', 'count': 'Signal Count'},
                color_discrete_sequence=['#ff4b4b', '#ffa600', '#00cc96']
            )

            fig_timeline.update_layout(height=400, hovermode='x unified')
            st.plotly_chart(fig_timeline, use_container_width=True)

        st.markdown("---")

        # ========================================
        # SYMBOL PAIR ANALYSIS
        # ========================================
        st.markdown("## Symbol Pair Analysis")

        # Get correlation break signals
        corr_signals = filtered_signals[
            (filtered_signals['type'] == 'CorrBreak') &
            (filtered_signals['secondary_symbol'].notna())
        ]

        if len(corr_signals) > 0:
            # Count signals per pair
            corr_signals['pair'] = corr_signals['primary_symbol'] + ' / ' + corr_signals['secondary_symbol']
            pair_counts = corr_signals['pair'].value_counts().head(10)

            fig_pairs = go.Figure(data=[go.Bar(
                x=pair_counts.values,
                y=pair_counts.index,
                orientation='h',
                marker_color='#ff4b4b',
                text=pair_counts.values,
                textposition='auto'
            )])

            fig_pairs.update_layout(
                title="Top 10 Most Active Pairs (Correlation Breaks)",
                xaxis_title="Signal Count",
                yaxis_title="Symbol Pair",
                height=400
            )

            st.plotly_chart(fig_pairs, use_container_width=True)

        # ========================================
        # RECENT SIGNALS TABLE
        # ========================================
        st.markdown("## Recent Signals")

        # Show top signals
        display_columns = ['timestamp_sec', 'signal_id', 'type', 'primary_symbol', 'secondary_symbol', 'signal_strength', 'confidence']
        display_columns = [col for col in display_columns if col in filtered_signals.columns]

        recent_signals = filtered_signals.nlargest(100, 'signal_id')[display_columns]

        # Format for display
        if 'timestamp_sec' in recent_signals.columns:
            recent_signals['timestamp_sec'] = recent_signals['timestamp_sec'].round(3)

        st.dataframe(
            recent_signals,
            use_container_width=True,
            height=400
        )

        # ========================================
        # TECHNICAL DETAILS EXPANDER
        # ========================================
        with st.expander("Technical Implementation Details"):
            st.markdown("""
            ### System Architecture

            **Threading Model:**
            - **Producer Thread**: Feed simulator generating market data ticks
            - **Consumer Thread**: Signal engine processing ticks and detecting patterns
            - **SPSC Queue**: Lock-free ring buffer (65536 capacity) connecting producer/consumer

            **Statistical Methods:**
            - **Welford's Algorithm**: Online mean/variance calculation with numerical stability
            - **Online Covariance**: Streaming correlation computation between symbol pairs
            - **EMA Smoothing**: Exponential moving averages for fast rolling statistics

            **Signal Detection:**
            - **Z-Score Breakout**: Triggers when price deviates > 2.5σ from mean
            - **Correlation Break**: Detects when pair correlation drops below threshold
            - **Volume Spike**: Identifies abnormal trading volume (> 3σ)

            **Performance Optimizations:**
            - Cache-line alignment (64-byte) to prevent false sharing
            - Move semantics and string_view to avoid allocations
            - Steady clock timestamps for monotonic latency measurement
            - Bounded memory with fixed-size ring buffers

            ### Build Instructions

            ```bash
            cmake -S . -B build && cmake --build build -j8
            ./build/demo_realtime --duration 30 --rate 2000
            ```
            """)

if __name__ == "__main__":
    main()
