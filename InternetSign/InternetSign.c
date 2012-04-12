/*
 * InternetSign.c
 *
 * Contains the main method for the internet sign
 *
 * EE400/EE401 2011-2012
 * Group 38 - Internet Sign
 * Benjamin Gwin, James Powers, Karl Rath
 *
 * Created: 1/12/2012 2:34:57 PM
 * Copyright (c) 2012 Benjamin Gwin
 *
 * This software is free/open source under the terms of the
 * GPL v3. See http://www.gnu.org/licenses/gpl.html for details.
 */ 

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <string.h>
#include <stdlib.h>

#include "System.h"
#include "Sign.h"
#include "Socket.h"
#include "RequestHandler.h"

#include <util/delay.h>

#define HTTP_PORT         80       // TCP/IP Port for HTTP

#define MAX_BUFFER_SIZE 1024
uint8_t buffer[MAX_BUFFER_SIZE];   // Common buffer used for network I/O

int buttonState = 1; // Old state of the rest button (active low)

static uint8_t defaultIp[] = {192, 168, 0, 20};
static uint8_t defaultGateway[] = {192, 168, 0, 1};
static uint8_t defaultMask[] = {255, 255, 255, 0};
static uint8_t defaultMac[] = {0x00, 0x16, 0x36, 0xDE, 0x58, 0xF6};

// Entry point for the internet sign
int main(void)
{	
	// Disable the watchdog timer to prevent constant resetting
	MCUSR = 0;
    wdt_disable();
			
	// Initialize scrolling timer
	TCCR0B |= (1 << CS02);   // Enable 8 bit timer with a 256 prescaler
	TCCR0A |= (1 << WGM01);  // Enable 8 bit timer CTC mode
	
	// Initialize reset timer
	TCCR1B |= (1 << CS12);   // Enable 16 bit timer with a 256 prescaler
	TCCR1B |= (1 << WGM12);  // Enable 16 bit timer CTC mode
	OCR1A = 9000;           // Set output compare for ~3 seconds
	
	// Initialize pin change interrupt on PD0
	set_input(DDRD, PD0);
	output_low(PORTD, PD0);
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT16);
	sei();
	
	init_wiznet();
	set_ip(EEGET(IP_ADDR + 0),EEGET(IP_ADDR + 1),EEGET(IP_ADDR + 2),EEGET(IP_ADDR + 3));
	set_gateway(EEGET(GTWY_ADDR + 0),EEGET(GTWY_ADDR + 1),EEGET(GTWY_ADDR + 2),EEGET(GTWY_ADDR + 3));
	set_mac(EEGET(MAC_ADDR + 0),EEGET(MAC_ADDR + 1),EEGET(MAC_ADDR + 2),EEGET(MAC_ADDR + 3),EEGET(MAC_ADDR + 4),EEGET(MAC_ADDR + 5));
	set_subnet(EEGET(SNET_MASK + 0),EEGET(SNET_MASK + 1),EEGET(SNET_MASK + 2),EEGET(SNET_MASK + 3));
	
    initialize_sign();
	set_brightness(EEGET(BRGHT_ADDR));
	set_speed(EEGET(SPEED_ADDR));
    
	uint8_t status;
	while (1) {
		status = sockstat(0);
		switch(status) {
			case SOCK_CLOSED:
			    if (socket(0, MR_TCP, HTTP_PORT) > 0) {
					if (listen(0) <= 0) {
						_delay_ms(1);
					}
				}
				break;
			case SOCK_ESTABLISHED:;
			    // This delay was needed because the Wiznet needed
				// some time before filling the received size register
				_delay_ms(10);
				
				// Read chunks until the request is exhausted or the
				// maximum buffer size has been read
				uint16_t len = 0;
	            uint16_t size = 1;
	            while (size && (len < MAX_BUFFER_SIZE)) {
		            size = recv(0, buffer + len, MAX_BUFFER_SIZE - len);
		            len += size;
	            }

                // Terminate the request with a null character
	            buffer[len >= MAX_BUFFER_SIZE - 1 ? MAX_BUFFER_SIZE - 1 : len] = '\0';
				
				handle_request(buffer);
					
				close(0);
				disconnect(0);
				break;
			case SOCK_FIN_WAIT:
			case SOCK_CLOSING:
			case SOCK_TIME_WAIT:
			case SOCK_CLOSE_WAIT:
			case SOCK_LAST_ACK:
			    close(0);
				break;
		}
		
		// Check if the timer compare has been reached, and
		// advance the sign to the next frame if it has
		if (TIFR0 & (1 << OCF0A)) {
			next_frame();			
			TIFR0 = (1 << OCF0A); // resets the timer
		}
	}
}

// Interrupt triggered when the reset button is pressed
ISR(PCINT2_vect) {
	if (buttonState == 1 && (PIND & PD0) == 0) {
		// Button went from off to on
		// Reset the timer
		TCNT1 = 0;
		TIFR1 = (1 << OCF1A);
	} else if (buttonState == 0 && (PIND & (1 << PD0)) == 1) {
		// Button went from on to off
		if (TIFR1 & (1 << OCF1A)) {
			// reset
			eeprom_update_block(defaultIp, IP_ADDR, 4);
			eeprom_update_block(defaultGateway, GTWY_ADDR, 4);
			eeprom_update_block(defaultMask, SNET_MASK, 4);
			eeprom_update_block(defaultMac, MAC_ADDR, 6);
			
			// Enable watchdog timer and loop until software
			// reset occurs
			cli();
			wdt_enable(WDTO_15MS);
			while (1) { };
		} else {
			if (get_mode() == MESSAGE) {
				set_mode(CONFIG);
			} else {
				set_mode(MESSAGE);
			}
		}
	}
	buttonState = PIND & (1 << PD0);
}