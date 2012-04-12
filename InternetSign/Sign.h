/*
 * Sign.h
 *
 * Created: 2/13/2012 8:14:05 PM
 *  Author: Benjamin
 */ 


#ifndef SIGN_H_
#define SIGN_H_

#define MAX_BRIGHTNESS 16
#define MAX_SPEED 10

typedef enum {
    MESSAGE,
	CONFIG,
} Mode;

void next_frame();
void set_message(const char* msg);
void save_message();
char* get_message();
int calc_extent(const char* msg);
void set_speed(uint8_t speed);
void update_buffer(const char* msg, uint8_t *buffer);
void write_buffer(const uint8_t *buffer);
void write_pixels(uint8_t address, uint8_t data);
void clear_display();
void initialize_sign();
void write_command(uint8_t command);
void set_brightness(uint8_t brightness);
void set_mode(Mode m);
Mode get_mode();

#endif /* SIGN_H_ */