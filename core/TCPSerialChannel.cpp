/*
 * TCPSerialChannel.cpp
 *
 *  Created on: Sep 22, 2017
 *      Author: michelo
 */

#include "TCPSerialChannel.h"
#include <boost/asio/deadline_timer.hpp>

#include <boost/bind.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/algorithm/string.hpp>
#include <chaos/common/ChaosCommon.h>
#include <common/debug/core/debug.h>
#include <boost/asio/read_until.hpp>
//using boost::lambda::var;

//using boost::lambda::_1;
namespace common {
namespace serial {

int TCPSerialChannel::readLine(std::string&buffer,const std::string delimiter,int timeo_ms){
	int ret=-100;
	boost::asio::streambuf streambuf;
	read_result = boost::asio::error::would_block;
	buffer="";
  boost::asio::async_read_until(socket, streambuf, delimiter,
    [delimiter, &streambuf,&buffer,this](
      const boost::system::error_code& error_code,
      std::size_t bytes_transferred)
    {
      // Verify streambuf contains more data beyond the delimiter. (e.g.
      // async_read_until read beyond the delimiter)
     // assert(streambuf.size() > bytes_transferred);
  //    DPRINT(" streambuf size %ld byte transferred %ld bytes",streambuf.size(),bytes_transferred);

      // Extract up to the first delimiter.
      buffer={buffers_begin(streambuf.data()),buffers_begin(streambuf.data()) + bytes_transferred- delimiter.size()};

      // Consume through the first delimiter so that subsequent async_read_until
      // will not reiterate over the same data.
      streambuf.consume(bytes_transferred);
	//  this->wait_read.notify_one();
   //   DPRINT(" received \"%s\" %ld bytes", buffer.c_str(),streambuf.size());
		read_result = error_code;

    }
  );
  if(timeo_ms>0){
		//	DPRINT("setting timeout to %d ms",ms_timeo);
			//timer.expires_from_now(boost::posix_time::milliseconds(ms_timeo));
			deadline.expires_from_now(boost::posix_time::milliseconds(timeo_ms));
			check_deadline();

	}
/*	if(timeo_ms>0){
	 	if(!this->wait_read.timed_wait(lock_wait,boost::posix_time::milliseconds(timeo_ms)){
			DERR("TIMEOUT");
			return ret;
		}
	} else {
		this->wait_read.wait(lock_wait);
	}*/
	io_service.reset();
	do {
	//	DPRINT(" waiting... delimiter %s",delimiter.c_str() );

		io_service.run_one(); 
	} while (read_result == boost::asio::error::would_block);
	if(read_result==boost::asio::error::timed_out){
		DPRINT(" TIMEOUT... delimiter %s",delimiter.c_str() );

		return -100;
	}
	return buffer.size();
}

TCPSerialChannel::TCPSerialChannel(const std::string& _ip_port):AbstractSerialChannel(_ip_port),socket(io_service),deadline(io_service),byte_read(0), read_result(boost::asio::error::would_block),command(true),timeout_arised(0)  {
	// TODO Auto-generated constructor stub
	std::vector<std::string> strs;
	//boost::split(strs, _ip_port, boost::is_any_of(":"));
	strs=chaos::split(_ip_port,":");
	if(strs.size()!=2){
		throw std::logic_error("bad IP:port specification");
	}
	ip=strs[0];
	port=atoi(strs[1].c_str());
	deadline.expires_at(boost::posix_time::pos_infin);
}

TCPSerialChannel::~TCPSerialChannel() {
	// TODO Auto-generated destructor stub
}


int TCPSerialChannel::init(){
	boost::system::error_code error = boost::asio::error::host_not_found;
	try{
		DPRINT("connecting %s:%d",ip.c_str(),port);
		char number[256];
		sprintf(number,"%d",port);
		boost::asio::ip::tcp::resolver::query query(ip.c_str(), number);
		boost::asio::ip::tcp::resolver resolver(io_service);
		boost::asio::ip::tcp::resolver::iterator destination = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end ;
		boost::asio::ip::tcp::endpoint endpoint;

		while ( destination != end ) {
			endpoint = *destination++;
			std::cout<<endpoint<<std::endl;
		}



		socket.connect(endpoint,error);

		if(error){
			DPRINT("Connecting %s:%d error:%s",ip.c_str(),port,error.message().c_str());
			return -1;
		}
		//check_deadline();
	} catch(boost::system::system_error e){
		DERR("and error occurred connecting: %s",e.what());
		return -2;
	}
	return 0;
}


int TCPSerialChannel::deinit(){
	socket.close();

	return 0;
}

void TCPSerialChannel::read_handler(const boost::system::error_code& ec, std::size_t size){
	if(ec){
		DERR("error ec:%s, size:%lu",ec.message().c_str(),size);
	} else {
	//	DPRINT("read %lu bytes",size);
	}
	read_result=ec;
	byte_read=size;
}

 void  ReadHandler(const boost::system::error_code& ec, std::size_t size){
	if(ec){
		DERR("error ec:%s, size:%lu",ec.message().c_str(),size);
	} else {
		//DPRINT("read %lu bytes",size);
	}

}

void TCPSerialChannel::check_deadline( )
 {
   // Check whether the deadline has passed. We compare the deadline against
   // the current time since a new asynchronous operation may have moved the
   // deadline before this actor had a chance to run.
   if (deadline.expires_at() <= asio::deadline_timer::traits_type::now())
   {
     // The deadline has passed. The socket is closed so that any outstanding
     // asynchronous operations are cancelled. This allows the blocked
     // connect(), read_line() or write_line() functions to return.
     boost::system::error_code ignored_ec;
     DERR("TIMEOUT!!");
     socket.cancel(ignored_ec);
     timeout_arised++;
	 read_result=boost::asio::error::timed_out;
     // There is no longer an active deadline. The expiry is set to positive
     // infinity so that the actor takes no action until a new deadline is set.
     deadline.expires_at(boost::posix_time::pos_infin);
   }

   // Put the actor back to sleep.
   deadline.async_wait(boost::bind(&TCPSerialChannel::check_deadline, this));
 }

int TCPSerialChannel::read(void *buff,int nb,int ms_timeo,int*td){
	if(socket.is_open()){
		read_result = boost::asio::error::would_block;

		boost::system::error_code error;
		int ret;
		timeout_arised=0;
	//	asio::deadline_timer timer(socket.get_io_service()); 

		if(ms_timeo>0){
		//	DPRINT("setting timeout to %d ms",ms_timeo);
			//timer.expires_from_now(boost::posix_time::milliseconds(ms_timeo));
			deadline.expires_from_now(boost::posix_time::milliseconds(ms_timeo));
			check_deadline();

		}
		asio::async_read( socket,  boost::asio::buffer(buff, nb), boost::asio::transfer_all(),boost::bind(&TCPSerialChannel::read_handler,this, boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
		//asio::async_read( socket,  boost::asio::buffer(buff, nb), boost::asio::transfer_all(), ReadHandler);
		//asio::async_read( socket,  boost::asio::buffer(buff, nb), boost::asio::transfer_all(), var(ec) = _1);
		io_service.reset();
		do io_service.run_one(); while (read_result == boost::asio::error::would_block);


		if(read_result){
			if(timeout_arised){
				if(td){
					*td=1;
				}
				return byte_read;
			} else {
				DERR("Error reading %p, len %d, error:%s",buff,nb,read_result.message().c_str());
			}
			return -1;
		}
		return byte_read;
	}
	DERR("socket is closed");
	return -2;
}


int TCPSerialChannel::read_async(void *buff,int nb){
	boost::system::error_code ec = boost::asio::error::would_block;
	asio::async_read( socket,  boost::asio::buffer(buff, nb), boost::bind(&TCPSerialChannel::read_handler,this,ec,byte_read));

	return 0;
}


int TCPSerialChannel::byte_available_read(){
	socket.io_control(command);

	std::size_t bytes_readable = command.get();
	return bytes_readable;
}


int TCPSerialChannel::write(void *buffer,int nb,int ms_timeo,int*timeout_arised){
	if(socket.is_open()){
		boost::system::error_code error;
		int ret;
		ret=asio::write(socket,asio::buffer(buffer,nb),error);
		if(error){
			DERR("Error writing %p, len %d, error:%s",buffer,nb,error.message().c_str());
			return -1;
		}
		return ret;
	}
	DERR("socket is closed");
	return -2;
}


int TCPSerialChannel::write_async(void *buffer,int nb){
	return 0;
}



int TCPSerialChannel::byte_available_write(){
	return 0;
}

void TCPSerialChannel::flush_write(){
	return ;
}


void TCPSerialChannel::flush_read(){
	return;
}




} /* namespace misc */
} /* namespace common */
