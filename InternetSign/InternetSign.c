/*
 * InternetSign.c
 *
 * Created: 1/12/2012 2:34:57 PM
 *  Author: Benjamin
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <string.h>

#include "System.h"
#include "Sign.h"
#include "Socket.h"

#include <util/delay.h>

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
	
	Init_Wiznet();

    // Ethernet Setup
    unsigned char mac_addr[] = {0x00,0x16,0x36,0xDE,0x58,0xF6};
    unsigned char ip_addr[] = {192,168,1,210};
    unsigned char sub_mask[] = {255,255,255,0};
    unsigned char gtw_addr[] = {192,168,1,1};
		
	set_ip(ip_addr);
	set_gateway(gtw_addr);
	set_mac(mac_addr);
	set_subnet(sub_mask);

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
		status = sockstat();
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
					
				strcpy_P((char *)buffer, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
				strcat_P((char *)buffer, HTML_HEADER);
				strcat_P((char *)buffer, PSTR("<h1>Internet Sign</h1><form method='post' action='/'><input type='text' maxlength='255' name='message'><input type='submit'></form>"));
				strcat((char *)buffer, message);
				strcat_P((char *)buffer, HTML_FOOTER);
					
				if (send(0, buffer, strlen((char *)buffer)) <= 0) break;
					
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
		
		
		update_buffer(message, frame_buffer, i);
		i--;
		if (i < -length) {
			i = SIGNW;
		}			
	    write_buffer(frame_buffer);
		//_delay_ms(10);
	}
}