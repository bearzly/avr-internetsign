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

void write_pixels(uint8_t address, uint8_t data);
void clear_display();
void update_buffer(const char* msg, uint8_t* buffer, int16_t idx);
void write_buffer(const uint8_t* buffer);
void initialize_sign(uint8_t brightness);
void write_commmand(uint8_t command);

#endif /* SIGN_H_ */