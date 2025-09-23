#!/usr/bin/env python3
"""
Python wrapper for the C++ Real-Time Trading System
Provides easy interface for Streamlit dashboard
"""

import subprocess
import os
import pandas as pd
import time
import json
from pathlib import Path
from typing import Dict, Any, Optional

class TradingSystemWrapper:
    def __init__(self, build_dir: str = "build"):
        self.build_dir = Path(build_dir)
        self.executable = self.build_dir / "demo_realtime"
        self.data_dir = Path("data")

        # Ensure data directory exists
        self.data_dir.mkdir(exist_ok=True)

        # Check if executable exists
        if not self.executable.exists():
            raise FileNotFoundError(f"C++ executable not found at {self.executable}")

    def run_simulation(self,
                      duration: int = 15,
                      symbols: str = "AAPL,MSFT,GOOGL,TSLA",
                      tick_rate: int = 1000,
                      zscore_threshold: float = 2.5) -> Dict[str, Any]:
        """
        Run the C++ trading system simulation

        Args:
            duration: Simulation duration in seconds
            symbols: Comma-separated list of symbols
            tick_rate: Ticks per second
            zscore_threshold: Z-score threshold for signals

        Returns:
            Dictionary with simulation results
        """
        print(f"🚀 Running C++ Trading System...")
        print(f"   Duration: {duration}s")
        print(f"   Symbols: {symbols}")
        print(f"   Tick Rate: {tick_rate} Hz")
        print(f"   Z-Score Threshold: {zscore_threshold}")

        # Clean up old data files
        for file in self.data_dir.glob("*.csv"):
            file.unlink()

        # Build command
        cmd = [
            str(self.executable),
            "--duration", str(duration),
            "--rate", str(tick_rate),
            "--zscore", str(zscore_threshold)
        ]

        start_time = time.time()

        try:
            # Run the C++ executable
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=duration + 10  # Add buffer time
            )

            execution_time = time.time() - start_time

            if result.returncode != 0:
                raise RuntimeError(f"C++ simulation failed: {result.stderr}")

            # Parse the output for performance metrics
            output_lines = result.stdout.split('\n')
            metrics = self._parse_output(output_lines)

            # Load generated CSV files
            signals_df = self._load_signals()
            latency_df = self._load_latency_histogram()

            return {
                'success': True,
                'execution_time': execution_time,
                'metrics': metrics,
                'signals': signals_df,
                'latency_histogram': latency_df,
                'raw_output': result.stdout,
                'config': {
                    'duration': duration,
                    'symbols': symbols.split(','),
                    'tick_rate': tick_rate,
                    'zscore_threshold': zscore_threshold
                }
            }

        except subprocess.TimeoutExpired:
            return {
                'success': False,
                'error': 'Simulation timed out',
                'execution_time': time.time() - start_time
            }
        except Exception as e:
            return {
                'success': False,
                'error': str(e),
                'execution_time': time.time() - start_time
            }

    def _parse_output(self, lines) -> Dict[str, Any]:
        """Parse C++ output for key metrics"""
        metrics = {}

        for line in lines:
            if "Total Ticks Processed:" in line:
                metrics['total_ticks'] = int(line.split(':')[1].strip())
            elif "Total Signals:" in line:
                metrics['total_signals'] = int(line.split(':')[1].strip())
            elif "Average Rate:" in line:
                rate_str = line.split(':')[1].strip().replace(" TPS", "")
                metrics['average_rate'] = float(rate_str)
            elif "Queue Drop Rate:" in line:
                drop_str = line.split(':')[1].strip().replace("%", "")
                metrics['drop_rate'] = float(drop_str)

        return metrics

    def _load_signals(self) -> Optional[pd.DataFrame]:
        """Load signals CSV file"""
        signals_file = self.data_dir / "signals.csv"
        if signals_file.exists():
            return pd.read_csv(signals_file)
        return None

    def _load_latency_histogram(self) -> Optional[pd.DataFrame]:
        """Load latency histogram CSV file"""
        latency_file = self.data_dir / "latency_histogram.csv"
        if latency_file.exists():
            return pd.read_csv(latency_file)
        return None

    def get_system_info(self) -> Dict[str, str]:
        """Get system information"""
        return {
            'executable_path': str(self.executable),
            'data_directory': str(self.data_dir),
            'executable_exists': self.executable.exists(),
            'build_directory': str(self.build_dir)
        }

if __name__ == "__main__":
    # Quick test
    wrapper = TradingSystemWrapper()
    print("System Info:", wrapper.get_system_info())

    # Run a quick simulation
    result = wrapper.run_simulation(duration=5, tick_rate=500)
    print("\nSimulation Result:")
    print(f"Success: {result['success']}")
    if result['success']:
        print(f"Metrics: {result['metrics']}")
        print(f"Signals Generated: {len(result['signals']) if result['signals'] is not None else 0}")
    else:
        print(f"Error: {result.get('error', 'Unknown error')}")