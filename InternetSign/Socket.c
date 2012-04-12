/*
 * Socket.c
 *
 * EE400/EE401 2011-2012
 * Group 38 - Internet Sign
 * Benjamin Gwin, James Powers, Karl Rath
 *
 * Created: 2/12/2012 5:05:20 PM
 * Copyright (c) 2012 Benjamin Gwin
 *
 * This software is free/open source under the terms of the
 * GPL v3. See http://www.gnu.org/licenses/gpl.html for details.
 */ 


#include <avr/io.h>
#include <string.h>

#include "System.h"
#include "Socket.h"

#include <util/delay.h>

#define SPI_PORT PORTB
#define SPI_DDR  DDRB
#define SPI_CS   PORTB2
#define SPI_SCK  PORTB5
#define SPI_MOSI PORTB3
#define SPI_MISO PORTB4

#define WIZNET_WRITE_OPCODE 0xF0
#define WIZNET_READ_OPCODE  0x0F

// Wiznet W5100 Register Addresses
#define MR         0x0000      // Mode Register
#define GAR        0x0001      // Gateway Address: 0x0001 to 0x0004
#define SUBR       0x0005      // Subnet mask Address: 0x0005 to 0x0008
#define SAR        0x0009      // Source Hardware Address (MAC): 0x0009 to 0x000E
#define SIPR       0x000F      // Source IP Address: 0x000F to 0x0012
#define RMSR       0x001A      // RX Memory Size Register
#define TMSR       0x001B      // TX Memory Size Register
#define S0_MR	   0x0400      // Socket 0: Mode Register Address
#define S0_CR	   0x0401      // Socket 0: Command Register Address
#define S0_IR	   0x0402      // Socket 0: Interrupt Register Address
#define S0_SR	   0x0403      // Socket 0: Status Register Address
#define S0_PORT    0x0404      // Socket 0: Source Port: 0x0404 to 0x0405
#define SO_TX_FSR  0x0420      // Socket 0: Tx Free Size Register: 0x0420 to 0x0421
#define S0_TX_RD   0x0422      // Socket 0: Tx Read Pointer Register: 0x0422 to 0x0423
#define S0_TX_WR   0x0424      // Socket 0: Tx Write Pointer Register: 0x0424 to 0x0425
#define S0_RX_RSR  0x0426      // Socket 0: Rx Received Size Pointer Register: 0x0425 to 0x0427
#define S0_RX_RD   0x0428      // Socket 0: Rx Read Pointer: 0x0428 to 0x0429
#define TXBUFADDR  0x4000      // W5100 Send Buffer Base Address
#define RXBUFADDR  0x6000      // W5100 Read Buffer Base Address
// S0_CR values
#define CR_OPEN          0x01	  // Initialize or open socket
#define CR_LISTEN        0x02	  // Wait connection request in tcp mode(Server mode)
#define CR_CONNECT       0x04	  // Send connection request in tcp mode(Client mode)
#define CR_DISCON        0x08	  // Send closing reqeuset in tcp mode
#define CR_CLOSE         0x10	  // Close socket
#define CR_SEND          0x20	  // Update Tx memory pointer and send data
#define CR_SEND_MAC      0x21	  // Send data with MAC address, so without ARP process
#define CR_SEND_KEEP     0x22	  // Send keep alive message
#define CR_RECV          0x40	  // Update Rx memory buffer pointer and receive data
#define TX_BUF_MASK      0x07FF   // Tx 2K Buffer Mask:
#define RX_BUF_MASK      0x07FF   // Rx 2K Buffer Mask:
#define NET_MEMALLOC     0x05     // Use 2K of Tx/Rx Buffer

// Loops until the SPI register is ready for use
#define WAIT_FOR_SPI while (!(SPSR & (1<<SPIF)));

// Writes the given data to an address on the Wiznet
void SPI_Write(uint16_t addr, uint8_t data) {
	SPI_PORT &= ~(1<<SPI_CS);
	SPDR = WIZNET_WRITE_OPCODE;
	
	WAIT_FOR_SPI;
	
	SPDR = (addr & 0xFF00) >> 8;
	
	WAIT_FOR_SPI;
	
	SPDR = addr & 0x00FF;
	
	WAIT_FOR_SPI;
	
	SPDR = data;
	
	WAIT_FOR_SPI;
	
	SPI_PORT |= (1 << SPI_CS);
}

// Reads a byte from the given address on the Wiznet
unsigned char SPI_Read(uint16_t addr) {
	SPI_PORT &= ~(1 << SPI_CS);
	
	SPDR = WIZNET_READ_OPCODE;
	
	WAIT_FOR_SPI;
	SPDR = (addr & 0xFF00) >> 8;
	WAIT_FOR_SPI;
	SPDR = addr & 0x00FF;
	WAIT_FOR_SPI;
	
	// Send dummy transmission for reading data
	SPDR = 0x00;
	WAIT_FOR_SPI;
	
	SPI_PORT |= (1 << SPI_CS);
	return SPDR;
}

// Initializes ports required for the Wiznet and performs
// basic Wiznet startup commands
void init_wiznet()
{
	SPI_DDR |= (1 << SPI_MOSI);
	SPI_DDR |= (1 << SPI_SCK);
	SPI_DDR |= (1 << SPI_CS);
	
    SPCR = (1<<SPE)|(1<<MSTR);
    SPSR |= (1<<SPI2X);
	
    // Set the MSB of the mode register to cause a reset
	// of the Wiznet registers
    SPI_Write(MR,0x80);
  
    _delay_ms(10);

    // Setting the Wiznet W5100 RX and TX Memory Size (2KB),
    SPI_Write(RMSR,NET_MEMALLOC);
    SPI_Write(TMSR,NET_MEMALLOC);
   
    close(0);
    disconnect(0);
    _delay_ms(10);
}

// Sets the current ip of the Wiznet
void set_ip(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) {
    SPI_Write(SIPR + 0, n1);
	SPI_Write(SIPR + 1, n2);
	SPI_Write(SIPR + 2, n3);
	SPI_Write(SIPR + 3, n4);
}

// Sets the current gateway address of the Wiznet
void set_gateway(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) {
    SPI_Write(GAR + 0, n1);
	SPI_Write(GAR + 1, n2);
	SPI_Write(GAR + 2, n3);
	SPI_Write(GAR + 3, n4);
}

// Sets the current MAC address of the Wiznet
void set_mac(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4, uint8_t n5, uint8_t n6) {
    SPI_Write(SAR + 0, n1);
	SPI_Write(SAR + 1, n2);
	SPI_Write(SAR + 2, n3);
	SPI_Write(SAR + 3, n4);
	SPI_Write(SAR + 4, n5);
	SPI_Write(SAR + 5, n6);
}

// Sets the current subnet mask of the Wiznet
void set_subnet(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) {
    SPI_Write(SUBR + 0, n1);
	SPI_Write(SUBR + 1, n2);
	SPI_Write(SUBR + 2, n3);
	SPI_Write(SUBR + 3, n4);
}

// Closes a socket
void close(uint8_t sock) {
	if (sock != 0) return;
	
	SPI_Write(S0_CR, CR_CLOSE);
	while (SPI_Read(S0_CR));
}

// Disconnects a socket
void disconnect(uint8_t sock) {
	if (sock != 0) return;
	
	SPI_Write(S0_CR, CR_DISCON);
	while (SPI_Read(S0_CR));
	
	_delay_ms(10);
}

// Creates a new socket with the given protocol and port
// Returns 1 upon success
uint8_t socket(uint8_t sock, uint8_t protocol, uint16_t port) {
	if (sock != 0) return 0;
	
	uint8_t ret = 0;
	
	if (SPI_Read(S0_SR) == SOCK_CLOSED) {
		close(sock);
	}
	
	SPI_Write(S0_MR, protocol);
	
	SPI_Write(S0_PORT, ((port & 0xFF00) >> 8));
	SPI_Write(S0_PORT + 1, port & 0x00FF);
	SPI_Write(S0_CR, CR_OPEN);
	
	while (SPI_Read(S0_CR));
	
	if (SPI_Read(S0_SR) == SOCK_INIT) {
		ret = 1;
	} else {
		close(sock);
	}
	
	return ret;
}

// Causes a socket to listen for requests
// Returns 0 on success and 1 on failure
uint8_t listen(uint8_t sock) {
	uint8_t ret = 0;
	if (sock != 0) return ret;
	if (SPI_Read(S0_SR) == SOCK_INIT) {
		SPI_Write(S0_CR, CR_LISTEN);
		
		while (SPI_Read(S0_CR));
		
		if (SPI_Read(S0_SR) == SOCK_LISTEN) {
			ret = 1;
		} else {
			close(sock);
		}
	}
	return ret;
}

// Sends 'len' bytes of buffer to the remote client
// Returns 1 on success and 0 on failure
uint16_t send(uint8_t sock, const uint8_t* buf, uint16_t len) {
	uint16_t ptr, offset, realaddr, txsize, timeout;
	
	if (len <= 0 || sock != 0) return 0;
	
	txsize = SPI_Read(SO_TX_FSR);
	txsize = (((txsize & 0x00FF) << 8) + SPI_Read(SO_TX_FSR + 1));
	
	timeout = 0;
	while (txsize < len) {
		_delay_ms(1);
		txsize = SPI_Read(SO_TX_FSR);
	    txsize = (((txsize & 0x00FF) << 8) + SPI_Read(SO_TX_FSR + 1));
		if (timeout++ > 1000) {
			disconnect(sock);
			return 0;
		}
	}
	
	ptr = SPI_Read(S0_TX_WR);
	offset = (((ptr & 0x00FF) << 8) + SPI_Read(S0_TX_WR + 1));
	
	while (len) {
		len--;
		
		realaddr = TXBUFADDR + (offset & TX_BUF_MASK);
		SPI_Write(realaddr, *buf);
		offset++;
		buf++;
	}
	
	SPI_Write(S0_TX_WR, (offset & 0xFF00) >> 8);
	SPI_Write(S0_TX_WR + 1, (offset & 0x00FF));
	
	SPI_Write(S0_CR, CR_SEND);
	
	while (SPI_Read(S0_CR));
	
	return 1;
}

// Receives data from the remote client into buf.
// The number of bytes received is less than or equal to len.
// Returns the number of bytes that were received
uint16_t recv(uint8_t sock, uint8_t *buf, uint16_t len) {
	if (sock != 0) return 0;
	
	uint16_t rcv_size = SPI_Read(S0_RX_RSR);
	rcv_size = (rcv_size << 8) | SPI_Read(S0_RX_RSR + 1);
	
	uint16_t offset = SPI_Read(S0_RX_RD);
	offset = (offset << 8) | SPI_Read(S0_RX_RD + 1);
	offset &= RX_BUF_MASK;
		
	uint16_t bytes_to_read = rcv_size;
	if ((offset + rcv_size) > (RX_BUF_MASK + 1)) {
		bytes_to_read = RX_BUF_MASK + 1 - offset;
	}
	
	for (int i = 0; i < bytes_to_read; i++) {
		*buf = SPI_Read(RXBUFADDR + (offset & RX_BUF_MASK));
		offset++;
		buf++;
	}
		
	SPI_Write(S0_RX_RD, (offset & 0xFF00) >> 8);
	SPI_Write(S0_RX_RD + 1, (offset & 0x00FF));
	
	SPI_Write(S0_CR, CR_RECV);
	while (SPI_Read(S0_CR));
	
	return bytes_to_read;
}

// Returns the current status of the socket
uint8_t sockstat(uint8_t socket) {
	if (socket != 0) return 0;
	return SPI_Read(S0_SR);
}