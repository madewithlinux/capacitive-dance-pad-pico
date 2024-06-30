#pragma once

#define WRITE_SERIAL_LOGS 0
// #define WRITE_SERIAL_LOGS 1

#if WRITE_SERIAL_LOGS
#define IF_SERIAL_LOG(x) x
#else
#define IF_SERIAL_LOG(x)
#endif


constexpr bool log_extra_sensor_info = false;

// #define PLAYER_NUMBER 1
#define PLAYER_NUMBER 2

#define SERIAL_TELEPLOT_REPORT_INTERVAL_US (10*1000)

#define SERIAL_TELEPLOT 0
// #define SERIAL_TELEPLOT 1
