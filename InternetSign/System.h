/*
 * System.h
 *
 * Contains important system constants and a few 
 * useful macros
 *
 * EE400/EE401 2011-2012
 * Group 38 - Internet Sign
 * Benjamin Gwin, James Powers, Karl Rath
 *
 * Created: 2/13/2012 7:02:34 PM
 * Copyright (c) 2012 Benjamin Gwin
 *
 * This software is free/open source under the terms of the
 * GPL v3. See http://www.gnu.org/licenses/gpl.html for details.
 */ 


#ifndef SYSTEM_H_
#define SYSTEM_H_

#define F_CPU 1000000UL // CPU clock speed

#define MSG_LENGTH 256  // Max length of the message

// EEPROM addresses
#define IP_ADDR    (void *)0x0000  // 4 bytes
#define MAC_ADDR   (void *)0x0004  // 6 bytes
#define SNET_MASK  (void *)0x000C  // 4 bytes
#define GTWY_ADDR  (void *)0x0010  // 4 bytes
#define BRGHT_ADDR (void *)0x0014  // 1 byte
#define SPEED_ADDR (void *)0x0015  // 1 byte
#define USER_ADDR  (void *)0x0020  // 32 bytes
#define PWD_ADDR   (void *)0x0040  // 64 bytes
#define MSG_ADDR   (void *)0x0060  // 256 bytes

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

#define EEGET(addr) eeprom_read_byte((const uint8_t*)addr)
#define IS_BYTE(x) ((x >= 0) && (x <= 255))

#endif /* SYSTEM_H_ */