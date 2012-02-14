/*
 * WebServer.c
 *
 * Created: 2/12/2012 5:05:20 PM
 *  Author: Benjamin
 */ 


#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "Socket.h"

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


#define WAIT_FOR_SPI while (!(SPSR & (1<<SPIF)));

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

void Init_Wiznet() {
	// Ethernet Setup
  unsigned char mac_addr[] = {0x00,0x16,0x36,0xDE,0x58,0xF6};
  unsigned char ip_addr[] = {192,168,1,210};
  unsigned char sub_mask[] = {255,255,255,0};
  unsigned char gtw_addr[] = {192,168,1,1};
  // Setting the Wiznet W5100 Mode Register: 0x0000
  SPI_Write(MR,0x80);            // MR = 0b10000000;
  // Setting the Wiznet W5100 Gateway Address (GAR): 0x0001 to 0x0004
  SPI_Write(GAR + 0,gtw_addr[0]);
  SPI_Write(GAR + 1,gtw_addr[1]);
  SPI_Write(GAR + 2,gtw_addr[2]);
  SPI_Write(GAR + 3,gtw_addr[3]);
  // Setting the Wiznet W5100 Source Address Register (SAR): 0x0009 to 0x000E
  SPI_Write(SAR + 0,mac_addr[0]);
  SPI_Write(SAR + 1,mac_addr[1]);
  SPI_Write(SAR + 2,mac_addr[2]);
  SPI_Write(SAR + 3,mac_addr[3]);
  SPI_Write(SAR + 4,mac_addr[4]);
  SPI_Write(SAR + 5,mac_addr[5]);
  // Setting the Wiznet W5100 Sub Mask Address (SUBR): 0x0005 to 0x0008
  SPI_Write(SUBR + 0,sub_mask[0]);
  SPI_Write(SUBR + 1,sub_mask[1]);
  SPI_Write(SUBR + 2,sub_mask[2]);
  SPI_Write(SUBR + 3,sub_mask[3]);
  // Setting the Wiznet W5100 IP Address (SIPR): 0x000F to 0x0012
  SPI_Write(SIPR + 0,ip_addr[0]);
  SPI_Write(SIPR + 1,ip_addr[1]);
  SPI_Write(SIPR + 2,ip_addr[2]);
  SPI_Write(SIPR + 3,ip_addr[3]);    

  // Setting the Wiznet W5100 RX and TX Memory Size (2KB),
  SPI_Write(RMSR,NET_MEMALLOC);
  SPI_Write(TMSR,NET_MEMALLOC);
}

void close(uint8_t sock) {
	if (sock != 0) return;
	
	SPI_Write(S0_CR, CR_CLOSE);
	while (SPI_Read(S0_CR));
}

void disconnect(uint8_t sock) {
	if (sock != 0) return;
	
	SPI_Write(S0_CR, CR_DISCON);
	while (SPI_Read(S0_CR));
}

uint8_t socket(uint8_t sock, uint8_t protocol, uint16_t port) {
	if (sock != 0) return;
	
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

uint16_t recv(uint8_t sock, uint8_t *buf, uint16_t len) {
	uint16_t ptr, offset, realaddr;
	
	if (len <= 0 || sock != 0) return 1;
		
	ptr = SPI_Read(S0_RX_RD);
	offset = ((ptr & 0x00FF) << 8 + SPI_Read(S0_RX_RD + 1));
	
	while (len) {
		len--;
		realaddr = RXBUFADDR + (offset & RX_BUF_MASK);
		*buf = SPI_Read(realaddr);
		offset++;
		buf++;
	}
	*buf = '\0';
	
	SPI_Write(S0_RX_RD, (offset & 0xFF00) >> 8);
	SPI_Write(S0_RX_RD + 1, (offset & 0x00FF));
	
	SPI_Write(S0_CR, CR_RECV);
	while (SPI_Read(S0_CR));
	
	return 1;
}

uint16_t recv_size() {
	return ((SPI_Read(S0_RX_RSR) & 0x00FF) << 8 ) + SPI_Read(S0_RX_RSR + 1);
}

uint8_t sockstat() {
	return SPI_Read(S0_SR);
}

#if 0

int main() {
	uint8_t sockstat;
	uint16_t rsize;
	
	SPI_DDR = (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_CS);
	SPI_PORT |= (1<<SPI_CS);
	
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
	
	Init_Wiznet();
	
	while (1) {
		sockstat = SPI_Read(S0_SR);
		switch(sockstat) {
			case SOCK_CLOSED:
			    if (socket(sockreg, MR_TCP, TCP_PORT) > 0) {
					if (listen(sockreg) <= 0) {
						_delay_ms(1);
					}
				}
				break;
			case SOCK_ESTABLISHED:
			    rsize = recv_size();
				if (rsize > 0) {
					if (recv(sockreg, buffer, rsize) <= 0) break;
					//TODO: Request parsing
					strcpy_P((char *)buffer, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					strcat_P((char *)buffer, PSTR("<html><body><h1>This is our EE400 design project!</h1></body></html>"));
					
					if (send(sockreg, buffer, strlen((char *)buffer)) <= 0) break;
					
					disconnect(sockreg);
				} else {
					_delay_us(10);
				}
				break;
			case SOCK_FIN_WAIT:
			case SOCK_CLOSING:
			case SOCK_TIME_WAIT:
			case SOCK_CLOSE_WAIT:
			case SOCK_LAST_ACK:
			    close(sockreg);
				break;
		}
	}
	
	return 0;
}

#endif