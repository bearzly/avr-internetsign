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

// EEPROM addresses
#define IP_ADDR   0x0000
#define MAC_ADDR  0x0004
#define SNET_MASK 0x000C
#define GTWY_ADDR 0x0010
#define MSG_ADDR  0x0020

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

const static PROGMEM char HTML_HEADER[] = "<!DOCTYPE html>"
"<html>"
"<head>"
"	<title>Internet Sign</title>"
"   <style type='text/css'>"
"body{font-family:Arial;background:#eee;padding:10px 0}#header{margin:auto;width:70%;background:#777;padding:10px 0}#header ul{padding:0;margin:0;margin-left:3px}#header li{display:inline;background:#555;padding:7px 5px;border:1px #222}#header a{color:#ccc;text-decoration:none}#header a:hover{text-decoration:underline}#container{margin:auto;width:70%;background:#fff;padding:10px 0}h1{margin:0}#content{padding:5px 10px}"
"   </style>"
"</head>"
"<body>"
"	<div id='header'>"
"		<ul>"
"			<li><a href='/'>Home</a></li>"
"			<li><a href='/config'>Configuration</a></li>"
"		</ul>"
"	</div>"
"   <div id='container'>"
"	<div id='content'>";

const static PROGMEM char HTML_FOOTER[] = "	</div></div>"
"</body>"
"</html>";

#endif /* SYSTEM_H_ */