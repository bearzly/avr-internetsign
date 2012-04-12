/*
 * Util.h
 *
 * Utility functions for performing string operations
 * on EEPROM strings
 *
 * EE400/EE401 2011-2012
 * Group 38 - Internet Sign
 * Benjamin Gwin, James Powers, Karl Rath
 *
 * Created: 18/03/2012 12:42:03 AM
 * Copyright (c) 2012 Benjamin Gwin
 *
 * This software is free/open source under the terms of the
 * GPL v3. See http://www.gnu.org/licenses/gpl.html for details.
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
	while ((*dst++ = eeprom_read_byte((uint8_t *)src++)));
	return dst;
}

#endif /* UTIL_H_ */