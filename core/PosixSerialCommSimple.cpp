
//
//  PosixSerialComm.cpp
//  serial
//
//  Created by andrea michelotti on 9/23/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.

#define USE_SELECT 1
#ifdef POSIX_SERIAL_COMM_DEBUG
#define DEBUG
#endif
#include "common/debug/debug.h"

#include "PosixSerialCommSimple.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
using namespace common::serial;
#define MIN(x,y) (x<y)?(x):(y)
PosixSerialCommSimple::PosixSerialCommSimple(std::string serial_dev,int baudrate,int parity,int bits,int stop):common::serial::AbstractSerialComm(serial_dev,baudrate,parity,bits,stop){
    fd = -1;
}

PosixSerialCommSimple::~PosixSerialCommSimple(){
  DPRINT("destroying\n");
  deinit();
}




/**
 initialises resource and channel
 @return 0 on success
 */
int PosixSerialCommSimple::init(){
    struct termios term;
    int ret;

    if(fd>=0){
      DPRINT("PosixSerialComm already initialsed\n");
      return 0;
    }

    memset(&term,0,sizeof(struct termios));
    
    fd = open(comm_dev.c_str(),O_RDWR|O_NOCTTY | O_SYNC/*|O_NONBLOCK*/);
    DPRINT("initialising PosixSerialComm, opening %s\n",comm_dev.c_str());
    if(fd<=0){
      DERR("cannot open serial device \"%s\"\n",comm_dev.c_str());
      return SERIAL_CANNOT_OPEN_DEVICE;
    }

    cfmakeraw(&term);
    if(parity==1){
        //odd parity
        term.c_cflag|=PARENB;
        term.c_cflag|=PARODD;
    } else if(parity==2){
      term.c_cflag|=PARENB;
      term.c_cflag&=~PARODD;
    } else if(parity==0){
        term.c_cflag&=~PARENB;
    } else {
      DERR("bad parity %d\n",parity);
      close (fd);
      fd =-1;
      return SERIAL_BAD_PARITY_PARAM;
    }
    if(bits==8){
        term.c_cflag |= CS8;
    } else if(bits == 7){
        term.c_cflag |= CS7;
    } else {
      DERR("bad bits %d\n",bits);
      close (fd);
      fd =-1;
      return SERIAL_BAD_BIT_PARAM;
    }
    
    if(stop==2){
        term.c_cflag |= CSTOPB;
    } else if(stop == 1){
        term.c_cflag &= ~CSTOPB;
    } else {
      DERR("bad stop %d\n",stop);
      close (fd);
      fd =-1;
      return SERIAL_BAD_BIT_PARAM;
    }
    
    switch (baudrate) {
        case 115200:
            ret= cfsetspeed(&term, B115200);
            break;
        case 38400:
            ret = cfsetspeed(&term,B38400);
            break;
        case 9600:
            ret = cfsetspeed(&term,B9600);
            break;
        case 4800:
            ret = cfsetspeed(&term,B4800);
            break;
        default:
            return SERIAL_UNSUPPORTED_SPEED;
            
    }
    term.c_cflag |= CLOCAL|CREAD;
    if (ret<0){
      DERR("bad baudrate %d\n",baudrate);
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_SET_BAUDRATE;
        
    }
    term.c_cc[VTIME]=10;
    term.c_cc[VMIN]=0;
    DPRINT("%s parameters baudrate:%d, parity %d stop %d bits %d\n",comm_dev.c_str(),baudrate,parity,stop,bits);
    if (tcsetattr(fd, TCSANOW, &term)<0){
      DERR("cannot set parameters baudrate:%d, parity %d stop %d bits %d\n",baudrate,parity,stop,bits);
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_SET_PARAMETERS;
    }

    return 0;
}

/**
 deinitalises resources and channel
 @return 0 on success
 */
int PosixSerialCommSimple::deinit(){
    
    if(fd){
        close(fd);
        fd = -1;
    }
    return 0;
}


int PosixSerialCommSimple::read(void *buffer,int nb,int ms_timeo,int *timeo){
  int ret=0,tot=0;
  assert(fd>=0);
  if(timeo) *timeo=0;
  while(tot<nb){
    DPRINT("reading %d/%d bytes timeo %d ms\n",tot,nb,ms_timeo);
    ret = ::read(fd,(void*)(((char*)buffer)+tot),nb-tot);
    if(ret<0){
      return ret;
    } else {
      tot+=ret;
    } 
  }
    return tot;
}

/**
 reads (asynchronous) nb bytes from channel
 @param buffer destination buffer
 @param nb number of bytes to read
 @return number of bytes read, negative on error
 */

int PosixSerialCommSimple::read_async(void *buffer,int nb){
  int ret =0;
  assert(0);
  return ret;
}

int PosixSerialCommSimple::write_async(void *buffer,int nb){
  int ret=0;
  assert(0);
  return ret;
}

/**
 in asynchronous mode returns the number of bytes available for read_async
 @return number of bytes available, negative on error (buffer overflow)
 */
int PosixSerialCommSimple::byte_available_read(){
  int ret=0;
  assert(0);
  return ret;
}

/**
 writes (synchronous) nb bytes to channel
 @param buffer source buffer
 @param nb number of bytes to write
 @param timeo milliseconds of timeouts, 0= no timeout
 @return number of bytes sucessfully written, negative on error or timeout
 */
int PosixSerialCommSimple::write(void *buffer,int nb,int ms_timeo,int* timeo){
  DPRINT("writing %d bytes timeo %d ms\n",nb,ms_timeo);
  return ::write(fd,buffer,nb);
}



/**
 in asynchronous mode returns the number of bytes to write into the channel
 @return number of bytes available, negative on error (buffer overflow)
 */
int PosixSerialCommSimple::byte_available_write(){
  int ret=0;
  assert(0);
  return ret;
}

/**
 flush bytes in the write buffer
 */
void PosixSerialCommSimple::flush_write(){
  
}

/**
 flush bytes in the read buffer
 */
void PosixSerialCommSimple::flush_read(){
  
}


