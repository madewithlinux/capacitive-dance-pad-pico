#pragma once

// #define WRITE_SERIAL_LOGS 0
#define WRITE_SERIAL_LOGS 1

#if WRITE_SERIAL_LOGS
#define IF_SERIAL_LOG(x) x
#else
#define IF_SERIAL_LOG(x)
#endif


constexpr bool log_extra_sensor_info = false;
