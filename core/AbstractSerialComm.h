//
//  AbstractSerialComm.h
//  serial
//
//  Created by andrea michelotti on 9/23/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#ifndef __serial__AbstractSerialComm__
#define __serial__AbstractSerialComm__

#include <iostream>
namespace common {
    namespace serial {
        
        typedef enum {
            SERIAL_CANNOT_OPEN_DEVICE =-100,
            SERIAL_BAD_PARITY_PARAM,
            SERIAL_BAD_BIT_PARAM,
            SERIAL_BAD_STOP_PARAM,
            SERIAL_UNSUPPORTED_SPEED,
            SERIAL_CANNOT_SET_BAUDRATE,
            SERIAL_CANNOT_SET_PARAMETERS,
            SERIAL_CANNOT_ALLOCATE_RESOURCES,
            SERIAL_TIMEOUT,
            SERIAL_READ_ERROR,
            SERIAL_CANNOT_FIND_DELIM
        } serial_error_t;
        
        // put your code here
        class AbstractSerialComm {
        protected:
            std::string comm_dev;
            int baudrate;
            int parity;
            int bits;
	    int stop;
            
        public:
	AbstractSerialComm(std::string commdev,int _baudrate,int _parity,int _bits,int _stop):comm_dev(commdev),baudrate(_baudrate),parity(_parity),bits(_bits),stop(_stop){}
            
            /**
             initialises resource and channel
             @return 0 on success
             */
            virtual int init()=0;
            
            /**
             deinitalises resources and channel
             @return 0 on success
             */
            virtual int deinit()=0;

            /**
             reads (synchronous) nb bytes from channel
             @param buffer destination buffer
             @param nb number of bytes to read
             @param timeo milliseconds of timeouts, 0= no timeout
	     @param timeout_arised returns if a timeout has been triggered
             @return number of bytes read, negative on error 
             */
            virtual int read(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0)=0;
            
            /**
             reads (asynchronous) nb bytes from channel
             @param buffer destination buffer
             @param nb number of bytes to read
             @return number of bytes read, negative on error
             */
            virtual int read_async(void *buffer,int nb)=0;
            
            /**
             in asynchronous mode returns the number of bytes available for read_async
             @return number of bytes available, negative on error (buffer overflow)
             */
            virtual int byte_available_read()=0;

            /**
             writes (synchronous) nb bytes to channel
             @param buffer source buffer
             @param nb number of bytes to write
             @param timeo milliseconds of timeouts, 0= no timeout
	     @param timeout_arised returns if a timeout has been triggered
             @return number of bytes sucessfully written, negative on error
             */
            virtual int write(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0)=0;
            
            /**
             writes (asynchronous) nb bytes to channel
             @param buffer source buffer
             @param nb number of bytes to write
             @return number of bytes sucessfully written, negative on error or timeout
             */
            virtual int write_async(void *buffer,int nb)=0;

            
            /**
             in asynchronous mode returns the number of bytes to write into the channel
             @return number of bytes available, negative on error (buffer overflow)
             */
            virtual int byte_available_write()=0;
            
            /**
             flush bytes in the write buffer
             */
            virtual void flush_write()=0;
            
            /**
             flush bytes in the read buffer
             */
            virtual void flush_read()=0;



        };
    };
};


#endif /* defined(__serial__AbstractSerialComm__) */
