/*
 * TCPSerialChannel.h
 *
 *  Created on: Sep 22, 2017
 *      Author: michelo
 */

#ifndef DRIVER_TCPCHANNEL_H_
#define DRIVER_TCPCHANNEL_H_

#include "AbstractSerialChannel.h"
#include <boost/asio.hpp>
#include <stdexcept>

namespace asio = boost::asio;

namespace common {
namespace serial {

class TCPSerialChannel: public AbstractSerialChannel {
	std::string ip;
	int port;
	size_t byte_read;
	boost::system::error_code read_result;

	asio::io_service io_service;
	asio::deadline_timer deadline;
	asio::ip::tcp::socket socket;
	boost::asio::socket_base::bytes_readable command;
	int timeout_arised;
public:
	TCPSerialChannel(const std::string& ipport);
	virtual ~TCPSerialChannel();

	void read_handler(const boost::system::error_code&, std::size_t);
	void check_deadline( );


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

      /**
       reads (synchronous) nb bytes from channel
       @param buffer destination buffer
       @param nb number of bytes to read
       @param timeo milliseconds of timeouts, 0= no timeout
   @param timeout_arised returns if a timeout has been triggered
       @return number of bytes read, negative on error
       */
      virtual int read(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0);

      /**
       reads (asynchronous) nb bytes from channel
       @param buffer destination buffer
       @param nb number of bytes to read
       @return number of bytes read, negative on error
       */
      virtual int read_async(void *buffer,int nb);

      /**
       in asynchronous mode returns the number of bytes available for read_async
       @return number of bytes available, negative on error (buffer overflow)
       */
      virtual int byte_available_read();

      /**
       writes (synchronous) nb bytes to channel
       @param buffer source buffer
       @param nb number of bytes to write
       @param timeo milliseconds of timeouts, 0= no timeout
   @param timeout_arised returns if a timeout has been triggered
       @return number of bytes sucessfully written, negative on error
       */
      virtual int write(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0);

      /**
       writes (asynchronous) nb bytes to channel
       @param buffer source buffer
       @param nb number of bytes to write
       @return number of bytes sucessfully written, negative on error or timeout
       */
      virtual int write_async(void *buffer,int nb);


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

} /* namespace misc */
} /* namespace common */

#endif /* DRIVER_TCPCHANNEL_H_ */
