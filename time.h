#ifndef __TIME_H__
#define __TIME_H__

uint8_t bcdencode(uint8_t a);
uint8_t bcddecode(uint8_t a);
uint8_t set_time_real(void);
char set_time(signed char *pin,signed char pini);

uint32_t get_timestamp_in_min(void);
void read_time(void);




#endif
