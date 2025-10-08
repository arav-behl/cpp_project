#!/usr/bin/env python3
"""
Streamlit Dashboard for C++ Real-Time Trading System
Perfect for showcasing to recruiters with LIVE terminal output!
"""

import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import subprocess
import threading
import queue
import time
from pathlib import Path

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
        margin: 0.2rem;
    }
    .terminal-output {
        background-color: #1e1e1e;
        color: #00ff00;
        padding: 1rem;
        border-radius: 5px;
        font-family: 'Courier New', monospace;
        font-size: 0.85rem;
        max-height: 400px;
        overflow-y: auto;
        white-space: pre-wrap;
    }
    .live-indicator {
        display: inline-block;
        width: 10px;
        height: 10px;
        background-color: #ff0000;
        border-radius: 50%;
        animation: blink 1s infinite;
        margin-right: 5px;
    }
    @keyframes blink {
        0%, 50% { opacity: 1; }
        51%, 100% { opacity: 0; }
    }
</style>
""", unsafe_allow_html=True)

def stream_process_output(process, output_queue, stop_event):
    """Stream output from subprocess in real-time"""
    try:
        for line in iter(process.stdout.readline, ''):
            if stop_event.is_set():
                break
            output_queue.put(line)
            if process.poll() is not None:
                break
    except Exception as e:
        output_queue.put(f"Error: {str(e)}\n")

def run_cpp_simulation(duration, tick_rate, zscore_threshold):
    """Run C++ simulation with live output streaming"""

    executable = Path("build/demo_realtime")
    data_dir = Path("data")
    data_dir.mkdir(exist_ok=True)

    # Clean old data files
    for file in data_dir.glob("*.csv"):
        file.unlink()

    # Build command
    cmd = [
        str(executable),
        "--duration", str(duration),
        "--rate", str(tick_rate),
        "--zscore", str(zscore_threshold)
    ]

    # Create queue for output
    output_queue = queue.Queue()
    stop_event = threading.Event()

    # Start process
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        universal_newlines=True
    )

    # Start output streaming thread
    output_thread = threading.Thread(
        target=stream_process_output,
        args=(process, output_queue, stop_event)
    )
    output_thread.daemon = True
    output_thread.start()

    return process, output_queue, stop_event, output_thread

def load_results():
    """Load CSV results after simulation"""
    data_dir = Path("data")

    signals_df = None
    latency_df = None

    signals_file = data_dir / "signals.csv"
    if signals_file.exists():
        try:
            signals_df = pd.read_csv(signals_file)
        except:
            pass

    latency_file = data_dir / "latency_histogram.csv"
    if latency_file.exists():
        try:
            latency_df = pd.read_csv(latency_file)
        except:
            pass

    return signals_df, latency_df

def parse_metrics_from_output(output_text):
    """Extract metrics from terminal output"""
    metrics = {}
    lines = output_text.split('\n')

    for line in lines:
        if "Total Ticks Processed:" in line:
            try:
                metrics['total_ticks'] = int(line.split(':')[1].strip())
            except:
                pass
        elif "Total Signals:" in line:
            try:
                metrics['total_signals'] = int(line.split(':')[1].strip())
            except:
                pass
        elif "Average Rate:" in line:
            try:
                rate_str = line.split(':')[1].strip().split()[0]
                metrics['average_rate'] = float(rate_str)
            except:
                pass
        elif "Queue Drop Rate:" in line:
            try:
                drop_str = line.split(':')[1].strip().replace("%", "")
                metrics['drop_rate'] = float(drop_str)
            except:
                pass

    return metrics

def main():
    # Title and introduction
    st.markdown('<h1 class="main-header">🚀 C++ Real-Time Trading System</h1>', unsafe_allow_html=True)

    st.markdown("""
    <div style="text-align: center; margin-bottom: 2rem;">
        <span class="tech-badge">C++20</span>
        <span class="tech-badge">Lock-Free Queues</span>
        <span class="tech-badge">Real-Time Analytics</span>
        <span class="tech-badge">Sub-μs Latency</span>
        <span class="tech-badge">Welford's Algorithm</span>
    </div>
    """, unsafe_allow_html=True)

    # Sidebar for configuration
    st.sidebar.header("🎛️ Simulation Parameters")

    duration = st.sidebar.slider("Duration (seconds)", 5, 60, 15)
    tick_rate = st.sidebar.selectbox("Tick Rate (Hz)", [100, 500, 1000, 2000, 5000], index=2)
    zscore_threshold = st.sidebar.slider("Z-Score Threshold", 1.0, 5.0, 2.5, 0.1)

    # System info
    st.sidebar.header("🔧 System Information")
    executable = Path("build/demo_realtime")
    if executable.exists():
        st.sidebar.success("✅ C++ Engine Ready")
        st.sidebar.code(f"{executable}", language="bash")
    else:
        st.sidebar.error("❌ C++ Engine Not Found")
        st.sidebar.text("Run: cmake -S . -B build && cmake --build build")
        return

    # Main content area
    col1, col2 = st.columns([2, 1])

    with col1:
        st.header("🎯 What This Demonstrates")
        st.markdown("""
        **Production-grade C++ skills for quantitative finance:**

        - 🔒 **Lock-Free Concurrency**: SPSC queue with acquire/release memory ordering
        - 📊 **Streaming Statistics**: Numerically stable Welford's algorithm
        - ⚡ **Ultra-Low Latency**: Sub-microsecond tick-to-signal processing
        - 🧮 **Quant Models**: Z-score, correlation, mean reversion
        - 📈 **Real-Time Analytics**: Live performance monitoring
        """)

    with col2:
        st.header("🏗️ Architecture")
        st.code("""
┌─────────────────┐
│ Feed Simulator  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ SPSC Queue 64K  │ ← Lock-free
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Signal Engine   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Latency Monitor │
└─────────────────┘
        """, language="text")

    # The main demo section
    st.header("🚀 Live Demo")

    # Initialize session state
    if 'running' not in st.session_state:
        st.session_state.running = False
        st.session_state.process = None
        st.session_state.output_queue = None
        st.session_state.stop_event = None

    # Run button
    col_btn1, col_btn2 = st.columns([1, 5])

    with col_btn1:
        if not st.session_state.running:
            if st.button("🎬 Run Demo", type="primary", use_container_width=True):
                st.session_state.running = True
                process, output_queue, stop_event, thread = run_cpp_simulation(
                    duration, tick_rate, zscore_threshold
                )
                st.session_state.process = process
                st.session_state.output_queue = output_queue
                st.session_state.stop_event = stop_event
                st.rerun()
        else:
            if st.button("⏹️ Stop", type="secondary", use_container_width=True):
                if st.session_state.stop_event:
                    st.session_state.stop_event.set()
                if st.session_state.process:
                    st.session_state.process.terminate()
                st.session_state.running = False
                st.rerun()

    with col_btn2:
        if st.session_state.running:
            st.markdown(
                '<div style="padding: 0.5rem; background-color: #ff4b4b; color: white; '
                'border-radius: 5px; text-align: center;">'
                '<span class="live-indicator"></span><b>LIVE - C++ Engine Running</b></div>',
                unsafe_allow_html=True
            )

    # Live output section
    if st.session_state.running:
        st.subheader("📟 Live Terminal Output")

        output_placeholder = st.empty()
        metrics_placeholder = st.empty()

        output_text = ""
        start_time = time.time()

        while st.session_state.running:
            # Check if process is still running
            if st.session_state.process.poll() is not None:
                st.session_state.running = False
                break

            # Get new output lines
            try:
                while not st.session_state.output_queue.empty():
                    line = st.session_state.output_queue.get_nowait()
                    output_text += line
            except:
                pass

            # Display live output in terminal-style
            with output_placeholder.container():
                st.markdown(
                    f'<div class="terminal-output">{output_text}</div>',
                    unsafe_allow_html=True
                )

            # Show elapsed time
            elapsed = time.time() - start_time
            with metrics_placeholder.container():
                col1, col2, col3 = st.columns(3)
                col1.metric("⏱️ Elapsed", f"{elapsed:.1f}s")
                col2.metric("🎯 Duration", f"{duration}s")
                col3.metric("📊 Progress", f"{min(100, elapsed/duration*100):.0f}%")

            # Update every 100ms
            time.sleep(0.1)

        # Simulation complete
        if not st.session_state.running and st.session_state.process:
            st.success("✅ Simulation Complete!")

            # Wait a moment for files to be written
            time.sleep(1)

            # Parse final metrics
            metrics = parse_metrics_from_output(output_text)

            # Load CSV results
            signals_df, latency_df = load_results()

            # Display results
            display_results(metrics, signals_df, latency_df, output_text)

            # Reset state
            st.session_state.process = None
            st.session_state.output_queue = None
            st.session_state.stop_event = None

def display_results(metrics, signals_df, latency_df, raw_output):
    """Display simulation results with beautiful charts"""

    # Performance metrics
    st.header("📊 Performance Metrics")

    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.metric(
            "Ticks Processed",
            f"{metrics.get('total_ticks', 0):,}",
            delta="Completed"
        )

    with col2:
        st.metric(
            "Signals Generated",
            f"{metrics.get('total_signals', 0):,}"
        )

    with col3:
        st.metric(
            "Throughput",
            f"{metrics.get('average_rate', 0):,.0f} TPS"
        )

    with col4:
        drop_rate = metrics.get('drop_rate', 0)
        st.metric(
            "Queue Efficiency",
            f"{100 - drop_rate:.2f}%"
        )

    # Charts section
    st.header("📈 Analytics Dashboard")

    # Create tabs for different views
    tab1, tab2, tab3 = st.tabs(["🎯 Signals", "⚡ Latency", "📟 Terminal Output"])

    with tab1:
        display_signals_analysis(signals_df)

    with tab2:
        display_latency_analysis(latency_df)

    with tab3:
        st.markdown(
            f'<div class="terminal-output" style="max-height: 600px;">{raw_output}</div>',
            unsafe_allow_html=True
        )

def display_signals_analysis(signals_df):
    """Display signals analysis"""
    if signals_df is None or signals_df.empty:
        st.warning("⚠️ No signals generated. Try lowering the Z-score threshold.")
        return

    col1, col2 = st.columns(2)

    with col1:
        # Signal types distribution
        signal_counts = signals_df['type'].value_counts()
        fig = px.pie(
            values=signal_counts.values,
            names=signal_counts.index,
            title="Signal Types Distribution",
            color_discrete_sequence=px.colors.qualitative.Set3
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
    st.subheader("🎯 Recent Signals")
    st.dataframe(
        signals_df[['timestamp', 'type', 'primary_symbol', 'signal_strength', 'confidence']].head(20),
        use_container_width=True
    )

def display_latency_analysis(latency_df):
    """Display latency histogram"""
    if latency_df is None or latency_df.empty:
        st.warning("⚠️ No latency data available.")
        return

    # Latency histogram
    fig = px.bar(
        latency_df,
        x='upper_bound_us',
        y='percentage',
        title="Latency Distribution (microseconds)",
        labels={'upper_bound_us': 'Latency Upper Bound (μs)', 'percentage': 'Percentage (%)'},
        color='percentage',
        color_continuous_scale='Blues'
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
            st.metric("P50 Latency", f"{p50} μs", delta="Median")
        with col2:
            st.metric("P95 Latency", f"{p95} μs", delta="95th %ile")
        with col3:
            st.metric("P99 Latency", f"{p99} μs", delta="99th %ile")

if __name__ == "__main__":
    main()
