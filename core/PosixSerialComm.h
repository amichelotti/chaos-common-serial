//
//  PosixSerialComm.h
//  serial
//
//  Created by andrea michelotti on 9/23/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//
// plain implementation using C libc, no boost
#ifndef __serial__PosixSerialComm__
#define __serial__PosixSerialComm__


#include <iostream>


#include <pthread.h>
#include "AbstractSerialChannel.h"


#ifndef POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_WRITE_SIZE
#define POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_WRITE_SIZE 8192
#endif

#ifndef POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_READ_SIZE
#define POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_READ_SIZE 8192
#endif


namespace common {
    namespace serial {
        // put your code here
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

    class PosixSerialComm: public AbstractSerialChannel {

            int fd; // file descriptor of the communication
            pthread_t wpid,rpid; // threads that handle write/read communication
            std::string comm_dev;
            int baudrate;
            int parity;
            int bits;
           	int stop;
           	 bool hwctrl;
            char *read_buffer;
#ifdef POSIX_WRITE_BUFFERING    
	    char *write_buffer;
            int w_write_buffer_ptr; // producer user
	    int r_write_buffer_ptr; // consumer thread
	    int write_full;
	    int write_buffer_size;
	    pthread_mutex_t write_mutex;
	    pthread_cond_t write_cond,write_full_cond;
	    unsigned long nwrites;
	    static void *write_thread(void *);
            int run_write();
#endif
	    int w_read_buffer_ptr; // producer thread
            int r_read_buffer_ptr; // consumer user
            

            int read_full;

            int read_buffer_size;
	    int err_read,err_write;

	    pthread_mutex_t read_mutex;
	    pthread_cond_t read_cond,read_full_cond;
            

            static void *read_thread(void *);
            int wait_timeo(pthread_cond_t* cond,pthread_mutex_t*,int timeo_ms);

	    unsigned long nreads;
            int force_exit;
            
            int producer(char*buffer,int bytes,pthread_mutex_t*);
            int consumer(char*buffer,int bytes,pthread_mutex_t*);
            int run_read();
#if 0
            inline int search_delim(int start,int end, char delim);
#endif
	    int read_async_atomic(void *buffer,int nb);
	    int write_async_atomic(void *buffer,int nb);
        public:
            PosixSerialComm(std::string serial_dev,int baudrate,int parity,int bits,int stop,bool hwctrl=false,int _write_buffer_size=POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_WRITE_SIZE,int _read_buffer_size=POSIX_SERIAL_COMM_DEFAULT_MAX_BUFFER_READ_SIZE);
            
            ~PosixSerialComm();
                       /**
             initialises resource and channel
             @return 0 on success
             */
            int init();
            
            /**
             deinitalises resources and channel
             @return 0 on success
             */
            int deinit();
            
            /**
             reads (synchronous) nb bytes from channel
             @param buffer destination buffer
             @param nb number of bytes to read
             @param timeo milliseconds of timeouts, 0= no timeout
	     @param timeout_arised returns if a timeout has been triggered
             @return number of bytes read, negative on error or timeout
             */
            int read(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0);
         
            
            /**
             search for the specified delimiter and if it's found read the buffer till the specified terminator (included)
             @param buffer destination buffer
             @param nb max number of bytes to read
             @param delim delimite
             @param timeo milliseconds of timeouts, 0= no timeout
             @param timeout_arised returns if a timeout has been triggered
             @return number of bytes read, negative on error or timeout
             */
            int search_and_read(void *buffer,int nb,char delim,int ms_timeo=0,int*timeout_arised=0);

            /**
             reads (asynchronous) nb bytes from channel
             @param buffer destination buffer
             @param nb number of bytes to read
             @return number of bytes read, negative on error
             */
            int read_async(void *buffer,int nb);
            
            /**
             in asynchronous mode returns the number of bytes available for read_async
             @return number of bytes available, negative on error (buffer overflow)
             */
            int byte_available_read();
            
            /**
             writes (synchronous) nb bytes to channel
             @param buffer source buffer
             @param nb number of bytes to write
             @param timeo milliseconds of timeouts, 0= no timeout
	     @param timeout_arised returns if a timeout has been triggered
             @return number of bytes sucessfully written, negative on error
             */
            int write(void *buffer,int nb,int ms_timeo=0,int*timeout_arised=0);
            
            /**
             writes (asynchronous) nb bytes to channel
             @param buffer source buffer
             @param nb number of bytes to write
             @return number of bytes sucessfully written, negative on error or timeout
             */
            int write_async(void *buffer,int nb);
            
            
            /**
             in asynchronous mode returns the number of bytes to write into the channel
             @return number of bytes available, negative on error (buffer overflow)
             */
            int byte_available_write();
            
            /**
             flush bytes in the write buffer
             */
            void flush_write();
            
            /**
             flush bytes in the read buffer
             */
            void flush_read();
            

        };
    };
};


#endif /* defined(__serial__PosixSerialComm__) */
