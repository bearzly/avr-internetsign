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
#define IP_ADDR    0x0000  // 4 bytes
#define MAC_ADDR   0x0004  // 6 bytes
#define SNET_MASK  0x000C  // 4 bytes
#define GTWY_ADDR  0x0010  // 4 bytes
#define BRGHT_ADDR 0x0014  // 1 byte
#define SPEED_ADDR 0x0015  // 1 byte
#define USER_ADDR  0x0020  // 32 bytes
#define PWD_ADDR   0x0040  // 64 bytes
#define MSG_ADDR   0x0060  // 256 bytes

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

const static PROGMEM char HTTP_OK[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
const static PROGMEM char HTTP_AUTH_REQUIRED[] = "HTTP/1.0 401 Authorization Required\r\nWWW-Authenticate: Basic realm=\"Secure Area\"\r\nContent-Type: text/plain\r\n\r\nAuthorization Required";
const static PROGMEM char HTTP_NOT_FOUND[] = "HTTP/1.0 404 Not Found\r\nContent-Type:text/plain\r\n\r\nThe page you requested does not exist";

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

const static PROGMEM char CONFIG_ROW[] = "<tr><td>%s</td><td>%d.%d.%d.%d</td><td><input type='text' name='%s'></td></tr>";
const static PROGMEM char MAC_ROW[] = "<tr><td>MAC Address</td><td>%.2X:%.2X:%.2X:%.2X:%.2X:%.2X</td><td><input type='text' name='mac'></td></tr>";
const static PROGMEM char OPTION_BOX[] = "<tr><td>%s</td><td><input type='text' name='%s' style='width:20px' maxlength='2' value='%d'></td></tr>";

const static PROGMEM char HTML_FOOTER[] = "</div></div>"
"</body>"
"</html>";

#endif /* SYSTEM_H_ */