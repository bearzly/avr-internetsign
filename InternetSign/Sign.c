/*
 * Sign.c
 *
 * Contains functions for interfacing with the LED display
 *
 * EE400/EE401 2011-2012
 * Group 38 - Internet Sign
 * Benjamin Gwin, James Powers, Karl Rath
 *
 * Created: 2/13/2012 8:14:16 PM
 * Copyright (c) 2012 Benjamin Gwin
 *
 * This software is free/open source under the terms of the
 * GPL v3. See http://www.gnu.org/licenses/gpl.html for details.
 */ 
#include <avr/io.h>
#include <avr/eeprom.h>

#include <string.h>
#include <stdio.h>

#include "Font.h"
#include "System.h"
#include "Sign.h"

#include <util/delay.h>

#define CHARW 5        // Maximum width of a character
#define SIGNW 4 * 8    // Total width of the sign

// Ports used for sign operation
#define SIGN_CS_PORT PORTB
#define SIGN_CS_DDR  DDRB
#define SIGN_CS      PB0
#define SIGN_DATA_PORT PORTD
#define SIGN_DATA_DDR  DDRD
#define SIGN_SCK       PD7
#define SIGN_DATA      PD6

// Prefixes used for sign interfacing
#define PREFIX_SIZE 3
#define CMD_PREFIX 0x04
#define WR_PREFIX 0x05

// Commands used for controlling the sign
#define CMD_SIZE 9
#define SYS_DIS        0x00
#define SYS_EN         0x01
#define LED_ON         0x03
#define LED_OFF        0x02
#define RC_MASTER_MODE 0x18
#define PWM_DUTY_4     0xA3
#define PWM_DUTY_8     0xA7
#define PWM_DUTY_16    0xAF
#define COM_OPTION     0x20

// Byte lengths for transmitting addresses or data
#define ADR_SIZE 7
#define DATA_SIZE 4

// Sign speed is controlled by these parameters
// New frames are created every SPEED_BASE+((MAX_SPEED - speed)*SPEED_MULTIPLIER) ticks
#define SPEED_BASE 70
#define SPEED_MULTIPLIER 20

// Helper to set the buffer value only if it has a valid index
#define SET_BUFFER(buffer, data, idx) if (0 <= idx && idx < SIGNW) buffer[idx] = data

static char g_message[MSG_LENGTH];      // The current message
static uint8_t g_frame_buffer[SIGNW];   // Contains the pixel data to be sent

static int g_cidx = 0;  // index of the current character being drawn
static int g_idx = 0;   // col index of the current character being drawn

Mode g_currentMode = MESSAGE;

// Changes the current message to the passed string
void set_message(const char* msg) {
	strcpy(g_message, msg);
}

// Causes the current message to be persisted to EEPROM
void save_message() {
	eeprom_update_block(g_message, (void *)MSG_ADDR, strlen(g_message) + 1);
}

// Returns a pointer to the message array
char* get_message() {
	return g_message;
}

// Advances the display to the next frame
void next_frame() {
	update_buffer(g_message, g_frame_buffer);
	write_buffer(g_frame_buffer);
}

// Calculates the total pixel width of a string by
// ignoring empty columns and treating spaces specially
int calc_extent(const char* str) {
	int length = 0;
	const int size = strlen(str);
	for (int i = 0; i < size; i++) {
		if (str[i] == ' ') {
			length += 2;
		} else {
			int j;
			for (j = 0; j < CHARW; j++) {
				if (pgm_read_byte(Font5x7 + (str[i] - 32) * 5 + j) == 0) {
					break;
				}
			}
			length += j + 1;
		}
	}
	return length;
}

// Sets and stores the current speed of the sign
void set_speed(uint8_t speed) {
	if (speed < 1) {
		speed = 1;
	} else if (speed > MAX_SPEED) {
		speed = MAX_SPEED;
	}
	OCR0A = SPEED_BASE + SPEED_MULTIPLIER * (MAX_SPEED - speed);
	eeprom_update_byte((uint8_t*)SPEED_ADDR, speed);
}

// Sets and saves the current brightness of the sign
void set_brightness(uint8_t brightness) {
	if (brightness < 1) {
		brightness = 1;
	} else if (brightness > MAX_BRIGHTNESS) {
		brightness = MAX_BRIGHTNESS;
	}
	write_command((PWM_DUTY_4 & 0xF0) + (brightness - 1));
	eeprom_update_byte((uint8_t *)BRGHT_ADDR, brightness);
}

// Initializes writing to the sign
void write_begin() {
    output_high(SIGN_CS_PORT, SIGN_CS);
    output_high(SIGN_DATA_PORT, SIGN_SCK);
    output_low(SIGN_CS_PORT, SIGN_CS);
}

// Signals that writing is complete
void write_end() {
    output_high(SIGN_CS_PORT, SIGN_CS);
}

// Writes 'bits' bits of data from 'data' to the sign,
// starting with the most significant bit
void write_data_msb(uint16_t data, uint8_t bits) {
    for (uint8_t i = bits; i > 0; i--) {
        if (data & (1 << (i - 1))) {
            output_high(SIGN_DATA_PORT, SIGN_DATA);
        } else {
            output_low(SIGN_DATA_PORT, SIGN_DATA);
        }
        output_low(SIGN_DATA_PORT, SIGN_SCK);
		output_high(SIGN_DATA_PORT, SIGN_SCK);
    }
}

// Writes 'bits' bits of data from 'data' to the sign,
// starting with the least significant bit
void write_data_lsb(uint16_t data, uint8_t bits) {
    for (uint8_t i = 0; i < bits; i++) {
        if (data & (1 << i)) {
            output_high(SIGN_DATA_PORT, SIGN_DATA);
        } else {
            output_low(SIGN_DATA_PORT, SIGN_DATA);
        }
        output_low(SIGN_DATA_PORT, SIGN_SCK);
		output_high(SIGN_DATA_PORT, SIGN_SCK);
    }
}

// Sends a given command to the sign, as defined above
// See the data sheet for more commands
void write_command(uint8_t command) {
    write_begin();
    write_data_msb(CMD_PREFIX, PREFIX_SIZE);
    write_data_msb(command << 1, CMD_SIZE);
    write_end();
}

// Writes a single 4 bit section of pixel data to the
// given address
void write_pixels(uint8_t address, uint8_t data) {
    write_begin();
    write_data_msb(WR_PREFIX, PREFIX_SIZE);
    write_data_msb(address, ADR_SIZE);
    write_data_lsb(data, DATA_SIZE);
    write_end();
}

// Draws a single character in the given column
void write_char(char c, uint8_t col) {
    unsigned char data;
    uint8_t addr = col * 2;
    write_begin();
    write_data_msb(WR_PREFIX, PREFIX_SIZE);
    write_data_msb(addr, ADR_SIZE);
    for (int i = 0; i < 5; i++) {
        data = pgm_read_byte(Font5x7 + (c - 32) * 5 + i);
        write_data_lsb(data, DATA_SIZE);
        write_data_lsb(data >> 4, DATA_SIZE);
    }
    write_end();
}

// Turns off all LEDs in the sign
void clear_display() {
    g_cidx = g_idx = 0;
	memset((void *)g_frame_buffer, 0, MSG_LENGTH);
	
	write_begin();
    write_data_msb(WR_PREFIX, PREFIX_SIZE);
    write_data_msb(0x00, ADR_SIZE);
    for (uint8_t i = 0; i < 0x40; i += 1) {
        write_data_msb(0x00, DATA_SIZE);
    }
    write_end();
}

// Draws the next frame of scrolling text to the buffer
void update_buffer(const char* msg, uint8_t* buffer) {
	memcpy((void *)buffer, (void *)(buffer + 1), SIGNW - 1);
	
	if (msg[g_cidx] == '\0') {
		if (g_idx > SIGNW) {
			g_cidx = 0;
			g_idx = 0;
		} else {
			g_idx++;
			buffer[SIGNW - 1] = 0;
		}
	} else {
	    uint8_t b = pgm_read_byte(Font5x7 + (msg[g_cidx] - 32) * CHARW + g_idx);
	    if (((b == 0) || (g_idx >= CHARW)) && (g_idx > 0)) {
		    g_cidx++;
		    g_idx = 0;
		    buffer[SIGNW - 1] = 0;
	    } else {
		    buffer[SIGNW - 1] = b;
		    g_idx++;
	    }
	}	
}

// Writes an entire buffer to the sign, starting at address 0
// and increasing sequentially
void write_buffer(const uint8_t* buffer) {
	write_begin();
	write_data_msb(WR_PREFIX, PREFIX_SIZE);
	write_data_msb(0, ADR_SIZE);
	for (int i = 0; i < SIGNW; i++) {
		write_data_lsb(buffer[i], DATA_SIZE);
		write_data_lsb(buffer[i] >> 4, DATA_SIZE);
	}
	write_end();
}

// Performs initialization of required sign pins
// and initializes the message
void initialize_sign() {
	set_output(SIGN_DATA_DDR, SIGN_SCK);
    set_output(SIGN_DATA_DDR, SIGN_DATA);
    set_output(SIGN_CS_DDR, SIGN_CS);
	
	output_high(SIGN_CS_PORT, SIGN_CS);
    
    write_command(SYS_EN);
    write_command(LED_ON);
    write_command(RC_MASTER_MODE);
    write_command(COM_OPTION);
	
	eeprom_read_block(g_message, (void *)MSG_ADDR, MSG_LENGTH);
	
	clear_display();
}

// Gets the current sign mode
Mode get_mode() {
	return g_currentMode;
}

// Changes the sign mode
void set_mode(Mode m) {
	switch(m) {
		case CONFIG:
		    sprintf_P(g_message, PSTR("IP:%d.%d.%d.%d   SUBNET:%d.%d.%d.%d"), 
			    EEGET(IP_ADDR+0),EEGET(IP_ADDR+1),EEGET(IP_ADDR+2),EEGET(IP_ADDR+3),
				EEGET(SNET_MASK+0),EEGET(SNET_MASK+1),EEGET(SNET_MASK+2),EEGET(SNET_MASK+3));
			set_speed(1);
			break;
		case MESSAGE:
		    set_speed(EEGET(SPEED_ADDR));
			eeprom_read_block(g_message, (void *)MSG_ADDR, MSG_LENGTH);
			break;
	}
	g_currentMode = m;
	clear_display();
}