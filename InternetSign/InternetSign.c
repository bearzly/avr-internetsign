/*
 * InternetSign.c
 *
 * Created: 1/12/2012 2:34:57 PM
 *  Author: Benjamin
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <string.h>

#include "System.h"
#include "Sign.h"
#include "Socket.h"

#include <util/delay.h>

#define EEGET(addr) eeprom_read_byte((const uint8_t*)addr)

#define HTTP_PORT         80       // TCP/IP Port for HTTP

#define MAX_BUFFER_SIZE 1024
uint8_t buffer[MAX_BUFFER_SIZE];

static char message[MSG_LENGTH] = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 !@#$%^&*() `~-_=+[];',.{}:\"<>?|\\";
static uint8_t frame_buffer[SIGNW];

int chartoint(char c) {
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	} else if ((c >= 'A') && (c <= 'Z')) {
		return c - 'A' + 10;
	} else if ((c >= 'a') && (c <= 'a')) {
		return c - 'a' + 10;
	} else {
		return -1;
	}
}

void urldecode(char* dest, const char* src, int size) {
	int j = 0;
	
	for (int i = 0; i < size; i++) {
		if (src[i] == '%') {
			dest[j] = (chartoint(src[i + 1]) << 4) | chartoint(src[i + 2]);
			i += 2;
		} else if (src[i] == '+') {
		    dest[j] = ' ';
		} else {
			dest[j] = src[i];
		}
		j++;
	}
	dest[j] = '\0';
}

void set_speed(uint8_t speed) {
	if (speed < 1) {
		speed = 1;
	} else if (speed > 10) {
		speed = 10;
	}
	OCR1A = 18000 + 4000 * (10 - speed);
}

int main(void)
{	
	uint8_t status;
	
	set_output(SIGN_DATA_DDR, SIGN_SCK);
    set_output(SIGN_DATA_DDR, SIGN_DATA);
    set_output(SIGN_CS_DDR, SIGN_CS);
    set_output(SPI_DDR, SPI_MOSI);
	set_output(SPI_DDR, SPI_SCK);
	set_output(SPI_DDR, SPI_CS);
	
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
	
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << WGM12);
	
	set_speed(5);
	
	Init_Wiznet();
	
	set_ip(
	    EEGET(IP_ADDR + 0),
	    EEGET(IP_ADDR + 1),
	    EEGET(IP_ADDR + 2),
	    EEGET(IP_ADDR + 3)
	);
	set_gateway(
	    EEGET(GTWY_ADDR + 0),
		EEGET(GTWY_ADDR + 1),
		EEGET(GTWY_ADDR + 2),
		EEGET(GTWY_ADDR + 3)
	);
	set_mac(
	    EEGET(MAC_ADDR + 0),
		EEGET(MAC_ADDR + 1),
		EEGET(MAC_ADDR + 2),
		EEGET(MAC_ADDR + 3),
		EEGET(MAC_ADDR + 4),
		EEGET(MAC_ADDR + 5)
	);
	set_subnet(
	    EEGET(SNET_MASK + 0),
		EEGET(SNET_MASK + 1),
		EEGET(SNET_MASK + 2),
		EEGET(SNET_MASK + 3)
	);

    eeprom_read_block(message, MSG_ADDR, MSG_LENGTH);

    initialize_sign();
	set_brightness(4);
    
	int length = 0;
	for (int i = 0; i < strlen(message); i++) {
		if (message[i] == ' ') {
			length += 2;
		} else {
			length += CHARW + 1;
		}
	}
	
	int16_t i = SIGNW;
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
				uint16_t len = 0;
				uint16_t size = 1;
				while (size && (len < MAX_BUFFER_SIZE)) {
					size = recv(0, buffer + len, MAX_BUFFER_SIZE - len);
					len += size;
				}

				buffer[len >= MAX_BUFFER_SIZE - 1 ? MAX_BUFFER_SIZE - 1 : len] = '\0';
					
				char* method = strtok((char *)buffer, " ");
				char* path = strtok(0, " ");
				char* post = strstr(path + strlen(path) + 1, "\r\n\r\n");
				const char* newmessage = strchr(post, '=');
				if (newmessage) {
					int size = strlen(newmessage);
					urldecode(message, newmessage+1, size);
					message[size] = '\0';
					eeprom_update_block(message, MSG_ADDR, MSG_LENGTH);
					i = SIGNW;
					
					length = 0;
	                for (int j = 0; j < strlen(message); j++) {
		                if (message[j] == ' ') {
			                length += 2;
		                } else {
			                length += CHARW + 1;
		                }
	                }
				}					
				
				int isConfig = strcmp_P(path, PSTR("/config")) == 0;
				int isOther = !isConfig && (strcmp_P(path, PSTR("/")) != 0);
				
				if (isOther) {
					strcpy_P((char *)buffer, HTTP_NOT_FOUND);
				} else {
				    strcpy_P((char *)buffer, HTTP_OK);
				    strcat_P((char *)buffer, HTML_HEADER);
				    if (isConfig) {
					    strcat_P((char *)buffer, PSTR("<h1>Configuration</h1><table>"));
						sprintf((char *)buffer+strlen(buffer), "<tr><td>IP Address</td><td>%d.%d.%d.%d</td></tr>", EEGET(IP_ADDR+0),EEGET(IP_ADDR+1),EEGET(IP_ADDR+2),EEGET(IP_ADDR+3));
						sprintf((char *)buffer+strlen(buffer), "<tr><td>Gateway Address</td><td>%d.%d.%d.%d</td></tr>", EEGET(GTWY_ADDR+0),EEGET(GTWY_ADDR+1),EEGET(GTWY_ADDR+2),EEGET(GTWY_ADDR+3));
						sprintf((char *)buffer+strlen(buffer), "<tr><td>Subnet Mask</td><td>%d.%d.%d.%d</td></tr>", EEGET(SNET_MASK+0),EEGET(SNET_MASK+1),EEGET(SNET_MASK+2),EEGET(SNET_MASK+3));
						sprintf((char *)buffer+strlen(buffer), "<tr><td>MAC Address</td><td>%.2X:%.2X:%.2X:%.2X:%.2X:%.2X</td></tr>", EEGET(MAC_ADDR+0),EEGET(MAC_ADDR+1),EEGET(MAC_ADDR+2),EEGET(MAC_ADDR+3),EEGET(MAC_ADDR+3),EEGET(MAC_ADDR+3));
						strcat((char *)buffer, "</table>");
				    } else {
				        strcat_P((char *)buffer, PSTR("<h1>Internet Sign</h1><form method='post' action='/'><input type='text' maxlength='255' name='message'><input type='submit'></form>"));
				        strcat_P((char *)buffer, PSTR("<p><strong>Current Message:</strong>"));
						sprintf((char*)buffer+strlen(buffer), "%d", len);
				        strcat_P((char *)buffer, PSTR("</p>"));
				    }				
				    strcat_P((char *)buffer, HTML_FOOTER);
				}				
				if (send(0, buffer, strlen((char *)buffer)) <= 0) break;
					
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
		
		if (TIFR1 & (1 << OCF1A)) {
			update_buffer(message, frame_buffer, i);
		    i--;
		    if (i < -length) {
			    i = SIGNW;
		    }			
	        write_buffer(frame_buffer);
			
			TIFR1 = (1 << OCF1A);
		}
	}
}