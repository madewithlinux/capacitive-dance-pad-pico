#pragma once
#include <cmath>
#include "config_values.hpp"

#pragma region f32 filters

/// @brief exponential moving average, AKA first-order IIR filter
struct ema_filter_f32 {
  float alpha;
  float value = 0;

  inline ema_filter_f32(float alpha) : alpha(alpha) {}

  inline void set_alpha(float a) { alpha = a; }
  inline float get_current_value() { return value; }
  inline float update(float next_value) {
    value = alpha * next_value + (1 - alpha) * value;
    return value;
  }
};

/// @brief exponential hull moving average
class ehma_filter_f32 {
  ema_filter_f32 filter_a;
  ema_filter_f32 filter_a_2;
  ema_filter_f32 filter_a_sqrt_2;

 public:
  inline ehma_filter_f32(float alpha) : filter_a(alpha), filter_a_2(alpha / 2), filter_a_sqrt_2(std::sqrt(alpha)) {}

  inline void set_alpha(float alpha) {
    filter_a.alpha = alpha;
    filter_a_2.alpha = alpha / 2;
    filter_a_sqrt_2.alpha = std::sqrt(alpha);
  }

  inline float get_current_value() { return filter_a_sqrt_2.get_current_value(); }
  inline float update(float next_value) {
    float raw_hma = 2 * filter_a_2.update(next_value) - filter_a.update(next_value);
    return filter_a_sqrt_2.update(raw_hma);
  }
};

struct normalized_sensor_data_filter {
  // copies of config values, for change detection
  float _iir_filter_b = iir_filter_b;
  float press_threshold = 100;
  float release_threshold = 80;
  // filters
  ehma_filter_f32 normalized_data_filter = ehma_filter_f32(iir_filter_b);
  // state
  bool is_pressed = false;

  inline float get_current_value() { return normalized_data_filter.get_current_value(); }
  inline bool update(uint16_t normalized_value) {
    // return true;
    if (iir_filter_b != _iir_filter_b) {
      // normalized_data_filter.alpha = iir_filter_b;
      normalized_data_filter.set_alpha(iir_filter_b);
      _iir_filter_b = iir_filter_b;
    }

    float v = normalized_data_filter.update(normalized_value);
    if (!is_pressed && v > press_threshold) {
      is_pressed = true;
    } else if (is_pressed && v < release_threshold) {
      is_pressed = false;
    }
    return is_pressed;
  }
};

#pragma endregion

#pragma region uint16 filters

constexpr size_t kWindowSize = 256;
using std::min;
using std::sqrt;

// Calculates the Weighted Moving Average for a given period size.
// Values provided to this class should fall in [−32,768, 32,767] otherwise it
// may overflow. We use a 32-bit integer for the intermediate sums which we
// then restrict back down to 16-bits.
class WeightedMovingAverage {
 public:
  inline WeightedMovingAverage(size_t size)
      : size_(min(size, kWindowSize)), cur_sum_(0), cur_weighted_sum_(0), values_{}, cur_count_(0) {}

  inline int16_t GetAverage(int16_t value) {
    // Add current value and remove oldest value.
    // e.g. with value = 5 and cur_count_ = 0
    // [4, 3, 2, 1] -> 10 becomes 10 + 5 - 4 = 11 -> [5, 3, 2, 1]
    int32_t next_sum = cur_sum_ + value - values_[cur_count_];
    // Update weighted sum giving most weight to the newest value.
    // [1*4, 2*3, 3*2, 4*1] -> 20 becomes 20 + 4*5 - 10 = 30
    //     -> [4*5, 1*3, 2*2, 3*1]
    // Subtracting by cur_sum_ is the same as removing 1 from each of the weight
    // coefficients.
    int32_t next_weighted_sum = cur_weighted_sum_ + size_ * value - cur_sum_;
    cur_sum_ = next_sum;
    cur_weighted_sum_ = next_weighted_sum;
    values_[cur_count_] = value;
    cur_count_ = (cur_count_ + 1) % size_;
    // Integer division is fine here since both the numerator and denominator
    // are integers and we need to return an int anyways. Off by one isn't
    // substantial here.
    // Sum of weights = sum of all integers from [1, size_]
    int16_t sum_weights = ((size_ * (size_ + 1)) / 2);
    return next_weighted_sum / sum_weights;
  }

  inline size_t GetSize() { return size_; }

  // Delete default constructor. Size MUST be explicitly specified.
  WeightedMovingAverage() = delete;

 private:
  size_t size_;
  int32_t cur_sum_;
  int32_t cur_weighted_sum_;
  // Keep track of all values we have in a circular array.
  int16_t values_[kWindowSize];
  size_t cur_count_;
};

// Calculates the Hull Moving Average. This is one of the better smoothing
// algorithms that will smooth the input values without wildly distorting the
// input values while still being responsive to input changes.
//
// The algorithm is essentially:
//   1. Calculate WMA of input values with a period of n/2 and double it.
//   2. Calculate WMA of input values with a period of n and subtract it from
//      step 1.
//   3. Calculate WMA of the values from step 2 with a period of sqrt(2).
//
// HMA = WMA( 2 * WMA(input, n/2) - WMA(input, n), sqrt(n) )
class HullMovingAverage {
 public:
  inline HullMovingAverage(size_t size) : wma1_(size / 2), wma2_(size), hull_(sqrt(size)) {}

  inline int16_t GetAverage(int16_t value) {
    int16_t wma1_value = wma1_.GetAverage(value);
    int16_t wma2_value = wma2_.GetAverage(value);
    int16_t hull_value = hull_.GetAverage(2 * wma1_value - wma2_value);

    return hull_value;
  }

  // Delete default constructor. Size MUST be explicitly specified.
  HullMovingAverage() = delete;

 private:
  WeightedMovingAverage wma1_;
  WeightedMovingAverage wma2_;
  WeightedMovingAverage hull_;
};

struct sensor_data_filter {
  // copies of config values, for change detection
  uint16_t press_threshold = 100;
  uint16_t release_threshold = 80;
  uint16_t window_size = cfg_hma_window_size;
  // filters
  HullMovingAverage hma_filter = HullMovingAverage(window_size);
  // state
  uint16_t current_value = 0;
  bool is_pressed = false;
  // parameters
  uint8_t sensor_index;
  uint16_t baseline_value;

  inline sensor_data_filter() : sensor_index(0), baseline_value(0) {}
  inline sensor_data_filter(uint8_t sensor_index, uint16_t baseline_value)
      : sensor_index(sensor_index), baseline_value(baseline_value) {}

  inline void update_from_config() {
    press_threshold = baseline_value + threshold_value;
    // TODO: configurable hysteresis
    release_threshold = baseline_value + threshold_value - 20;
    if (cfg_hma_window_size != window_size) {
      hma_filter = HullMovingAverage(cfg_hma_window_size);
      window_size = cfg_hma_window_size;
    }
  }

  inline float get_current_value() {
    // convert to float first in case result is negative
    return ((float)current_value) - ((float)baseline_value);
  }

  inline bool update(uint16_t raw_value) {
    update_from_config();
    current_value = hma_filter.GetAverage(raw_value);
    if (!is_pressed && current_value > press_threshold) {
      is_pressed = true;
    } else if (is_pressed && current_value < release_threshold) {
      is_pressed = false;
    }
    return is_pressed;
  }
};

#pragma endregion