#pragma once

// config values:
extern float threshold_factor;
extern uint64_t threshold_sampling_duration_us;
extern uint64_t sampling_duration_us;
extern uint64_t serial_teleplot_report_interval_us;
extern bool usb_hid_enabled;
extern int filter_type;
extern float iir_filter_b;
// TODO: player1/player2 config option

#define FILTER_TYPE_MEDIAN 0
#define FILTER_TYPE_AVG 1
#define FILTER_TYPE_IIR 2

// TODO: make these config values

// #define teleplot_normalize_values (1)
#define teleplot_normalize_values (0)

// value subtracted from sampling_duration_us to account for processing time
#define sampling_buffer_time_us (100)
