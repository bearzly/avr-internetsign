/*
 * System.h
 *
 * Created: 2/13/2012 7:02:34 PM
 *  Author: Benjamin
 */ 


#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <avr/pgmspace.h>

#define F_CPU 1000000UL

#define MSG_LENGTH 256

#define SIGN_CS_PORT PORTB
#define SIGN_CS_DDR  DDRB
#define SIGN_CS      PB0

#define SIGN_DATA_PORT PORTD
#define SIGN_DATA_DDR  DDRD
#define SIGN_SCK       PD7
#define SIGN_DATA      PD6

#define SPI_PORT PORTB
#define SPI_DDR  DDRB
#define SPI_CS   PORTB2
#define SPI_SCK  PORTB5
#define SPI_MOSI PORTB3
#define SPI_MISO PORTB4

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

const static PROGMEM char HTML_HEADER[] = "<!DOCTYPE html>"
"<html>"
"<head>"
"	<title>Internet Sign</title>"
"</head>"
"<body>"
"	<div id='header'>"
"		<ul>"
"			<li><a href='/'>Home</a></li>"
"			<li><a href='/config'>Configuration</a></li>"
"		</ul>"
"	</div>"
"	<div id='content'>";

const static PROGMEM char HTML_FOOTER[] = "	</div>"
"</body>"
"</html>";

#endif /* SYSTEM_H_ */