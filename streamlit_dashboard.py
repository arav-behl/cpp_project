#!/usr/bin/env python3
"""
Streamlit Dashboard for C++ Real-Time Trading System
Perfect for showcasing to recruiters!
"""

import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import time
import json
from cpp_trading_wrapper import TradingSystemWrapper

# Page configuration
st.set_page_config(
    page_title="C++ Real-Time Trading System",
    page_icon="🚀",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Custom CSS for better styling
st.markdown("""
<style>
    .main-header {
        font-size: 3rem;
        font-weight: bold;
        text-align: center;
        color: #1f77b4;
        margin-bottom: 2rem;
    }
    .metric-card {
        background-color: #f0f2f6;
        padding: 1rem;
        border-radius: 10px;
        border-left: 5px solid #1f77b4;
    }
    .tech-badge {
        background-color: #ff4b4b;
        color: white;
        padding: 0.25rem 0.5rem;
        border-radius: 15px;
        font-size: 0.8rem;
        font-weight: bold;
    }
</style>
""", unsafe_allow_html=True)

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

    # Sidebar for configuration
    st.sidebar.header("🎛️ Simulation Parameters")

    duration = st.sidebar.slider("Duration (seconds)", 5, 60, 15)
    symbols = st.sidebar.multiselect(
        "Trading Symbols",
        ["AAPL", "MSFT", "GOOGL", "TSLA", "AMZN", "NVDA"],
        default=["AAPL", "MSFT", "GOOGL", "TSLA"]
    )
    tick_rate = st.sidebar.selectbox("Tick Rate (Hz)", [100, 500, 1000, 2000], index=2)
    zscore_threshold = st.sidebar.slider("Z-Score Threshold", 1.0, 5.0, 2.5, 0.1)

    # System info
    st.sidebar.header("🔧 System Information")
    try:
        wrapper = TradingSystemWrapper()
        system_info = wrapper.get_system_info()

        if system_info['executable_exists']:
            st.sidebar.success("✅ C++ Engine Ready")
        else:
            st.sidebar.error("❌ C++ Engine Not Found")
            st.sidebar.text(f"Looking for: {system_info['executable_path']}")
    except Exception as e:
        st.sidebar.error(f"❌ System Error: {str(e)}")
        return

    # Main content area
    col1, col2 = st.columns([2, 1])

    with col1:
        st.header("🎯 What This Demonstrates")
        st.markdown("""
        This system showcases **production-grade C++** skills essential for quantitative finance:

        - **🔒 Lock-Free Concurrency**: SPSC queue with acquire/release memory ordering
        - **📊 Streaming Statistics**: Numerically stable Welford's algorithm for online mean/variance
        - **⚡ Ultra-Low Latency**: Sub-microsecond tick-to-signal processing
        - **🧮 Quantitative Models**: Z-score breakouts, correlation analysis, mean reversion
        - **📈 Real-Time Analytics**: Live performance monitoring and latency histograms
        """)

    with col2:
        st.header("🏗️ Architecture")
        st.markdown("""
        ```
        Feed Simulator
             ↓
        Lock-Free Queue
             ↓
        Signal Engine
             ↓
        Strategy Rules
             ↓
        Performance Monitor
        ```
        """)

    # The main demo button
    st.header("🚀 Live Demo")

    if st.button("🎬 Run Trading System Demo", type="primary"):
        # Show what's happening
        st.info(f"🔄 Starting C++ trading engine with {len(symbols)} symbols at {tick_rate} Hz for {duration} seconds...")

        # Create placeholder for live updates
        progress_bar = st.progress(0)
        status_text = st.empty()

        # Run the simulation
        symbols_str = ",".join(symbols)

        try:
            result = wrapper.run_simulation(
                duration=duration,
                symbols=symbols_str,
                tick_rate=tick_rate,
                zscore_threshold=zscore_threshold
            )

            progress_bar.progress(100)

            if result['success']:
                st.success(f"✅ Simulation completed in {result['execution_time']:.2f} seconds!")
                display_results(result)
            else:
                st.error(f"❌ Simulation failed: {result.get('error', 'Unknown error')}")

        except Exception as e:
            st.error(f"❌ Error running simulation: {str(e)}")

def display_results(result):
    """Display simulation results with beautiful charts"""

    metrics = result['metrics']
    signals_df = result['signals']
    latency_df = result['latency_histogram']
    config = result['config']

    # Performance metrics
    st.header("📊 Performance Metrics")

    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.metric(
            "Ticks Processed",
            f"{metrics.get('total_ticks', 0):,}",
            delta=f"{metrics.get('average_rate', 0):,.0f} TPS"
        )

    with col2:
        st.metric(
            "Signals Generated",
            f"{metrics.get('total_signals', 0):,}",
            delta=f"{(metrics.get('total_signals', 0) / max(metrics.get('total_ticks', 1), 1) * 100):.1f}% hit rate"
        )

    with col3:
        st.metric(
            "Throughput",
            f"{metrics.get('average_rate', 0):,.0f} TPS",
            delta="Real-time processing"
        )

    with col4:
        drop_rate = metrics.get('drop_rate', 0)
        st.metric(
            "Queue Efficiency",
            f"{100 - drop_rate:.1f}%",
            delta=f"{drop_rate:.2f}% drops"
        )

    # Charts section
    st.header("📈 Analytics Dashboard")

    # Create tabs for different views
    tab1, tab2, tab3, tab4 = st.tabs(["🎯 Signals", "⚡ Latency", "📊 Performance", "🔧 Raw Output"])

    with tab1:
        display_signals_analysis(signals_df, config)

    with tab2:
        display_latency_analysis(latency_df)

    with tab3:
        display_performance_analysis(metrics, config)

    with tab4:
        st.subheader("📝 C++ System Output")
        st.code(result['raw_output'], language='text')

def display_signals_analysis(signals_df, config):
    """Display signals analysis"""
    if signals_df is None or signals_df.empty:
        st.warning("No signals generated during this run. Try lowering the Z-score threshold.")
        return

    col1, col2 = st.columns(2)

    with col1:
        # Signal types distribution
        signal_counts = signals_df['type'].value_counts()
        fig = px.pie(
            values=signal_counts.values,
            names=signal_counts.index,
            title="Signal Types Distribution"
        )
        st.plotly_chart(fig, use_container_width=True)

    with col2:
        # Signal timeline
        signals_df['timestamp'] = pd.to_datetime(signals_df['timestamp'], unit='ms')
        fig = px.scatter(
            signals_df,
            x='timestamp',
            y='signal_strength',
            color='type',
            title="Signal Timeline",
            hover_data=['primary_symbol', 'confidence']
        )
        st.plotly_chart(fig, use_container_width=True)

    # Signals table
    st.subheader("🎯 Generated Signals")
    st.dataframe(
        signals_df[['timestamp', 'type', 'primary_symbol', 'signal_strength', 'confidence']].head(20),
        use_container_width=True
    )

def display_latency_analysis(latency_df):
    """Display latency histogram"""
    if latency_df is None or latency_df.empty:
        st.warning("No latency data available.")
        return

    # Latency histogram
    fig = px.bar(
        latency_df,
        x='upper_bound_us',
        y='percentage',
        title="Latency Distribution (μs)",
        labels={'upper_bound_us': 'Latency Upper Bound (μs)', 'percentage': 'Percentage (%)'}
    )
    st.plotly_chart(fig, use_container_width=True)

    # Key latency metrics
    col1, col2, col3 = st.columns(3)

    # Calculate percentiles from histogram
    total_samples = latency_df['count'].sum()
    if total_samples > 0:
        cumulative = 0
        p50 = p95 = p99 = 0

        for _, row in latency_df.iterrows():
            cumulative += row['count']
            percentile = cumulative / total_samples * 100

            if percentile >= 50 and p50 == 0:
                p50 = row['upper_bound_us']
            if percentile >= 95 and p95 == 0:
                p95 = row['upper_bound_us']
            if percentile >= 99 and p99 == 0:
                p99 = row['upper_bound_us']

        with col1:
            st.metric("P50 Latency", f"{p50} μs")
        with col2:
            st.metric("P95 Latency", f"{p95} μs")
        with col3:
            st.metric("P99 Latency", f"{p99} μs")

def display_performance_analysis(metrics, config):
    """Display performance analysis"""

    # Performance comparison chart
    theoretical_max = config['tick_rate'] * len(config['symbols'])
    actual_rate = metrics.get('average_rate', 0)
    efficiency = (actual_rate / theoretical_max * 100) if theoretical_max > 0 else 0

    fig = go.Figure(go.Bar(
        x=['Theoretical Max', 'Actual Throughput'],
        y=[theoretical_max, actual_rate],
        marker_color=['lightblue', 'darkblue']
    ))
    fig.update_layout(
        title="Throughput Performance",
        yaxis_title="Ticks Per Second",
        showlegend=False
    )
    st.plotly_chart(fig, use_container_width=True)

    # Configuration summary
    st.subheader("⚙️ Configuration Used")
    config_df = pd.DataFrame([
        {"Parameter": "Duration", "Value": f"{config['duration']} seconds"},
        {"Parameter": "Symbols", "Value": ", ".join(config['symbols'])},
        {"Parameter": "Tick Rate", "Value": f"{config['tick_rate']} Hz"},
        {"Parameter": "Z-Score Threshold", "Value": config['zscore_threshold']},
        {"Parameter": "Efficiency", "Value": f"{efficiency:.1f}%"}
    ])
    st.dataframe(config_df, use_container_width=True, hide_index=True)

if __name__ == "__main__":
    main()