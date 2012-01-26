/*
 * InternetSign.c
 *
 * Created: 1/12/2012 2:34:57 PM
 *  Author: Benjamin
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "Font.h"

#define F_CPU 1000000UL
#include <avr/delay.h>

#define CS1 PD4
#define CLK PD3
#define DATA PD1
#define LED PD2

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

#define PREFIX_SIZE 3
#define CMD_PREFIX 0x04
#define WR_PREFIX 0x05

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

#define ADR_SIZE 7
#define DATA_SIZE 4

#define MSG_LENGTH 256
#define CHARH 7
#define CHARW 5
#define SIGNW 4 * 8

void delay() {
    for (int i = 0; i < 100; i++);
}

void deselect() {
    output_high(PORTD, CS1);
    //delay();
}    

void select() {
    output_low(PORTD, CS1);
    //delay();
}

void clock_pulse() {
    output_low(PORTD, CLK);
    output_high(PORTD, CLK);
}

void write_begin() {
    deselect();
    output_high(PORTD, CLK);
    select();
}

void write_end() {
    deselect();
}

void write_data_msb(uint16_t data, uint8_t bits) {
    for (uint8_t i = bits; i > 0; i--) {
        if (data & (1 << (i - 1))) {
            output_high(PORTD, DATA);
        } else {
            output_low(PORTD, DATA);
        }
        output_low(PORTD, CLK);
        output_high(PORTD, CLK);
    }
}

void write_data_lsb(uint16_t data, uint8_t bits) {
    for (uint8_t i = 0; i < bits; i++) {
        if (data & (1 << i)) {
            output_high(PORTD, DATA);
        } else {
            output_low(PORTD, DATA);
        }
        output_low(PORTD, CLK);
        output_high(PORTD, CLK);
    }
}

void write_command(uint8_t command) {
    write_begin();
    write_data_msb(CMD_PREFIX, PREFIX_SIZE);
    write_data_msb(command << 1, CMD_SIZE);
    write_end();
}

void write_pixels(uint8_t address, uint8_t data) {
    write_begin();
    write_data_msb(WR_PREFIX, PREFIX_SIZE);
    write_data_msb(address, ADR_SIZE);
    write_data_lsb(data, DATA_SIZE);
    write_end();
}

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

void clear_display() {
    write_begin();
    write_data_msb(WR_PREFIX, PREFIX_SIZE);
    write_data_msb(0x00, ADR_SIZE);
    for (uint8_t i = 0; i < 0x40; i += 1) {
        write_data_msb(0x00, DATA_SIZE);
    }
    write_end();
}

static char message[MSG_LENGTH] = "THIS THING IS REALLY FLAKY";
static uint8_t frame_buffer[SIGNW];

void set_buffer(uint8_t* buffer, uint8_t data, uint8_t idx) {
	if (idx < 0 || idx > SIGNW) {
		return;
	}
	buffer[idx] = data; 
} 

void update_buffer(const char* msg, uint8_t* buffer, int idx) {
	int real_pos = 0;
	
	memset((void *)buffer, 0, SIGNW);
	
	for (int i = 0; i < strlen(msg); i++) {
		char c = msg[i];
		if (c == ' ') {
			real_pos += 2;
			set_buffer(buffer, 0, real_pos + idx);
			set_buffer(buffer, 0, real_pos + idx + 1);
		} else {
			for (int j = 0; j < 5; j++) {
				real_pos++;
                uint8_t data = pgm_read_byte(Font5x7 + (c - 32) * 5 + j);
                set_buffer(buffer, data, real_pos + idx);
            }
			real_pos += 1;
		    set_buffer(buffer, 0, real_pos + idx);
		}
	}
}

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

int main(void)
{	
    set_output(DDRD, CLK);
    set_output(DDRD, LED);
    set_output(DDRD, DATA);
    set_output(DDRD, CS1);
    
    deselect();
    
    write_command(SYS_EN);
    write_command(LED_ON);
    write_command(RC_MASTER_MODE);
    write_command(COM_OPTION);
    write_command(PWM_DUTY_8);
    
	int length = 0;
	for (int i = 0; i < strlen(message); i++) {
		if (message[i] == ' ') {
			length += 2;
		} else {
			length += CHARW + 1;
		}
	}
	
	uint8_t i = SIGNW;
	while (1) {
		update_buffer(message, frame_buffer, i);
		i--;
		if (i < 0) {
			i = SIGNW;
		}			
	    write_buffer(frame_buffer);
		_delay_ms(20);
	}
}