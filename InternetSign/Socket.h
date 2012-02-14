/*
 * Socket.h
 *
 * Created: 2/13/2012 9:19:56 PM
 *  Author: Benjamin
 */ 


#ifndef SOCKET_H_
#define SOCKET_H_

// S0_SR values
#define SOCK_CLOSED      0x00     // Closed
#define SOCK_INIT        0x13	  // Init state
#define SOCK_LISTEN      0x14	  // Listen state
#define SOCK_SYNSENT     0x15	  // Connection state
#define SOCK_SYNRECV     0x16	  // Connection state
#define SOCK_ESTABLISHED 0x17	  // Success to connect
#define SOCK_FIN_WAIT    0x18	  // Closing state
#define SOCK_CLOSING     0x1A	  // Closing state
#define SOCK_TIME_WAIT	 0x1B	  // Closing state
#define SOCK_CLOSE_WAIT  0x1C	  // Closing state
#define SOCK_LAST_ACK    0x1D	  // Closing state
#define SOCK_UDP         0x22	  // UDP socket
#define SOCK_IPRAW       0x32	  // IP raw mode socket
#define SOCK_MACRAW      0x42	  // MAC raw mode socket
#define SOCK_PPPOE       0x5F	  // PPPOE socket
// S0_MR values
#define MR_CLOSE	  0x00    // Unused socket
#define MR_TCP		  0x01    // TCP
#define MR_UDP		  0x02    // UDP
#define MR_IPRAW	  0x03	  // IP LAYER RAW SOCK
#define MR_MACRAW	  0x04	  // MAC LAYER RAW SOCK
#define MR_PPPOE	  0x05	  // PPPoE
#define MR_ND		  0x20	  // No Delayed Ack(TCP) flag
#define MR_MULTI	  0x80	  // support multicating

void Init_Wzinet();
void close(uint8_t sock);
void disconnect(uint8_t sock);
uint8_t socket(uint8_t sock, uint8_t protocol, uint16_t port);
uint8_t listen(uint8_t sock);
uint16_t send(uint8_t sock, const uint8_t* buf, uint16_t len);
uint16_t recv(uint8_t sock, uint8_t* buf, uint16_t len);
uint16_t recv_size();
uint8_t sockstat();

#endif /* SOCKET_H_ */