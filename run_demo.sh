#!/bin/bash
set -e

echo "🚀 C++ Real-Time Trading System Demo Setup"
echo "=========================================="

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "📦 Building C++ system..."
    cmake -S . -B build
    cmake --build build -j8
    echo "✅ C++ system built successfully!"
else
    echo "✅ C++ system already built"
fi

# Check if Python dependencies are installed
echo "🐍 Checking Python dependencies..."
if ! python3 -c "import streamlit, pandas, plotly" 2>/dev/null; then
    echo "📦 Installing Python dependencies..."
    pip install -r requirements.txt
    echo "✅ Python dependencies installed!"
else
    echo "✅ Python dependencies already installed"
fi

# Create data directory
mkdir -p data

echo ""
echo "🎬 Starting Streamlit Dashboard..."
echo "   Open your browser to the URL shown below"
echo "   Click 'Run Trading System Demo' to see C++ in action!"
echo ""

# Launch Streamlit
streamlit run streamlit_dashboard.py