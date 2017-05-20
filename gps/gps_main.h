#ifndef GPS_MAIN_H__
#define GPS_MAIN_H__

#include <stdint.h>

void gps_on(void);
void gps_off(void);
void gps_buf_fill_data(uint8_t cr);

void gps_thread(void * arg);

#endif

