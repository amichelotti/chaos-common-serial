/*
 * SerialChannelFactory.h
 *
 *  Created on: Aug 2, 2017
 *      Author: michelo
 */

#ifndef DRIVER_SERIAL_CHANNELFACTORY_H_
#define DRIVER_SERIAL_CHANNELFACTORY_H_
#include "AbstractSerialChannel.h"
#include <boost/thread.hpp>
#include <common/misc/driver/ConfigDriverMacro.h>

#ifdef CHAOS
#include <chaos/common/exception/CException.h>
#include <chaos/common/data/CDataWrapper.h>

#endif
#include <stdexcept>
#include <map>
namespace common {
namespace serial {

class SerialChannelFactory {
	static std::map<std::string,AbstractSerialChannel_psh> unique_channels;
	static ChaosMutex chanmutex;
public:
	SerialChannelFactory();
	virtual ~SerialChannelFactory();
	// retrieve a Serial Channel
	static AbstractSerialChannel_psh getChannel(std::string serial_dev,int baudrate,int parity,int bits,int stop,bool hwctrl=false);
	//retrieve a tcp channel
	static AbstractSerialChannel_psh getChannel(const std::string& ip, int port );
#ifdef CHAOS
    static AbstractSerialChannel_psh getChannel(const chaos::common::data::CDataWrapper& config)  throw (chaos::CException);
#endif
    static AbstractSerialChannel_psh getChannelFromJson(const std::string& json)  throw (std::logic_error);
	static void removeChannel(const std::string& uid);
	static void removeChannel(AbstractSerialChannel_psh& ch);

};

} /* namespace misc */
} /* namespace common */

#endif /* DRIVER_CHANNELFACTORY_H_ */
