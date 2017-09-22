//
//  OcemProtocolScheduleCFQ.h
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#ifndef __serial__OcemProtocolCFQ__
#define __serial__OcemProtocolCFQ__

#ifdef OCEM_PROTOCOL_BUFFER_DEBUG
#undef DEBUG
#define DEBUG
#endif
#include <common/debug/core/debug.h>

#include <iostream>
#include "OcemProtocol.h"
#include <map>
#include <queue>
#define MAX_WRITE_QUEUE 2
#define MAX_READ_QUEUE 32
#define READ_PER_WRITE 1
#define ERRORS_TOBE_FATAL 8
#define PAUSE_POLL_NO_DATA 200 //MS
#define PAUSE_INTRA_SELECT 100
#define PAUSE_SELECT_BUSY 1000
#define SLEEP_IF_INACTIVE 10 //MS

namespace common {
namespace serial {
namespace ocem{

class OcemProtocolScheduleCFQ:public OcemProtocol {
public:
	struct Request{
		std::string buffer;
		uint64_t timestamp;
		uint32_t timeo_ms;
		int32_t retry;
		int32_t ret;
		Request(){timeo_ms=0;timestamp=0;retry=0;ret=0;}
	};
	struct OcemData{
		std::queue<Request> queue;
		std::string second_command;
		uint64_t must_wait_to;
		int last_req_index;
		int nsuccessive_busy;
		int protocol_errors;    // number of protocol errors
		uint64_t last_req_time; // timestamp of last request
		uint64_t old_req_time; // timestamp of last request
		uint64_t last_op; // last operation
		uint64_t avg_req_time; // average request time
		uint64_t done_req_time;// average time to accomplish th request
		uint64_t req_ok;        // number of request accomplished ok
		uint64_t req_bad;        // number of request accomplished ok
		uint64_t crc_err;
		uint64_t reqs;  // number of total request
		pthread_mutex_t qmutex;
		pthread_cond_t awake;
		OcemData();
		int pop();
		int size();
		int empty();
		int push(Request& req);

		Request& front();
		Request& back();



	};
	typedef std::pair< int,std::pair< OcemData*,OcemData* > > qdata_t;
	typedef std::map<  int,std::pair< OcemData*,OcemData* >  >  ocem_queue_t;

	typedef std::vector< qdata_t  >  ocem_queue_sorted_t;
	ocem_queue_t slave_queue;
	//   ocem_queue_sorted_t   slave_queue_sorted;
	pthread_mutex_t mutex_buffer;
	pthread_mutex_t mutex_slaves;

	int slaves;
private:
	static void *schedule_thread(void *);
	pthread_t rpid;
	int run;
	int initialized;
	int wait_timeo(pthread_cond_t* cond,pthread_mutex_t*mutex,int timeo_ms);
	uint32_t minChannelAccess_ms;
public:

	//  OcemProtocolScheduleCFQ(const char*serdev,int max_answer_size=8192,int baudrate=9600,int parity=0,int bits=8,int stop=1);
	OcemProtocolScheduleCFQ(::common::misc::driver::AbstractChannel_psh chan,uint32_t minAccess_ms=PAUSE_POLL_NO_DATA);
	~OcemProtocolScheduleCFQ();

	int registerSlave(int slaveid);
	int unRegisterAll();

	int unRegisterSlave(int slaveid);
	void* runSchedule();

	/**
             perform a poll request toward the given slave
             @param slave slave address
             @param buf the returning buffer
             @param size max size of the returning buffer
             @param timeo timeout in milliseconds (0 indefinite wait)
             @param timeoccur return 1 if a timeout occured
             @return the number of characters read or negative for error
	 */
	int poll(int slave,char * buf,int size,int timeo=1000,int*timeoccur=0);

	/**
             perform a select request toward the given slave
             @param slave slave address
             @param command a zero terminated string containing the command
             @param timeo timeout in milliseconds (=0 indefinite wait)
             @param timeoccur return 1 if a timeout occured
             @return the number of characters of the command sent or negative for error
	 */
	int select(int slave,const char* command,int timeo=1000,int*timeoccur=0);

	int init();
	int start();
	int stop();

	int deinit();
	int getWriteSize(int slave);
	int getReadSize(int slave);

};
};
};
};

#endif /* defined(__serial__OcemProtocolScheduleCFQ__) */
