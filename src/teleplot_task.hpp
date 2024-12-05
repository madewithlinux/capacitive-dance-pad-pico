#pragma once


void teleplot_task();

typedef int16_t sensor_data_send_buffer_value_type;
void write_to_sensor_data_send_buffer(sensor_data_send_buffer_value_type data);
