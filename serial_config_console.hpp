#pragma once
#include <string>



void serial_console_task(void);

// config values:
extern float threshold_factor;
extern uint64_t threshold_sampling_duration_us;
extern uint64_t sampling_duration_us;
extern uint64_t serial_teleplot_report_interval_us;
// #define SERIAL_TELEPLOT_REPORT_INTERVAL_US (serial_teleplot_report_interval_us)
