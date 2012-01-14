/*
 * InternetSign.c
 *
 * Created: 1/12/2012 2:34:57 PM
 *  Author: Benjamin
 */ 

#include <avr/io.h>
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
		data = Font5x7[(c - 32) * 5 + i];
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
	
	clear_display();
	
	int idx = 0;
	int direction = 1;
    while(1)
    {
		clear_display();
		write_char('E', idx);
		write_char('E', 5 + idx);
		write_char('4', 10 + idx);
		write_char('0', 15 + idx);
		write_char('1', 20 + idx);
		
		idx += direction;
		if (idx > 8) {
			direction = -1;
			idx = 8;
		} else if (idx == 0) {
			direction = 1;
			idx = 0;
		}				
		_delay_ms(100);			
    }
}