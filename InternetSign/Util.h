/*
 * Util.h
 *
 * Created: 18/03/2012 12:42:03 AM
 *  Author: Benjamin
 */ 


#ifndef UTIL_H_
#define UTIL_H_

#include <avr/eeprom.h>

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

#endif /* UTIL_H_ */