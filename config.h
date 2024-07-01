#pragma once

#ifndef FORMAT_BUFFER_SIZE
#define FORMAT_BUFFER_SIZE 128
#endif

#ifndef WRITE_SERIAL_LOGS
#define WRITE_SERIAL_LOGS 0
// #define WRITE_SERIAL_LOGS 1
#endif

#if WRITE_SERIAL_LOGS
#define IF_SERIAL_LOG(x) x
#else
#define IF_SERIAL_LOG(x)
#endif

constexpr bool log_extra_sensor_info = false;

#ifndef PLAYER_NUMBER
// #define PLAYER_NUMBER 1
#define PLAYER_NUMBER 2
#endif

#define SERIAL_TELEPLOT_REPORT_INTERVAL_US (20 * 1000)

// #define SERIAL_TELEPLOT 0
#define SERIAL_TELEPLOT 1
