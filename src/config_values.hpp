#pragma once

// config values:
extern float threshold_factor;
extern float threshold_value;
extern int threshold_type;
extern uint64_t threshold_sampling_duration_us;
extern uint64_t sampling_duration_us;
extern uint64_t serial_teleplot_report_interval_us;
extern bool teleplot_normalize_values;
extern bool usb_hid_enabled;
extern int filter_type;
extern float iir_filter_b;
// `sleep_us_between_samples` was added in an attempt to reduce signal noise. It did not work, and should be set to 0.
extern uint64_t sleep_us_between_samples;
// NOTE: this is us (microseconds), NOT ms (milliseconds)
extern uint64_t debounce_us;
// TODO: player1/player2 config option

#define FILTER_TYPE_MEDIAN 0
#define FILTER_TYPE_AVG 1
#define FILTER_TYPE_IIR 2

#define THRESHOLD_TYPE_FACTOR 0
#define THRESHOLD_TYPE_VALUE 1

// TODO: make these config values
// value subtracted from sampling_duration_us to account for processing time
#define sampling_buffer_time_us (100)
