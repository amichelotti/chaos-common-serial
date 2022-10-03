#include "TCPSocketClient.h"
#include <boost/algorithm/string.hpp>
#include <common/debug/core/debug.h>

namespace common {
namespace serial {

TCPSocketClient::TCPSocketClient(const std::string& _ip_port):AbstractSerialChannel(_ip_port)
{
    std::vector<std::string> strs;
	boost::split(strs, _ip_port, boost::is_any_of(":"));
	if(strs.size()!=2){
		throw std::logic_error("bad IP:port specification");
	}
	ip=strs[0];
	port=atoi(strs[1].c_str());
    #ifdef _WIN32
		
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			exit(-1);
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
		serv_addr.sin_port = htons(port);
		memset((char*)&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		iResult = getaddrinfo(ip.c_str(), strs[1].c_str(), &hints, &resultAddrInfo);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			exit(-1);
		}
		/*ConnectSocket = socket(resultAddrInfo->ai_family, resultAddrInfo->ai_socktype,
			resultAddrInfo->ai_protocol);*/
		ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			exit(-1);
		}
#else
		struct hostent* server;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cout << "ERROR opening socket";
			exit(0);
		}
		server = gethostbyname(ip.c_str());
		if (server == NULL) {
			std::cout << "ERROR, no such host\n";
			exit(0);
		}
		memset((char*)&serv_addr,0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memmove((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
		serv_addr.sin_port = htons(port);
#endif
	//deadline.expires_at(boost::posix_time::pos_infin);      commented if not using boost::asio

}

TCPSocketClient::~TCPSocketClient() {

}

int TCPSocketClient::init()
{
    	DPRINT("connecting %s:%d",ip.c_str(),port);
	int iResult;
#ifdef _WIN32
		 iResult = ::connect(ConnectSocket, resultAddrInfo->ai_addr, (int)resultAddrInfo->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			closesocket(ConnectSocket);
			is_open = false;
			ConnectSocket = INVALID_SOCKET;
			WSACleanup();
			return iResult;
		}
		else is_open = true;
		
		return 0;
#else
		iResult = ::connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		is_open = (iResult >= 0);
		if (!is_open)
		{
			std::cout << "ERROR connecting" << std::endl;
			return -1;
		}
		return 0;
			
#endif
}
int TCPSocketClient::deinit() {
#ifdef _WIN32
		freeaddrinfo(resultAddrInfo);
		closesocket(ConnectSocket);
		WSACleanup();
#else
		::close(sockfd);
#endif
		this->is_open = false;

	return 0;
}

int TCPSocketClient::read(void *buff,int nb,int ms_timeo,int*td){
		int iResult;
        if (is_open)
        {
            memset(buff, 0, nb);
#ifdef _WIN32
            if (setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ms_timeo, sizeof(int)) < 0)
				printf("setsockopt failed\n");
            iResult = recv(ConnectSocket, (char*)buff, 256, 0);
             if (iResult < 0)
            {
                int err = WSAGetLastError();
				if (err == WSAETIMEDOUT)
				{
					if (td)
					{
						*td = 1;
					}
					printf("Timeout Arised while reading from socket");
				}
				else 
					printf("reading failed with error: %ld\n", err);
            }
#else 
            struct timeval timeout;
            timeout.tv_sec = (int)(ms_timeo / 1000);
	        timeout.tv_usec = ((ms_timeo % 1000) * 1000);
			
            int err=setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			if (err < 0)
            {
                int errtype=errno;
               
				printf("setsockopt rec failed with err %d and errno %d\n",err,errtype);
            }
            iResult = ::read(this->sockfd, buff, nb);
            if (iResult < 0)
            {
                int errorCode = errno;
				if (errorCode == EWOULDBLOCK)
				{
                    if(td){
					    *td=1;
				    }
                    std::cout << "Timeout arised while reading from socket" << std::endl;
                }
                else
                    std::cout << "Error reading from socket" << std::endl;
            }
#endif
            
            return iResult;
        }
        return -1;
}

int TCPSocketClient::write(void *buffer,int nb,int ms_timeo,int*timeout_arised)
{
		if (is_open)
		{
			int iResult;
			std::cout << "Writing " << ((const char*)buffer) << std::endl;
#ifdef _WIN32
            if (setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ms_timeo, sizeof(int)) < 0)
				printf("setsockopt failed\n");

			iResult = send(ConnectSocket,(const char*) buffer, nb, 0);
			if (iResult == SOCKET_ERROR) 
			{
				printf("send failed with error: %d\n", WSAGetLastError());
			}
#else
            struct timeval timeout;
			timeout.tv_sec = (int)(ms_timeo / 1000);
	        timeout.tv_usec = ((ms_timeo % 1000) * 1000);
            
			if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
				printf("setsockopt wr failed\n");

			iResult = ::write(sockfd, buffer, nb);
			if (iResult < 0)
			{
				std::cout << "ERROR writing to socket" << std::endl;
				return -1;
			}
#endif
			return iResult;
		}
		else
		{
			std::cout << "ERROR socket alredy closed" << std::endl;
			return -2;
		}
}

int TCPSocketClient::write_async(void *buffer,int nb){
	return 0;
}



int TCPSocketClient::byte_available_write(){
	return 0;
}

void TCPSocketClient::flush_write(){
	return ;
}


void TCPSocketClient::flush_read(){
	return;
}
int TCPSocketClient::read_async(void *buff,int nb){
    return 0;
}


int TCPSocketClient::byte_available_read(){
	return 0;
}


}}