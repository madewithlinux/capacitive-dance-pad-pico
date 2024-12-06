#pragma once
#include <cmath>
#include "config_values.hpp"
#include "teleplot_task.hpp"

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

/// adapted from https://github.com/teejusb/fsr/blob/master/fsr.ino

// Calculates the Weighted Moving Average for a given period size.
// Values provided to this class should fall in [0, 65,535] otherwise it
// may overflow. We use a 64-bit integer for the intermediate sums which we
// then restrict back down to 16-bits.
class WeightedMovingAverage {
 public:
  inline WeightedMovingAverage(size_t size)
      : size_(min(size, kWindowSize)), cur_sum_(0), cur_weighted_sum_(0), values_{}, cur_count_(0) {}

  inline uint16_t GetAverage(uint16_t value) {
    // Add current value and remove oldest value.
    // e.g. with value = 5 and cur_count_ = 0
    // [4, 3, 2, 1] -> 10 becomes 10 + 5 - 4 = 11 -> [5, 3, 2, 1]
    int64_t next_sum = cur_sum_ + value - values_[cur_count_];
    // Update weighted sum giving most weight to the newest value.
    // [1*4, 2*3, 3*2, 4*1] -> 20 becomes 20 + 4*5 - 10 = 30
    //     -> [4*5, 1*3, 2*2, 3*1]
    // Subtracting by cur_sum_ is the same as removing 1 from each of the weight
    // coefficients.
    int64_t next_weighted_sum = cur_weighted_sum_ + size_ * value - cur_sum_;
    cur_sum_ = next_sum;
    cur_weighted_sum_ = next_weighted_sum;
    values_[cur_count_] = value;
    cur_count_ = (cur_count_ + 1) % size_;
    // Integer division is fine here since both the numerator and denominator
    // are integers and we need to return an int anyways. Off by one isn't
    // substantial here.
    // Sum of weights = sum of all integers from [1, size_]
    uint16_t sum_weights = ((size_ * (size_ + 1)) / 2);
    return next_weighted_sum / sum_weights;
  }

  // Delete default constructor. Size MUST be explicitly specified.
  WeightedMovingAverage() = delete;

 private:
  size_t size_;
  int64_t cur_sum_;
  int64_t cur_weighted_sum_;
  // Keep track of all values we have in a circular array.
  uint16_t values_[kWindowSize];
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

  inline uint16_t GetAverage(uint16_t value) {
    uint16_t wma1_value = wma1_.GetAverage(value);
    uint16_t wma2_value = wma2_.GetAverage(value);
    uint16_t hull_value = hull_.GetAverage(2 * wma1_value - wma2_value);

    return hull_value;
  }

  // Delete default constructor. Size MUST be explicitly specified.
  HullMovingAverage() = delete;

 private:
  WeightedMovingAverage wma1_;
  WeightedMovingAverage wma2_;
  WeightedMovingAverage hull_;
};

// TODO: this is too noisy to be useful...
class derivative_filter_i16 {
  int16_t prev_value = 0;
  int16_t d_dv = 0;

 public:
  inline int16_t get_current_value() { return d_dv; }
  inline void update(int16_t next_value) {
    d_dv = next_value - prev_value;
    prev_value = next_value;
  }
};

class sensor_data_filter {
 private:
  // copies of config values, for change detection
  uint64_t _cfg_press_threshold = cfg_press_threshold;
  uint64_t _cfg_release_threshold = cfg_release_threshold;
  uint16_t _cfg_hma_window_size = cfg_hma_window_size;
  // filters
  HullMovingAverage hma_filter = HullMovingAverage(cfg_hma_window_size);
  // derivative_filter_i16 deriv1;
  // state
  uint16_t current_raw_value = 0;
  volatile bool is_pressed = false;
  uint16_t press_raw_threshold = UINT16_MAX;
  uint16_t release_raw_threshold = UINT16_MAX;
  bool should_update_config = true;
  // parameters
  uint8_t sensor_index;
  uint16_t baseline_value;

 public:
  // default constructor (only to satisfy compiler)
  inline sensor_data_filter() : sensor_index(0), baseline_value(0) {}
  // actual constructor to be used
  inline sensor_data_filter(uint8_t sensor_index, uint16_t baseline_value)
      : sensor_index(sensor_index), baseline_value(baseline_value) {
    update_from_config();
  }

  inline void set_should_update_config() { should_update_config = true; }

  inline float get_current_value_raw_f32() { return ((float)current_raw_value); }
  inline float get_current_value_norm_f32() {
    // convert to float first in case result is negative
    return ((float)current_raw_value) - ((float)baseline_value);
  }
  inline bool get_is_pressed() { return is_pressed; }

  inline bool update(uint16_t raw_value) {
    if (should_update_config) {
      update_from_config();
      should_update_config = false;
    }
    current_raw_value = hma_filter.GetAverage(raw_value);
    // if (sensor_index == 1) {
    //   write_to_sensor_data_send_buffer(current_raw_value);
    // }
    // deriv1.update(current_raw_value);

    if (!is_pressed && current_raw_value > press_raw_threshold) {
      is_pressed = true;
    } else if (is_pressed && current_raw_value < release_raw_threshold) {
      is_pressed = false;
    }
    return is_pressed;
  }

 private:
  inline void update_from_config() {
    press_raw_threshold = baseline_value + cfg_press_threshold;
    _cfg_press_threshold = cfg_press_threshold;
    release_raw_threshold = baseline_value + cfg_release_threshold;
    _cfg_release_threshold = cfg_release_threshold;
    if (cfg_hma_window_size != _cfg_hma_window_size) {
      hma_filter = HullMovingAverage(cfg_hma_window_size);
      _cfg_hma_window_size = cfg_hma_window_size;
    }
  }
};

#pragma endregion