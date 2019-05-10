/*
 * SerialChannelFactory.cpp
 *
 *  Created on: Aug 2, 2017
 *      Author: michelo
 */

#include "SerialChannelFactory.h"
#include "PosixSerialComm.h"
#include <common/debug/core/debug.h>
#include "TCPSerialChannel.h"

namespace common {
namespace serial {

std::map<std::string,AbstractSerialChannel_psh> SerialChannelFactory::unique_channels;
boost::mutex SerialChannelFactory::chanmutex;

#ifdef CHAOS
using namespace chaos::common::data;
AbstractSerialChannel_psh SerialChannelFactory::getChannelFromJson(const std::string& json)  throw (std::logic_error){
	try{
		chaos::common::data::CDataWrapper data;
		data.setSerializedJsonData(json.c_str());
		return SerialChannelFactory::getChannel(data);
	} catch (...){
		throw std::logic_error("bad json");
	}
}
AbstractSerialChannel_psh SerialChannelFactory::getChannel(const chaos::common::data::CDataWrapper& json )  throw(chaos::CException) {
	AbstractSerialChannel_psh tt;
	GET_PARAMETER_TREE((&json),channel){
		GET_PARAMETER_DO(channel,serdev,string,0){
			//serial channel
			GET_PARAMETER(channel,baudrate,int32_t,1);
			GET_PARAMETER(channel,parity,int32_t,1);
			GET_PARAMETER(channel,stop,int32_t,1);
			GET_PARAMETER(channel,hwctrl,int32_t,1);
			GET_PARAMETER(channel,bits,int32_t,1);
			return getChannel(serdev,baudrate,parity,bits,stop,hwctrl);

		}
		GET_PARAMETER_DO(channel,tcp,string,0){
			GET_PARAMETER(channel,port,int32_t,1);

			return getChannel(tcp,port);

		}
	}
	return tt;
}
#else
AbstractSerialChannel_psh SerialChannelFactory::getChannelFromJson(const std::string& json)  throw (std::logic_error){
	throw std::logic_error("not implemented");

}

#endif

AbstractSerialChannel_psh SerialChannelFactory::getChannel(std::string serial_dev,int baudrate,int parity,int bits,int stop,bool hwctrl){
	boost::mutex::scoped_lock l(chanmutex);
	std::map<std::string,AbstractSerialChannel_psh>::iterator i=unique_channels.find(serial_dev);
	if(i!=unique_channels.end()){
		DPRINT("retrieving SERIAL channel '%s' @%p in use count %ld",serial_dev.c_str(),i->second.get(),i->second.use_count());
		return i->second;
	}
	common::serial::PosixSerialComm* ptr=new common::serial::PosixSerialComm(serial_dev,baudrate,parity,bits,stop,hwctrl);
	AbstractSerialChannel_psh ret(ptr);
	unique_channels[serial_dev]=ret;
	DPRINT("creating SERIAL channel '%s' @%p",serial_dev.c_str(),ret.get());

	return ret;

}
AbstractSerialChannel_psh SerialChannelFactory::getChannel(const std::string& ip, int port ){
	AbstractSerialChannel_psh p;
	std::stringstream ss;
	ss<<ip<<":"<<port;

	boost::mutex::scoped_lock l(chanmutex);
	std::map<std::string,AbstractSerialChannel_psh>::iterator i=unique_channels.find(ss.str());
	if(i!=unique_channels.end()){
		DPRINT("retrieving TCP channel '%s' @%p in use count %ld",ss.str().c_str(),i->second.get(),i->second.use_count());
		return i->second;
	}
	DPRINT("creating TCP channel '%s' @%p",ss.str().c_str(),p.get());
	TCPSerialChannel* ptr=new TCPSerialChannel(ss.str());
	AbstractSerialChannel_psh ret(ptr);
	unique_channels[ss.str()]=ret;
	return ret;
}

void SerialChannelFactory::removeChannel(const std::string& uid){
	boost::mutex::scoped_lock l(chanmutex);

	std::map<std::string,AbstractSerialChannel_psh>::iterator i=unique_channels.find(uid);
	if(i!=unique_channels.end()){
		if(i->second.use_count()==1){
			DPRINT("REMOVING CHANNEL '%s' @%p in use %ld",uid.c_str(),i->second.get(),i->second.use_count());
			unique_channels.erase(i);
		} else {
			DPRINT("CANNOT REMOVE CHANNEL '%s' @%p in use %ld",uid.c_str(),i->second.get(),i->second.use_count());
		}
	}
}
void SerialChannelFactory::removeChannel(AbstractSerialChannel_psh& ch){
	if(ch.get()==0){
		DPRINT("CHANNEL REMOVED in use %ld",ch.use_count());
		return;
	}
	std::string uid=ch->getUid();

	DPRINT("ATTEMPT TO REMOVE CHANNEL'%s' @%p in use in %ld places",uid.c_str(),ch.get(),ch.use_count());
	ch.reset();

	removeChannel(uid);
}

SerialChannelFactory::SerialChannelFactory() {
	// TODO Auto-generated constructor stub

}

SerialChannelFactory::~SerialChannelFactory() {
	// TODO Auto-generated destructor stub
}


} /* namespace misc */
} /* namespace common */
