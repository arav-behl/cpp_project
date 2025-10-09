# 📊 Streamlit Dashboard for C++ Trading System

This interactive dashboard visualizes the performance and signals from the C++ Real-Time Trading System.

## 🚀 Features

- **Live Performance Metrics**: View throughput, latency percentiles (P50/P95/P99), and signal counts
- **Interactive Charts**:
  - Latency distribution histogram
  - Signal type breakdown (pie chart)
  - Signal strength distributions
  - Signals over time timeline
  - Top active symbol pairs
- **Filtering Controls**: Filter by signal type, symbol, and time range
- **Recent Signals Table**: Browse the 100 most recent trading signals
- **Technical Details**: Expand to see implementation architecture

## 📦 Requirements

```bash
pip install -r requirements.txt
```

Dependencies:
- streamlit >= 1.28.0
- pandas >= 2.0.0
- plotly >= 5.15.0
- numpy >= 1.24.0

## 🎯 Usage

### Option 1: View on Streamlit Cloud (Live Demo)

Visit the deployed dashboard at: **[Your Streamlit Cloud URL]**

### Option 2: Run Locally

1. **Build the C++ system** (if not already built):
   ```bash
   cmake -S . -B build && cmake --build build -j8
   ```

2. **Generate data** by running the C++ demo:
   ```bash
   ./build/demo_realtime --duration 30 --rate 2000
   ```

   This creates CSV files in `data/`:
   - `signals.csv` - All detected trading signals
   - `latency_histogram.csv` - Latency distribution data

3. **Launch the dashboard**:
   ```bash
   streamlit run streamlit_demo.py
   ```

4. **Open your browser** to `http://localhost:8501`

## 📁 Data Files

The dashboard expects these CSV files in the `data/` directory:

### `signals.csv`
Columns:
- `timestamp` - Unix timestamp in microseconds
- `signal_id` - Unique signal identifier
- `type` - Signal type (ZBreak, CorrBreak, VolSpike)
- `primary_symbol` - Main symbol (e.g., AAPL)
- `secondary_symbol` - Paired symbol for correlation signals (e.g., MSFT)
- `signal_strength` - Strength/magnitude of signal
- `confidence` - Confidence score (0-1)
- `latency_us` - Processing latency in microseconds

### `latency_histogram.csv`
Columns:
- `lower_bound_us` - Bucket lower bound in microseconds
- `upper_bound_us` - Bucket upper bound in microseconds
- `count` - Number of samples in this bucket
- `percentage` - Percentage of total samples

## 🎨 Dashboard Sections

### 1. Performance Metrics (Top Row)
- Total signals detected
- Average latency
- P99 latency
- Active symbol pairs

### 2. Latency Distribution
- Interactive histogram showing latency buckets
- Percentile breakdown (P50, P75, P90, P95, P99)

### 3. Signal Analysis
- Pie chart of signal type distribution
- Histogram of signal strengths

### 4. Signals Over Time
- Timeline showing signal frequency across the run duration

### 5. Symbol Pair Analysis
- Bar chart of most active correlation pairs

### 6. Recent Signals Table
- Interactive table of the 100 most recent signals
- Sortable and filterable

## 🎛️ Sidebar Controls

- **Signal Type Filter**: Show all signals or filter by type (ZBreak, CorrBreak, VolSpike)
- **Symbol Filter**: Focus on specific symbols
- **Time Range Slider**: Narrow down to specific time windows

## 🔧 Customization

To customize the dashboard, edit `streamlit_demo.py`:

- **Colors**: Modify the color schemes in plotly chart definitions
- **Chart Types**: Replace plotly charts with alternative visualizations
- **Metrics**: Add new KPIs in the metrics section
- **Filters**: Add additional filtering dimensions

## 📸 Screenshots

[Add screenshots of your dashboard here]

## 🐛 Troubleshooting

**No data showing:**
- Ensure `data/signals.csv` and `data/latency_histogram.csv` exist
- Run the C++ demo first to generate data
- Check file permissions

**Streamlit crashes:**
- Check Python version (3.8+ required)
- Verify all dependencies are installed: `pip install -r requirements.txt`

**Large CSV files:**
- The signals.csv can be very large for long runs
- Consider filtering data or sampling in the loading function
- For demo purposes, use shorter run durations (10-30 seconds)

## 📊 Performance Tips

For large datasets (>1M signals):
- Use `nrows` parameter in `pd.read_csv()` to limit rows
- Implement data sampling for visualization
- Consider using Parquet format instead of CSV
- Add data caching with `@st.cache_data`

## 🌐 Deployment to Streamlit Cloud

1. Push your code to GitHub
2. Visit [share.streamlit.io](https://share.streamlit.io)
3. Connect your repository
4. Set main file to `streamlit_demo.py`
5. Ensure `requirements.txt` is in the root directory
6. Deploy!

**Note**: Streamlit Cloud cannot compile/run C++ code. Upload pre-generated CSV files to the repository for cloud demos.

## 📝 License

MIT License - See main project README
