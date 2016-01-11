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

        class OcemProtocolBuffered:public OcemProtocol {
            
                       
            std::map<int,std::queue<std::string> > slave_poll_queue;
            std::map<int,std::queue<std::string> > slave_select_queue;
	    pthread_mutex_t schedule_mutex;
	    pthread_cond_t awake;
	    unsigned long nwrites;
	    static void *schedule_thread(void *);

        public:

            OcemProtocolBuffered(const char*serdev,int max_answer_size=8192,int baudrate=9600,int parity=0,int bits=8,int stop=1);
            ~OcemProtocolBuffered();
            
            int registerSlave(int slaveid);

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

#endif /* defined(__serial__OcemProtocolBuffered__) */
