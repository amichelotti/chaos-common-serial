//
//  OcemProtocol.h
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#ifndef __serial__OcemProtocol__
#define __serial__OcemProtocol__

#ifdef OCEM_PROTOCOL_DEBUG
#define DEBUG
#endif

#include <common/debug/core/debug.h>

#include <iostream>
//#include <common/serial/serial.h>
#include <common/misc/driver/AbstractChannel.h>
#include <pthread.h>

namespace common {
namespace serial {
namespace ocem {
class OcemProtocol {

	enum OcemCtrlChars {
		STX = 0x2,
		ETX = 0x3,
		EOT = 0x4,
		ENQ = 0x5,
		ACK = 0x6,
		NAK = 0x15
	} ;




	int max_answer_size;

	pthread_mutex_t serial_chan_mutex;
protected:
	common::misc::driver::AbstractChannel_psh serial;
	// return number of char copied if success, negative otherwise
	int check_and_extract(char*buffer,char*protbuf,int size);
	// return the total size of the message
	int build_cmd(int slave,char*protbuf,const char* cmd);

	// return 0 if ok
	int sendAck(int ackType,int timeo);
	// return the acktype or an error
	int waitAck(int timeo);


public:

	enum OcemErrors {
		OCEM_CANNOT_OPEN_DEVICE =-100,
		OCEM_CANNOT_INITALIZE,
		OCEM_BAD_SLAVEID,
		OCEM_WRITE_FAILED,
		OCEM_READ_FAILED,
		OCEM_NO_TRAFFIC, // -95
		OCEM_POLL_ANSWER_CRC_FAILED,
		OCEM_MALFORMED_POLL_ANSWER,
		OCEM_UNEXPECTED_SLAVE_ANSWER, //-92
		OCEM_EOT_MISSING_FROM_SLAVE,
		OCEM_NO_ACK_FROM_SLAVE, // -90
		OCEM_ERROR_WRITING_ACK,
		OCEM_NO_ETX_MATCHED,
		OCEM_SLAVE_BUSY, // -87
		OCEM_BAD_SELECT_COMMAND,
		OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE
	} ;
	OcemProtocol(common::misc::driver::AbstractChannel_psh chan);

	//    OcemProtocol(const char*serdev,int max_answer_size=8192,int baudrate=9600,int parity=0,int bits=8,int stop=1);
	~OcemProtocol();
	/**
             perform a poll request toward the given slave
             @param slave slave address
             @param buf the returning buffer
             @param size max size of the returning buffer
             @param timeo timeout in milliseconds (0 indefinite wait)
             @param timeoccur return 1 if a timeout occured
             @return the number of characters read or negative for error
	 */
	virtual int poll(int slave,char * buf,int size,int timeo=1000,int*timeoccur=0);

	/**
             perform a select request toward the given slave
             @param slave slave address
             @param command a zero terminated string containing the command
             @param timeo timeout in milliseconds (=0 indefinite wait)
             @param timeoccur return 1 if a timeout occured
             @return the number of characters of the command sent or negative for error
	 */
	virtual int select(int slave,const char* command,int timeo=1000,int*timeoccur=0);

	virtual int init();
	virtual int deinit();
	virtual int start(){return 0;}
	virtual int stop(){return 0;}
	virtual int registerSlave(int slaveid){return 0;}
	virtual int unRegisterSlave(int slaveid){return 0;}
	virtual int getWriteSize(int slave){return 0;}
	virtual int getReadSize(int slave){return 0;}
	void decodeBuf(char*inpbuf,char*outbuf,int size);
	common::misc::driver::AbstractChannel_psh getChannel() const{return serial;};
};
};
};
};

#endif /* defined(__serial__OcemProtocol__) */
