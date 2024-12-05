#pragma once
#include "config_values.hpp"

// #ifdef __cplusplus
// extern "C"
// {
// #endif

typedef int16_t value_t;
typedef uint32_t count_t;
typedef uint32_t sum_t;

class running_stats {
 public:
  value_t threshold = 0;
  sum_t sum = 0;
  count_t count_above_threshold = 0;
  count_t count_below_threshold = 0;
  float iir_filter_value = -1;

  inline void add_value(value_t v) {
    sum += v;
    if (v > threshold) {
      count_above_threshold++;
    } else {
      count_below_threshold++;
    }
    if (iir_filter_value < 0) {
      iir_filter_value = (float)v;
    } else {
      iir_filter_value = iir_filter_b * iir_filter_value + (1.0 - iir_filter_b) * ((float)v);
    }
  }

  inline count_t get_total_count() const { return count_above_threshold + count_below_threshold; }

  inline value_t get_mean() const { return sum / (count_above_threshold + count_below_threshold); }

  inline float get_mean_float() const { return (float)sum / (float)(count_above_threshold + count_below_threshold); }

  inline float get_iir_filtered_value() const { return iir_filter_value; }

  inline bool is_above_threshold() const { return count_above_threshold >= count_below_threshold; }
  inline bool median_is_above_threshold() const { return count_above_threshold >= count_below_threshold; }
  inline bool avg_is_above_threshold() const { return get_mean_float() >= threshold; }
  inline bool iir_is_above_threshold() const { return get_iir_filtered_value() >= threshold; }

  // TODO reorganize
  inline bool median_is_above_threshold_hysteresis(bool currently_active, float hysteresis) const {
    return count_above_threshold >= count_below_threshold;
    // TODO does this work? probably not
    // if (currently_active) {
    //   return count_above_threshold >= (count_below_threshold - hysteresis);
    // } else {
    //   return count_above_threshold >= count_below_threshold;
    // }
  }
  inline bool avg_is_above_threshold_hysteresis(bool currently_active, float hysteresis) const {
    if (currently_active) {
      return get_mean_float() >= (threshold - hysteresis);
    } else {
      return get_mean_float() >= threshold;
    }
  }
  inline bool iir_is_above_threshold_hysteresis(bool currently_active, float hysteresis) const {
    if (currently_active) {
      return get_iir_filtered_value() >= (threshold - hysteresis);
    } else {
      return get_iir_filtered_value() >= threshold;
    }
  }

  inline void reset() { *this = running_stats(); }
};

// #ifdef __cplusplus
// }
// #endif
