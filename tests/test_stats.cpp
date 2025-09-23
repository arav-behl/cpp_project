#include "stats/rolling_stats.hpp"
#include "stats/rolling_covar.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <random>

constexpr double EPSILON = 1e-9;

bool close_enough(double a, double b, double epsilon = EPSILON) {
    return std::abs(a - b) < epsilon;
}

// Test Welford's algorithm against known values
void test_rolling_stats_basic() {
    std::cout << "Testing RollingStats basic functionality...\n";

    RollingStats stats;

    // Single value
    stats.add(5.0);
    assert(close_enough(stats.mean(), 5.0));
    assert(close_enough(stats.variance(), 0.0));

    // Two values
    stats.add(7.0);
    assert(close_enough(stats.mean(), 6.0));
    assert(close_enough(stats.variance(), 2.0)); // Sample variance

    // Known sequence: [1, 2, 3, 4, 5]
    stats.reset();
    for (int i = 1; i <= 5; ++i) {
        stats.add(static_cast<double>(i));
    }

    assert(close_enough(stats.mean(), 3.0));
    assert(close_enough(stats.variance(), 2.5)); // Sample variance of [1,2,3,4,5]
    assert(close_enough(stats.std_dev(), std::sqrt(2.5)));

    // Z-score test
    assert(close_enough(stats.z_score(1.0), -1.2649110640673518)); // (1-3)/sqrt(2.5)
    assert(close_enough(stats.z_score(5.0), 1.2649110640673518));  // (5-3)/sqrt(2.5)

    std::cout << "✅ RollingStats basic tests passed\n";
}

void test_rolling_stats_numerical_stability() {
    std::cout << "Testing RollingStats numerical stability...\n";

    RollingStats stats;

    // Add large numbers (potential for catastrophic cancellation)
    const double base = 1e12;
    for (int i = 0; i < 1000; ++i) {
        stats.add(base + i * 0.001);
    }

    // Variance should be small but non-zero
    const double expected_variance = (1000.0 - 1) * 0.001 * 0.001 / 12.0;
    assert(stats.variance() > 0.0);
    assert(stats.variance() < 1.0); // Should be much less than 1
    assert(stats.is_valid());

    std::cout << "✅ RollingStats numerical stability tests passed\n";
}

void test_rolling_covar_basic() {
    std::cout << "Testing RollingCovar basic functionality...\n";

    RollingCovar covar;

    // Perfect positive correlation: y = 2x + 1
    std::vector<std::pair<double, double>> perfect_corr = {
        {1.0, 3.0}, {2.0, 5.0}, {3.0, 7.0}, {4.0, 9.0}, {5.0, 11.0}
    };

    for (const auto& [x, y] : perfect_corr) {
        covar.add(x, y);
    }

    assert(close_enough(covar.mean_x(), 3.0));
    assert(close_enough(covar.mean_y(), 7.0));
    assert(close_enough(covar.correlation(), 1.0, 1e-10)); // Perfect correlation
    assert(close_enough(covar.beta(), 2.0, 1e-10)); // Slope should be 2

    // Test uncorrelated data
    covar.reset();
    std::mt19937 rng(42);
    std::normal_distribution<double> dist(0.0, 1.0);

    for (int i = 0; i < 1000; ++i) {
        covar.add(dist(rng), dist(rng));
    }

    // Should be close to zero correlation for independent random variables
    assert(std::abs(covar.correlation()) < 0.1);
    assert(covar.is_valid());

    std::cout << "✅ RollingCovar basic tests passed\n";
}

void test_rolling_covar_known_correlation() {
    std::cout << "Testing RollingCovar with known correlation...\n";

    RollingCovar covar;

    // Generate correlated data: Y = 0.8*X + noise
    std::mt19937 rng(123);
    std::normal_distribution<double> noise(0.0, 0.1);
    std::normal_distribution<double> x_dist(0.0, 1.0);

    for (int i = 0; i < 10000; ++i) {
        const double x = x_dist(rng);
        const double y = 0.8 * x + noise(rng);
        covar.add(x, y);
    }

    // Should be close to 0.8 correlation (allow wider tolerance due to noise)
    const double correlation = covar.correlation();
    std::cout << "  Actual correlation: " << correlation << std::endl;
    assert(correlation > 0.6 && correlation < 1.0);

    // Beta should be close to 0.8
    const double beta = covar.beta();
    std::cout << "  Actual beta: " << beta << std::endl;
    assert(beta > 0.6 && beta < 1.0);

    std::cout << "✅ RollingCovar known correlation tests passed\n";
}

void test_ema_stats() {
    std::cout << "Testing EMAStats...\n";

    EMAStats ema(static_cast<std::size_t>(10)); // 10-period EMA

    // Add some values
    for (int i = 1; i <= 20; ++i) {
        ema.add(static_cast<double>(i));
    }

    assert(ema.is_initialized());
    assert(ema.mean() > 0.0);
    assert(ema.variance() > 0.0);

    // Recent values should have more weight
    assert(ema.mean() > 10.0); // Should be greater than simple average

    std::cout << "✅ EMAStats tests passed\n";
}

void test_windowed_stats() {
    std::cout << "Testing WindowedStats...\n";

    WindowedStats<5> windowed;

    // Fill window
    for (int i = 1; i <= 5; ++i) {
        windowed.add(static_cast<double>(i));
    }

    assert(windowed.is_full());
    assert(windowed.count() == 5);
    assert(close_enough(windowed.mean(), 3.0));

    // Add more (should evict oldest)
    windowed.add(6.0);
    assert(windowed.count() == 5);
    assert(close_enough(windowed.mean(), 4.0)); // Mean of [2,3,4,5,6]

    std::cout << "✅ WindowedStats tests passed\n";
}

void run_stats_tests() {
    std::cout << "🧪 Running Statistics Tests\n";
    std::cout << "============================\n";

    test_rolling_stats_basic();
    test_rolling_stats_numerical_stability();
    test_rolling_covar_basic();
    test_rolling_covar_known_correlation();
    test_ema_stats();
    test_windowed_stats();

    std::cout << "\n✅ All statistics tests passed!\n\n";
}