#ifndef DRIVER_TCPSOCKETCHANNEL
#define DRIVER_TCPSOCKETCHANNEL
#include <string>
#include <string.h>
#include <iostream>
#include<stdio.h>
#include <sstream>
#include <vector>
#include <common/serial/core/AbstractSerialChannel.h>
#ifdef _WIN32	
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#endif
namespace common {
namespace serial {

class TCPSocketClient: public AbstractSerialChannel
{
	std::string ip;
	int port;
	size_t byte_read;
    private:
	struct sockaddr_in serv_addr;
	bool is_open = false;
#ifdef _WIN32
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo hints, *resultAddrInfo=NULL;
	
#else
	int sockfd;
	
#endif
public:   
   TCPSocketClient(const std::string& ipport);
	virtual ~TCPSocketClient();

     /**
       initialises resource and channel
       @return 0 on success
       */
      virtual int init();

      /**
       deinitalises resources and channel
       @return 0 on success
       */
      virtual int deinit();

      virtual int read(void *buff,int nb,int ms_timeo,int*td);

      virtual int write(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0);

       virtual int write_async(void *buffer,int nb);

        virtual int read_async(void *buffer,int nb);

      /**
       in asynchronous mode returns the number of bytes available for read_async
       @return number of bytes available, negative on error (buffer overflow)
       */
      virtual int byte_available_read();
      /**
       in asynchronous mode returns the number of bytes to write into the channel
       @return number of bytes available, negative on error (buffer overflow)
       */
      virtual int byte_available_write();

      /**
       flush bytes in the write buffer
       */
      virtual void flush_write();

      /**
       flush bytes in the read buffer
       */
      virtual void flush_read();
};



}}
#endif