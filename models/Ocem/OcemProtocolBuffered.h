//
//  OcemProtocolBuffered.h
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#ifndef __serial__OcemProtocolBuffered__
#define __serial__OcemProtocolBuffered__

#ifdef OCEM_PROTOCOL_DEBUG
#define DEBUG
#endif
#include <common/debug/core/debug.h>

#include <iostream>
#include "OcemProtocol.h"
#include <map>
#include <queue>

namespace common {
    namespace serial {
        namespace ocem{
        struct Request{
            std::string buffer;
            uint64_t timestamp;
            uint32_t timeo_ms;
            int32_t retry;
            Request(){timeo_ms=0;timestamp=0;retry=0;}
        };
        struct OcemData{
            std::queue<Request> queue;
            int last_req_index;
            int protocol_errors;    // number of protocol errors
            uint64_t last_req_time; // timestamp of last request
            uint64_t avg_req_time; // average request time
            uint64_t done_req_time;// average time to accomplish th request
            uint64_t req_ok;        // number of request accomplished ok
            uint64_t reqs;  // number of total request
            OcemData(){last_req_time=0;req_ok=0;reqs=0;}
            int pushRequest(Request&req);
            int popRequest(Request& req);
        };
        class OcemProtocolBuffered:public OcemProtocol {
            
            typedef std::map<int,std::pair<OcemData*,OcemData*>  >  ocem_queue_t;
            ocem_queue_t slave_queue;
	    pthread_mutex_t schedule_write_mutex,schedule_read_mutex;
	    pthread_cond_t awake;
	    int slaves;
	    static void *schedule_thread(void *);
            pthread_t rpid;
            int run;
	    int initialized;
        public:

            OcemProtocolBuffered(const char*serdev,int max_answer_size=8192,int baudrate=9600,int parity=0,int bits=8,int stop=1);
            ~OcemProtocolBuffered();
            
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
            int select(int slave,char* command,int timeo=1000,int*timeoccur=0);
            
            int init();
            int deinit();
	    
        };
    };
    };
};

#endif /* defined(__serial__OcemProtocolBuffered__) */
