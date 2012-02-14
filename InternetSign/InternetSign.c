/*
 * InternetSign.c
 *
 * Created: 1/12/2012 2:34:57 PM
 *  Author: Benjamin
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "System.h"
#include "Sign.h"
#include "Socket.h"

#define HTTP_PORT         80       // TCP/IP Port for HTTP

#define MAX_BUFFER_SIZE 512
uint8_t buffer[MAX_BUFFER_SIZE];

static char message[MSG_LENGTH] = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 !@#$%^&*() `~-_=+[];',.{}:\"<>?|\\";
static uint8_t frame_buffer[SIGNW];

int main(void)
{	
	uint8_t status, rsize;
	
	set_output(SIGN_DATA_DDR, SIGN_SCK);
    set_output(SIGN_DATA_DDR, SIGN_DATA);
    set_output(SIGN_CS_DDR, SIGN_CS);
    set_output(SPI_DDR, SPI_MOSI);
	set_output(SPI_DDR, SPI_SCK);
	set_output(SPI_DDR, SPI_CS);
	
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
	
	Init_Wiznet();
	
    initialize_sign(4);
    
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
			case SOCK_ESTABLISHED:
			    rsize = recv_size();
				if (rsize > 0) {
					if (recv(0, buffer, rsize) <= 0) break;
					//TODO: Request parsing
					strcpy_P((char *)buffer, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					strcat_P((char *)buffer, PSTR("<html><body><h1>This is our EE400 design project!</h1></body></html>"));
					
					if (send(0, buffer, strlen((char *)buffer)) <= 0) break;
					
					disconnect(0);
				} else {
					_delay_us(10);
				}
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
		//_delay_ms(0);
	}
}