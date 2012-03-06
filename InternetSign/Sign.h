/*
 * Sign.h
 *
 * Created: 2/13/2012 8:14:05 PM
 *  Author: Benjamin
 */ 


#ifndef SIGN_H_
#define SIGN_H_

#define CHARH 7
#define CHARW 5
#define SIGNW 4 * 8

void next_frame();
void store_message();
char* get_message();
int calc_extent(const char* msg);
void set_speed(uint8_t speed);
void update_buffer(const char* msg, uint8_t *buffer, int16_t idx);
void write_buffer(const uint8_t *buffer);
void write_pixels(uint8_t address, uint8_t data);
void clear_display();
void initialize_sign();
void write_commmand(uint8_t command);
void set_brightness(uint8_t brightness);

#endif /* SIGN_H_ */