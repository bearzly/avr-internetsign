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
#include <stdlib.h>
#include <stdio.h>

#include "System.h"
#include "Sign.h"
#include "Socket.h"

#include <util/delay.h>

#define EEGET(addr) eeprom_read_byte((const uint8_t*)addr)
#define IS_BYTE(x) ((x >= 0) && (x <= 255))

#define HTTP_PORT         80       // TCP/IP Port for HTTP

#define MAX_BUFFER_SIZE 1024
uint8_t buffer[MAX_BUFFER_SIZE];

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

char base64digit(uint8_t n) {
	if ((n >= 0) && (n <= 25)) {
		return 'A' + n;
	} else if ((n >= 26) && (n <= 51)) {
		return 'a' + n - 26;
	} else if ((n >= 52) && (n <= 61)) {
		return '0' + n - 52;
	} else if (n == 62) {
		return '+';
	} else if (n == 63) {
		return '/';
	} else {
		return '=';
	}
}

char* const base64encode(const char* src) {
	int size = strlen(src);
	const char* end = src + size;
	char* encoded = malloc((size * 4 / 3) + 1);
	if (encoded == NULL) {
		return NULL;
	}
	int idx = 0;
	for (const char* p = src; p < end; p += 3) {
	    uint8_t b3 = (p + 2) > end ? 0 : *(p + 2);
		uint8_t b2 = (p + 1) > end ? 0 : *(p + 1);
		uint32_t buffer = ((uint32_t)(*p) << 16) | ((uint16_t)b2 << 8) | b3;
		for (int i = 0; i < 4; i++) {
			uint8_t num = (buffer >> (6 * (3 - i))) & 0x3f;
			char digit;
			if ((p + i) > end) {
				digit = '=';
			} else {
				digit = base64digit(num);
			}
			encoded[idx++] = digit;
		}
	}
	encoded[idx] = '\0';
	return encoded;
}

size_t strlen_E(const char* ptr) {
	size_t i = 0;
	while (eeprom_read_byte((uint8_t *)ptr++) != '\0') {
		i++;
	}
	return i;
}

char* strcpy_E(char* dst, const char* src) {
	while (*dst++ = eeprom_read_byte((uint8_t *)src++));
	return dst;
}

int main(void)
{			
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
	
	init_wiznet();

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
			    _delay_ms(10);
				uint16_t len = 0;
				uint16_t size = 1;
				while (size && (len < MAX_BUFFER_SIZE)) {
					size = recv(0, buffer + len, MAX_BUFFER_SIZE - len);
					len += size;
				}

				buffer[len >= MAX_BUFFER_SIZE - 1 ? MAX_BUFFER_SIZE - 1 : len] = '\0';
										
				char* method = strtok((char *)buffer, " ");
				char* path = strtok(0, " ");
				
				int isConfig = strcmp_P(path, PSTR("/config")) == 0;
				int isRoot = strcmp_P(path, PSTR("/")) == 0;
				
				int authenticated = 0;
				
				char* header, * nextHeader;
				header = strtok(NULL, "\r\n");
				nextHeader = header;
				while (nextHeader != NULL) {
					if (strstr((char *)header, "Authorization") == (char *)header) {
						char* info = strrchr(header, ' ') + 1;
						char* login = malloc(strlen_E((char *)USER_ADDR) + strlen_E((char *)PWD_ADDR) + 2);
						if (login != NULL) {
							strcpy_E(login, (char *)USER_ADDR);
							size_t size = strlen_E((char *)USER_ADDR);
							login[size] = ':';
							strcpy_E(login + size + 1, (char *)PWD_ADDR);
					        const char* expected = base64encode(login);
						    if (strcmp(info, expected) == 0) {
							    authenticated = 1;
						    }
							free((void *)login);
						    free((void *)expected);
						}
					}
					nextHeader = strtok(NULL, "\r\n");
					if (nextHeader != NULL) header = nextHeader;
				}
				
				if (!authenticated) {
					strcpy_P((char *)buffer, HTTP_AUTH_REQUIRED);
					send(0, buffer, strlen((char *)buffer));
					close(0);
				    disconnect(0);
				    break;
				}
				
				if (strcmp_P(method, PSTR("POST")) == 0) {
				
				    char* post = header;
				    char* param = strtok(post, "&");
				
				    while (param != NULL) {
					    char* equals = strchr(param, '=');
					    char* nextParam = strtok(NULL, "&");
					    if ((equals != NULL) && (((nextParam - equals) > 2) || (nextParam == NULL))) {
						    *equals = '\0';
							const char* value = equals + 1;
						    const size_t size = strlen(value);
						    if (isRoot) {
							    if (strcmp_P(param, PSTR("message")) == 0) {
								    char* message_buf = get_message();
								    urldecode(message_buf, value, size);
								    store_message();
							    }
						    } else if (isConfig) {
							    if (strcmp_P(param, PSTR("ip"))== 0) {
								    int b1, b2, b3, b4;
									int ret = sscanf_P(value, PSTR("%3d.%3d.%3d.%3d"), &b1, &b2, &b3, &b4);
									if (ret == 4) {
										if (IS_BYTE(b1) && IS_BYTE(b2) && IS_BYTE(b3) && IS_BYTE(b4)) {
											set_ip((uint8_t)b1, (uint8_t)b2, (uint8_t)b3, (uint8_t)b4);
											eeprom_update_byte((uint8_t*)IP_ADDR + 0, (uint8_t)b1);
											eeprom_update_byte((uint8_t*)IP_ADDR + 1, (uint8_t)b2);
											eeprom_update_byte((uint8_t*)IP_ADDR + 2, (uint8_t)b3);
											eeprom_update_byte((uint8_t*)IP_ADDR + 3, (uint8_t)b4);
										}
									}
									
							    } else if (strcmp_P(param, PSTR("mac")) == 0) {
								    int b1, b2, b3, b4, b5, b6;
									int ret = sscanf_P(value, PSTR("%2X%%3A%2X%%3A%2X%%3A%2X%%3A%2X%%3A%2X"), &b1, &b2, &b3, &b4, &b5, &b6);
									if (ret == 6) {
										if (IS_BYTE(b1) && IS_BYTE(b2) && IS_BYTE(b3) && IS_BYTE(b4) && IS_BYTE(b5) && IS_BYTE(b6)) {
											set_mac((uint8_t)b1, (uint8_t)b2, (uint8_t)b3, (uint8_t)b4, (uint8_t)b5, (uint8_t)b6);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 0, (uint8_t)b1);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 1, (uint8_t)b2);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 2, (uint8_t)b3);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 3, (uint8_t)b4);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 4, (uint8_t)b5);
											eeprom_update_byte((uint8_t*)MAC_ADDR + 5, (uint8_t)b6);
										}
									}
							    } else if (strcmp_P(param, PSTR("gtwy")) == 0) {
								    int b1, b2, b3, b4;
									int ret = sscanf_P(value, PSTR("%3d.%3d.%3d.%3d"), &b1, &b2, &b3, &b4);
									if (ret == 4) {
										if (IS_BYTE(b1) && IS_BYTE(b2) && IS_BYTE(b3) && IS_BYTE(b4)) {
											set_gateway((uint8_t)b1, (uint8_t)b2, (uint8_t)b3, (uint8_t)b4);
											eeprom_update_byte((uint8_t*)GTWY_ADDR + 0, (uint8_t)b1);
											eeprom_update_byte((uint8_t*)GTWY_ADDR + 1, (uint8_t)b2);
											eeprom_update_byte((uint8_t*)GTWY_ADDR + 2, (uint8_t)b3);
											eeprom_update_byte((uint8_t*)GTWY_ADDR + 3, (uint8_t)b4);
										}
									}
							    } else if (strcmp_P(param, PSTR("snet")) == 0) {
								    int b1, b2, b3, b4;
									int ret = sscanf_P(value, PSTR("%3d.%3d.%3d.%3d"), &b1, &b2, &b3, &b4);
									if (ret == 4) {
										if (IS_BYTE(b1) && IS_BYTE(b2) && IS_BYTE(b3) && IS_BYTE(b4)) {
											set_ip((uint8_t)b1, (uint8_t)b2, (uint8_t)b3, (uint8_t)b4);
											eeprom_update_byte((uint8_t*)SNET_MASK + 0, (uint8_t)b1);
											eeprom_update_byte((uint8_t*)SNET_MASK + 1, (uint8_t)b2);
											eeprom_update_byte((uint8_t*)SNET_MASK + 2, (uint8_t)b3);
											eeprom_update_byte((uint8_t*)SNET_MASK + 3, (uint8_t)b4);
										}
									}
							    } else if (strcmp_P(param, PSTR("brt")) == 0) {
								    int brightness;
									if (sscanf_P(value, PSTR("%d"), &brightness) == 1) {
									    if ((brightness >= 1) && (brightness <= 16)) {
										    set_brightness(brightness);
										}										
									}	
							    } else if (strcmp_P(param, PSTR("speed")) == 0) {
								    int speed;
									if (sscanf_P(value, PSTR("%d"), &speed) == 1) {
									    if ((speed >= 1) && (speed <= 10)) {
										    set_speed(speed);
										}										
									}										
							    }												
						    }
					    }
					    param = nextParam;
				    }
				}				
				
				if (!isConfig && !isRoot) {
					strcpy_P((char *)buffer, HTTP_NOT_FOUND);
				} else {
				    strcpy_P((char *)buffer, HTTP_OK);
				    strcat_P((char *)buffer, HTML_HEADER);
					send(0, buffer, strlen((char *)buffer));
				    if (isConfig) {
					    strcpy_P((char *)buffer, PSTR("<h1>Configuration</h1><form action='/config' method='post'><table>"));
						sprintf_P((char *)buffer+strlen((char *)buffer), CONFIG_ROW, "IP Address", EEGET(IP_ADDR+0),EEGET(IP_ADDR+1),EEGET(IP_ADDR+2),EEGET(IP_ADDR+3), "ip");
						sprintf_P((char *)buffer+strlen((char *)buffer), CONFIG_ROW, "Gateway Address", EEGET(GTWY_ADDR+0),EEGET(GTWY_ADDR+1),EEGET(GTWY_ADDR+2),EEGET(GTWY_ADDR+3), "gtwy");
						sprintf_P((char *)buffer+strlen((char *)buffer), CONFIG_ROW, "Subnet Mask", EEGET(SNET_MASK+0),EEGET(SNET_MASK+1),EEGET(SNET_MASK+2),EEGET(SNET_MASK+3), "snet");
						sprintf_P((char *)buffer+strlen((char *)buffer), MAC_ROW, EEGET(MAC_ADDR+0),EEGET(MAC_ADDR+1),EEGET(MAC_ADDR+2),EEGET(MAC_ADDR+3),EEGET(MAC_ADDR+4),EEGET(MAC_ADDR+5));
						sprintf_P((char *)buffer+strlen((char *)buffer), PSTR("</table><br><table>"));
						sprintf_P((char *)buffer+strlen((char *)buffer), OPTION_BOX, "Brightness (1-16): ", "brt", EEGET((uint8_t *)BRGHT_ADDR));
						sprintf_P((char *)buffer+strlen((char *)buffer), OPTION_BOX, "Speed (1-10): ", "speed", EEGET((uint8_t *)SPEED_ADDR));
						strcat_P((char *)buffer, PSTR("</table><br><input type='submit' value='Save'></form>"));
				    } else {
				        strcpy_P((char *)buffer, PSTR("<h1>Internet Sign</h1><form method='post' action='/'><input type='text' maxlength='255' name='message'><input type='submit'></form>"));
				        strcat_P((char *)buffer, PSTR("<p><strong>Current Message:</strong>"));
						strcat((char *)buffer, get_message());
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
			next_frame();			
			TIFR1 = (1 << OCF1A);
		}
	}
}