
//
//  PosixSerialComm.cpp
//  serial
//
//  Created by andrea michelotti on 9/23/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.

#undef DEBUG
#ifdef POSIX_SERIAL_COMM_DEBUG
#define DEBUG
#endif


#include <common/debug/core/debug.h>

#include "PosixSerialComm.h"
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
PosixSerialComm::PosixSerialComm(std::string serial_dev,int _baudrate,int _parity,int _bits,int _stop,bool _hw,int _write_buffer_size,int _read_buffer_size):common::serial::AbstractSerialChannel::AbstractSerialChannel(serial_dev),comm_dev(serial_dev),baudrate(_baudrate),parity(_parity),bits(_bits),stop(_stop),hwctrl(_hw),read_buffer(NULL),read_buffer_size(_read_buffer_size),w_read_buffer_ptr(0),r_read_buffer_ptr(0){
#ifdef POSIX_WRITE_BUFFERING    
  write_buffer=NULL;
  r_write_buffer_ptr=0;
  w_write_buffer_ptr=0;
  write_buffer_size=_write_buffer_size;
  nwrites=0;
  err_write=0;
  DPRINT("creating dev %s,baudrate %d parity %d bits %d stop %d write_buffer size %d read buffer size %d",serial_dev.c_str(),baudrate,parity,bits,stop,write_buffer_size,read_buffer_size);
#else
    DPRINT("creating dev %s,baudrate %d parity %d bits %d stop %d read buffer size %d",serial_dev.c_str(),baudrate,parity,bits,stop,read_buffer_size);
#endif
    fd = -1;

    nreads=0;
    force_exit=0;
    err_read =0;


}

PosixSerialComm::~PosixSerialComm(){
  DPRINT("destroying");
  deinit();
}



int PosixSerialComm::run_read(){

    int ret;
    struct timeval tm;
    fd_set readfd,excfd;
    assert(fd>=0);
    FD_ZERO(&readfd);
    FD_SET(fd,&readfd);
    FD_ZERO(&excfd);
    FD_SET(fd,&excfd);

    DPRINT("read thread buffer size %d bytes, readfd %d ",read_buffer_size,fd);
    while(force_exit==0){
      int tot_read=0;
        //waits 
      DPRINT("waiting for data wptr %d rptr %d full %d",w_read_buffer_ptr,r_read_buffer_ptr,read_full);
      tm.tv_sec=5;
      tm.tv_usec=0;
      ret = select(fd+1,&readfd,NULL,&excfd,&tm);
      if(ret==0){
	DPRINT("nothing received");
	FD_SET(fd,&readfd);
	FD_SET(fd,&excfd);
	continue;
      } else if((ret<0)|| (FD_ISSET(fd,&excfd))){
	err_read++;
	DERR("select:error on receive %d",err_read);
	continue;
      }

      pthread_mutex_lock(&read_mutex);
      DPRINT("receiving readfd: %d readexc: %d ... ret=%d",FD_ISSET(fd,&readfd),FD_ISSET(fd,&excfd),ret);
      if(read_full){
	DPRINT("read buffer FULL wptr:%d, rptr:%d, waiting",w_read_buffer_ptr,r_read_buffer_ptr);
	wait_timeo(&read_full_cond,&read_mutex,0);
      }

	if((w_read_buffer_ptr-r_read_buffer_ptr) == 0){
	  // EMPTY
	  w_read_buffer_ptr=r_read_buffer_ptr=0;
	  DPRINT("reading max %d ... wptr:%d = rptr:%d",read_buffer_size-w_read_buffer_ptr,r_read_buffer_ptr,w_read_buffer_ptr);
	  ret= ::read(fd,&read_buffer[w_read_buffer_ptr],read_buffer_size-w_read_buffer_ptr);
	  DPRINT("read %d/%d bytes  wptr:%d = rptr:%d (empty)",ret,read_buffer_size-w_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
	  if(ret>0){
	    w_read_buffer_ptr+=ret;
	    tot_read+=ret;
	    if(w_read_buffer_ptr == read_buffer_size){
	      w_read_buffer_ptr=0;
	      if(r_read_buffer_ptr>0){
		DPRINT("reading again max %d ... wptr:%d <= rptr:%d",r_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
		ret= ::read(fd,&read_buffer[0],r_read_buffer_ptr);
		DPRINT("read %d/%d bytes  wptr:%d < rptr:%d",ret,r_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
		if(ret>0){
		  w_read_buffer_ptr=ret;
		  tot_read+=ret;
		} 
	      }
	    }
	  }
	} else if(w_read_buffer_ptr-r_read_buffer_ptr> 0){
	  DPRINT("reading max %d ... wptr:%d > rptr:%d",read_buffer_size-w_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
	  ret= ::read(fd,&read_buffer[w_read_buffer_ptr],read_buffer_size-w_read_buffer_ptr);
	  DPRINT("read %d/%d bytes  wptr:%d > rptr:%d",ret,read_buffer_size-w_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
	  if(ret>0){
	    w_read_buffer_ptr+=ret;
	    tot_read+=ret;
	  }
        } else {
	  DPRINT("reading max %d ... wptr:%d < rptr:%d",r_read_buffer_ptr-w_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr);
	  ret= ::read(fd,&read_buffer[w_read_buffer_ptr],r_read_buffer_ptr-w_read_buffer_ptr);
	  DPRINT("read %d/%d bytes  wptr:%d < rptr:%d, full %d",ret,r_read_buffer_ptr-w_read_buffer_ptr,w_read_buffer_ptr,r_read_buffer_ptr,read_full);
	  if(ret>0){
	    w_read_buffer_ptr+=ret;
	    tot_read+=ret;
	  }	  
        }
	if(w_read_buffer_ptr== read_buffer_size)
	  w_read_buffer_ptr = 0;

	if((tot_read>0)&&((w_read_buffer_ptr==r_read_buffer_ptr))){
	  read_full=1;
	}
	DPRINT("%d] read completed wptr:%d  rptr:%d, tot_read %d,full %d ",nreads,w_read_buffer_ptr,r_read_buffer_ptr,tot_read,read_full);
	
        
	if(tot_read>0){
	  pthread_cond_signal(&read_cond);
	}
        nreads++;
	pthread_mutex_unlock(&read_mutex);
    }
    DPRINT("read thread exiting");
    return 0;
}

#ifdef POSIX_WRITE_BUFFERING    

void* PosixSerialComm::write_thread(void* p){
    PosixSerialComm* pnt = (PosixSerialComm*)p;
    return (void*)pnt->run_write();
}


int PosixSerialComm::run_write(){
    fd_set writefd,excfd;
    int ret;
    FD_ZERO(&writefd);
    FD_SET(fd,&writefd);
    FD_ZERO(&excfd);
    FD_SET(fd,&excfd);
    DPRINT("write thread buffer size %d bytes",write_buffer_size);
    while(force_exit==0){
      ret = 0;
      DPRINT("waiting for write full %d",write_full);
      ret = select(fd+1,NULL,&writefd,&excfd,NULL);
      if(ret==0){
	DPRINT("cannot send");
	FD_SET(fd,&writefd);
	FD_SET(fd,&excfd);
	continue;
      } else if((ret<0)|| (FD_ISSET(fd,&excfd))){
	err_write++;
	DPRINT("error on write %d",err_write);
	continue;
      }
      pthread_mutex_lock(&write_mutex);
      if((write_full==0) && (w_write_buffer_ptr-r_write_buffer_ptr)==0){
	DPRINT("Empty ... ready for write full %d",write_full);
	pthread_cond_signal(&write_full_cond);
	wait_timeo(&write_cond,&write_mutex, 0);
      }
      DPRINT("enter writing wptr %d rptr %d full %d",w_write_buffer_ptr,r_write_buffer_ptr,write_full);
      ret = 0;
      if(write_full){
	  //	  DPRINT("write buffer wptr %d rptr %d FULL %d",w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	  assert((w_write_buffer_ptr-r_write_buffer_ptr)==0);
	    
        }
	if((w_write_buffer_ptr-r_write_buffer_ptr) == 0){
	  // full
	  if((w_write_buffer_ptr==0) || (w_write_buffer_ptr==write_buffer_size) ){
	    ret= ::write(fd,&write_buffer[0],write_buffer_size);
	    DPRINT("writing %d/%d bytes wptr %d == rptr %d , full %d",ret,write_buffer_size,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	  } else {    
	    ret= ::write(fd,&write_buffer[r_write_buffer_ptr],write_buffer_size-r_write_buffer_ptr);
	    DPRINT("writing %d/%d bytes wptr %d <==> rptr %d , full %d",ret,write_buffer_size-r_write_buffer_ptr,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	    if(ret == (write_buffer_size-r_write_buffer_ptr)){
	      r_write_buffer_ptr=0;
	      write_full =0;
	      ret= ::write(fd,&write_buffer[0],w_write_buffer_ptr);
	      DPRINT("writing %d/%d bytes wptr %d <===> rptr %d , full %d",ret,w_write_buffer_ptr,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	    }
	  }
	} else if((w_write_buffer_ptr-r_write_buffer_ptr)< 0){
	  // WRAP
	  ret= ::write(fd,&write_buffer[r_write_buffer_ptr],write_buffer_size-r_write_buffer_ptr);
	  DPRINT("writing %d/%d bytes wptr %d < rptr %d , full %d",ret,write_buffer_size-r_write_buffer_ptr,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	  if(ret == (write_buffer_size-r_write_buffer_ptr)){
	    write_full =0;
	    r_write_buffer_ptr=0;
	    ret= ::write(fd,&write_buffer[0],w_write_buffer_ptr);
	    DPRINT("writing %d/%d bytes wptr %d < rptr %d , full %d",ret,w_write_buffer_ptr,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	  }
	} else {
	  // NORMAL
	  ret= ::write(fd,&write_buffer[r_write_buffer_ptr],w_write_buffer_ptr-r_write_buffer_ptr);
	  DPRINT("writing %d/%d bytes wptr %d > rptr %d , full %d",ret,w_write_buffer_ptr-r_write_buffer_ptr,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
	}

	if(ret>0){
	  r_write_buffer_ptr +=ret;
	  if(r_write_buffer_ptr>=write_buffer_size) 
	    r_write_buffer_ptr-=write_buffer_size;
	  if(write_full)
	    pthread_cond_signal(&write_full_cond);
	  write_full  =0;
	}
	nwrites++;
	DPRINT("%d] end wptr %d  rptr %d, full %d",nwrites,w_write_buffer_ptr,r_write_buffer_ptr,write_full);
        pthread_mutex_unlock(&write_mutex);
    }
    DPRINT("write thread exiting");    
    return 0;
}

#endif

 void* PosixSerialComm::read_thread(void* p){
    PosixSerialComm* pnt = (PosixSerialComm*)p;
    return (void*)pnt->run_read();
}

/**
 initialises resource and channel
 @return 0 on success
 */
int PosixSerialComm::init(){
    struct termios term;
    int ret;
    pthread_attr_t attr;
    pthread_condattr_t cond_attr;
    if(fd>=0){
      DPRINT("PosixSerialComm already initialsed");
      return 0;
    }
    wpid=0;
    rpid=0;
    memset(&term,0,sizeof(termios));
    fd = open(comm_dev.c_str(),O_RDWR|O_NOCTTY|O_EXCL);
  
    DPRINT("initialising PosixSerialComm");
    if(fd<=0){
      DERR("cannot open serial device \"%s\"",comm_dev.c_str());
      return SERIAL_CANNOT_OPEN_DEVICE;
    }
    if(comm_dev == "/dev/ptmx"){
      char* p=ptsname(fd);
      if((p!=NULL) && (grantpt(fd)==0)&& (unlockpt(fd)==0)){
	std::cout << "* client pty is \""<<p<<"\""<<std::endl;
      } else {
	DERR("cannot open PTY SERVER serial device \"%s\"",comm_dev.c_str());
	return SERIAL_CANNOT_OPEN_DEVICE;
      }
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
      DERR("bad parity %d",parity);
      close (fd);
      fd =-1;
      return SERIAL_BAD_PARITY_PARAM;
    }
    if(bits==8){
        term.c_cflag |= CS8;
    } else if(bits == 7){
        term.c_cflag |= CS7;
    } else {
      DERR("bad bits %d",bits);
      close (fd);
      fd =-1;
      return SERIAL_BAD_BIT_PARAM;
    }
    
    if(stop==2){
        term.c_cflag |= CSTOPB;
    } else if(stop == 1){
        term.c_cflag &= ~CSTOPB;
    } else {
      DERR("bad stop %d",stop);
      close (fd);
      fd =-1;
      return SERIAL_BAD_BIT_PARAM;
    }
    if(hwctrl)
      term.c_cflag |=CRTSCTS;

    switch (baudrate) {
        case 115200:
            ret= cfsetospeed(&term, B115200);
            break;
        case 38400:
            ret = cfsetospeed(&term,B38400);
            break;
        case 9600:
            ret = cfsetospeed(&term,B9600);
            break;
        case 4800:
            ret = cfsetospeed(&term,B4800);
            break;
        default:
            DERR("unsupported speed %d",baudrate);
            return SERIAL_UNSUPPORTED_SPEED;
            
    }
    if (ret<0){
      DERR("bad baudrate %d",baudrate);
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_SET_BAUDRATE;
        
    }
    term.c_cflag |= CLOCAL|CREAD;
    term.c_cc[VTIME]=1;
    term.c_cc[VMIN]=0;
    DPRINT("%s parameters baudrate:%d, parity %d stop %d bits %d,fd:%d",comm_dev.c_str(),baudrate,parity,stop,bits,fd);
    if (tcsetattr(fd, TCSANOW, &term)<0){
      DERR("cannot set parameters baudrate:%d, parity %d stop %d bits %d",baudrate,parity,stop,bits);
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_SET_PARAMETERS;
    }
    
    r_read_buffer_ptr = 0;
    w_read_buffer_ptr = 0;

    read_full = 0;
    nreads=0;
    force_exit=0;
    err_read =0;

    if(read_buffer==NULL){
      read_buffer = new char[read_buffer_size];
    }

#ifdef POSIX_WRITE_BUFFERING    
    w_write_buffer_ptr = 0;
    r_write_buffer_ptr = 0;
    write_full =0;
    nwrites=0;
    err_write =0;
    if(write_buffer==NULL){
        write_buffer = new char[write_buffer_size];
    }
    if(write_buffer==NULL){
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_ALLOCATE_RESOURCES;
    }
#endif
    if(read_buffer == NULL){
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_ALLOCATE_RESOURCES;
    }

    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&read_full_cond,&cond_attr);
    pthread_cond_init(&read_cond,&cond_attr);

    pthread_mutex_init(&read_mutex,NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#ifdef POSIX_WRITE_BUFFERING    
    pthread_cond_init(&write_cond,&cond_attr);
    pthread_cond_init(&write_full_cond,&cond_attr);
    pthread_mutex_init(&write_mutex,NULL);

    if(pthread_create(&wpid,&attr,write_thread,this)<0){
      DERR("cannot create write thread");
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_ALLOCATE_RESOURCES;
    }
#endif
    if(pthread_create(&rpid,&attr,read_thread,this)<0){
      DERR("cannot create read thread");
      close (fd);
      fd =-1;
      return SERIAL_CANNOT_ALLOCATE_RESOURCES;
    }
    sleep(1);
    return 0;
}

/**
 deinitalises resources and channel
 @return 0 on success
 */
int PosixSerialComm::deinit(){
    int* pwret,*prret;
    int wret,rret;
    DPRINT("deinit fd %d, 0x%x 0x%x",fd,wpid,rpid);
    pwret =&wret;
    prret = &rret;
    
    if(fd>=0){
        close(fd);
        fd = -1;
    } else {
    	DPRINT("Already deinitialized");
    	return 0;
    }
    if(read_buffer){
        delete read_buffer;
        read_buffer = NULL;
    }
#ifdef POSIX_WRITE_BUFFERING    
    if(write_buffer){
        delete write_buffer;
        write_buffer = NULL;
    }
#endif
    force_exit=1;
    

#ifdef POSIX_WRITE_BUFFERING    
    if(wpid){
      DPRINT("waiting writer for x%x",wpid);

      pthread_cond_broadcast(&write_cond);
      pthread_cond_broadcast(&write_full_cond);
      pthread_join(wpid,(void**)&pwret);
      
    }
#endif
    if(rpid){
      pthread_cond_broadcast(&read_cond);
      pthread_cond_broadcast(&read_full_cond);
      DPRINT("waiting reader for x%x",rpid);
      pthread_join(rpid,(void**)&prret);
    }
      DPRINT("done");
    return 0;
}

int PosixSerialComm::wait_timeo(pthread_cond_t* cond,pthread_mutex_t*mutex,int timeo_ms){
  int ret;
  if(timeo_ms>0){
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    ts.tv_sec=tv.tv_sec + timeo_ms/1000;
    ts.tv_nsec=tv.tv_usec*1000 + (timeo_ms%1000)*1000000;
    DPRINT("waiting on %x for %d",cond,timeo_ms);
    if(pthread_cond_timedwait(cond, mutex, &ts)!=0){
            return SERIAL_TIMEOUT;
    }
    DPRINT("exiting from wait on %x for %d",cond,timeo_ms);
    return 0;
  }
  DPRINT("indefinite wait on %x",cond);
  ret = pthread_cond_wait(cond, mutex);
  DPRINT("exiting from indefinite wait on %x",cond);
  return ret;
}
#if 0
int PosixSerialComm::search_delim(int start, int end, char delim){
    for(int cnt = start;cnt<end;cnt++){
        if(read_buffer[cnt]== delim)
            return cnt;
    }
    return -1;
}
#endif
/**
 reads (synchronous) nb bytes from channel
 @param buffer destination buffer
 @param nb number of bytes to read
 @param timeo milliseconds of timeouts, 0= no timeout
 @return number of bytes read, negative on error or timeout
 */
int PosixSerialComm::read(void *buffer,int nb,int ms_timeo,int *timeo){
  int ret=0,tot=0;
  assert(fd>=0);
  if(timeo) *timeo=0;
  while(tot<nb){
    DPRINT("read %d/%d data wptr: %d rptr:%d, read_full %d",tot,nb,w_read_buffer_ptr, r_read_buffer_ptr,read_full); 
    ret = read_async((char*)buffer+tot,nb-tot);
    if(ret==0){
      int res;
      pthread_mutex_lock(&read_mutex);
      res = (w_read_buffer_ptr==r_read_buffer_ptr);
      
      if(res && (read_full==0)){
	w_read_buffer_ptr=r_read_buffer_ptr=0; //reset pointers
	DPRINT("Blocking read %d/%d data wptr: %d rptr:%d (because empty)",tot,nb,w_read_buffer_ptr, r_read_buffer_ptr);    

	res = wait_timeo(&read_cond,&read_mutex,ms_timeo);
	DPRINT("UnBlocking read %d/%d data wptr: %d rptr:%d",tot,nb,w_read_buffer_ptr, r_read_buffer_ptr);
	
	if(res<0){
	  *timeo = 1;
	  pthread_mutex_unlock(&read_mutex);
		
	  return tot;
	}
      }
      pthread_mutex_unlock(&read_mutex);
    } else if (ret>0){
      tot+=ret;
    } else {
      return ret;
    }
  }
  return tot;
}
#if 0
int PosixSerialComm::search_and_read(void *buffer,int nb,char delim,int ms_timeo,int*timeout_arised){
    int found=0;
    int received=byte_available_read();
    int timeout=0;
    int ret;
    do{
        pthread_mutex_lock(&read_mutex);
        if((w_read_buffer_ptr- r_read_buffer_ptr)==0){
            if(read_full){
                if((ret= search_delim(r_read_buffer_ptr,read_buffer_size))>0){
                    int byte_to_read =MIN(nb,ret+1);
                    memcpy(buffer,&buffer[r_read_buffer_ptr],byte_to_read);
                    r_read_buffer_ptr +=byte_to_read;
                    read_full = 0;
                    if(r_read_buffer_ptr== read_buffer_size)
                        r_read_buffer_ptr=0;
                    
                    
                    pthread_mutex_unlock(&read_mutex);
                    pthread_cond_signal(&read_full_cond);
                    return byte_to_read;
                }
                if((ret= search_delim(0,r_read_buffer_ptr))>0){
                    int tot=0;
                    int byte_to_read =MIN(nb,read_buffer_size-r_read_buffer_ptr);
                    memcpy(buffer,&buffer[r_read_buffer_ptr],byte_to_read);
                    nb-=byte_to_read;
                    r_read_buffer_ptr+=byte_to_read;
                    tot +=byte_to_read;
                    if(nb>0){
                        int byte_to_read =MIN(nb,ret+1);
                        memcpy(buffer,&buffer[0],byte_to_read);
                        r_read_buffer_ptr=byte_to_read;
                        tot +=byte_to_read;

                    }
                    read_full = 0;
                    if(r_read_buffer_ptr== read_buffer_size)
                        r_read_buffer_ptr=0;
                    
                    
                    pthread_mutex_unlock(&read_mutex);
                    pthread_cond_signal(&read_full_cond);
                    return tot;
                }
                pthread_mutex_unlock(&read_mutex);
                return SERIAL_CANNOT_FIND_DELIM;
            }
            // nothing in
            DPRINT("Blocking read %d data wptr: %d rptr:%d",nb,w_read_buffer_ptr, r_read_buffer_ptr);
            res = wait_timeo(&read_cond,&read_mutex,ms_timeo);
            
        }
        pthread_mutex_unlock(&read_mutex);
    } while(found==0 && timeout==0);
    
    if(received>0){
    }
    
    
}
#endif
/**
 reads (asynchronous) nb bytes from channel
 @param buffer destination buffer
 @param nb number of bytes to read
 @return number of bytes read, negative on error
 */

int PosixSerialComm::read_async(void *buffer,int nb){
  int ret=0,res;
  assert(fd>=0);
  while(ret<nb){
    if((res=read_async_atomic((char*)buffer+ret,nb-ret))>=0){
      ret+=res;
      if(res<=0) 
	return ret;
    }
  }
  return ret;
}

int PosixSerialComm::write_async(void *buffer,int nb){
  int ret=0,res;
  assert(fd>=0);
  while(ret<nb){
      if((res=write_async_atomic((char*)buffer+ret,nb-ret))>=0){
	ret+=res;
	if(res<=0) 
	  return ret;
      }
    }
    return ret;
}
int PosixSerialComm::read_async_atomic(void *buffer,int nb){
    int ret=0;
    int byte_to_copy;
    if(nb<=0) return 0;
    pthread_mutex_lock(&read_mutex);
    DPRINT("read_async_atomic %d data wptr: %d rptr:%d read_full %d",nb,w_read_buffer_ptr, r_read_buffer_ptr,read_full);    
    assert((w_read_buffer_ptr<read_buffer_size)&&(r_read_buffer_ptr <read_buffer_size));

    if((w_read_buffer_ptr- r_read_buffer_ptr)==0){

        if(read_full){
	  byte_to_copy = MIN(nb,read_buffer_size-r_read_buffer_ptr);
	  memcpy(buffer, &read_buffer[r_read_buffer_ptr],byte_to_copy);
	  nb-=byte_to_copy;
	  ret=byte_to_copy;
	  r_read_buffer_ptr+=byte_to_copy;
	  if(nb>0){
	    int to_copy = MIN(nb,w_read_buffer_ptr);
	    memcpy((char*)buffer+ byte_to_copy, &read_buffer[0],to_copy);
	    r_read_buffer_ptr = to_copy;
	    ret+=to_copy;
	  }
	  read_full=0;
        }
	if(r_read_buffer_ptr== read_buffer_size)
	  r_read_buffer_ptr=0;

	pthread_cond_signal(&read_full_cond);
	pthread_mutex_unlock(&read_mutex);

        return ret;
    }
    
    if((w_read_buffer_ptr- r_read_buffer_ptr)> 0){
      //NORMAL
        byte_to_copy =  MIN(nb,(w_read_buffer_ptr- r_read_buffer_ptr));
        memcpy(buffer, &read_buffer[r_read_buffer_ptr],byte_to_copy);
        r_read_buffer_ptr+=byte_to_copy;
        ret = byte_to_copy;
        
    } else if((w_read_buffer_ptr- r_read_buffer_ptr)< 0){
        // WRAP
        int size_to_wrap=read_buffer_size-r_read_buffer_ptr;
        byte_to_copy= MIN(nb,size_to_wrap);
        memcpy(buffer, &read_buffer[r_read_buffer_ptr],byte_to_copy);
        nb-=byte_to_copy;
        r_read_buffer_ptr+=byte_to_copy;
        ret=byte_to_copy;
        if(nb>0){
            int to_copy = MIN(nb,w_read_buffer_ptr);
            memcpy((char*)buffer+byte_to_copy, &read_buffer[0],to_copy);
            ret+=to_copy;
            r_read_buffer_ptr = to_copy;
        }
    }
    if(ret>0){
      read_full=0;
      if(r_read_buffer_ptr== read_buffer_size)
	r_read_buffer_ptr=0;
      pthread_cond_signal(&read_full_cond);
    }
    pthread_mutex_unlock(&read_mutex);
    return ret;
}

/**
 in asynchronous mode returns the number of bytes available for read_async
 @return number of bytes available, negative on error (buffer overflow)
 */
int PosixSerialComm::byte_available_read(){
    int ret;
    
    if(read_full>0){
      DPRINT("buffer full available %d bytes",read_buffer_size);
      return read_buffer_size;
    }
    pthread_mutex_lock(&read_mutex);
    ret = w_read_buffer_ptr - r_read_buffer_ptr;
    pthread_mutex_unlock(&read_mutex);
    ret = ret>=0 ? ret:ret+read_buffer_size;

    
    DPRINT("buffer available %d bytes",ret);
    assert(ret<read_buffer_size);
    return ret;
}

/**
 writes (synchronous) nb bytes to channel
 @param buffer source buffer
 @param nb number of bytes to write
 @param timeo milliseconds of timeouts, 0= no timeout
 @return number of bytes sucessfully written, negative on error or timeout
 */
int PosixSerialComm::write(void *buffer,int nb,int ms_timeo,int* timeo){
#ifndef POSIX_WRITE_BUFFERING
  return ::write(fd,buffer,nb);
#else
  int tot=0,ret=0;
  assert(fd>=0);
  if(timeo){
    *timeo=0;
  }
  while(tot<nb) {
    DPRINT("writing %d data write_full:%d, at 0x%x[%d]",nb-tot,write_full,(char*)buffer+tot,tot);
    ret = write_async((char*)buffer+tot,nb-tot);
    DPRINT("wrote %d/%d data write_full:%d, at 0x%x[%d]",ret,nb-tot,write_full,(char*)buffer+tot,tot);
    if(ret==0){
      int res;
      pthread_mutex_lock(&write_mutex);
      
      DPRINT("Blocking write %d data write_full:%d , at 0x%x[%d]",nb-tot,write_full,(char*)buffer+tot,tot);
      res = wait_timeo(&write_full_cond,&write_mutex,ms_timeo);
      pthread_mutex_unlock(&write_mutex);
      DPRINT("Unblocking write %d data write_full:%d, ret=%d, at 0x%x[%d]",nb-tot,write_full,ret,(char*)buffer+tot,tot);
      if(res<0){
	*timeo = 1;
	return tot;
      }
    } else if(ret>0){
      tot+=ret;
    } else {
      return ret;
    }
  } ;
  
  return tot;
  #endif
}


/**
 writes (asynchronous) nb bytes to channel
 @param buffer source buffer
 @param nb number of bytes to write
 @return number of bytes sucessfully written, negative on error or timeout
 */

int PosixSerialComm::write_async_atomic(void *buffer,int nb){
    int byte_to_copy;
    int size_to_wrap; 
    int ret=0;
    if(nb<=0) return 0;
#ifndef POSIX_WRITE_BUFFERING
    return ::write(fd,buffer,nb);
#else
    pthread_mutex_lock(&write_mutex);
    if(write_full>0){
      DPRINT("write skipped because FULL x%x of %d data",buffer,nb);    
      pthread_mutex_unlock(&write_mutex);
      return 0;
    }
    


    if((w_write_buffer_ptr-r_write_buffer_ptr) >= 0){
        size_to_wrap =  write_buffer_size - w_write_buffer_ptr;
        assert(size_to_wrap>=0);

        // <- write_buffer_size
        // ... byte_to_copy
        // <- W
        ///...
        // <- R
        // ... to_copy
        // 0

        byte_to_copy =  MIN(nb,size_to_wrap);
	DPRINT("write %d/%d data wptr: %d >= rptr:%d full %d, at @x%x",byte_to_copy,nb,w_write_buffer_ptr, r_write_buffer_ptr,write_full,buffer);    
        memcpy(&write_buffer[w_write_buffer_ptr], buffer,byte_to_copy);
        w_write_buffer_ptr+= byte_to_copy;
        ret+=byte_to_copy;
        nb-=byte_to_copy;
        if(nb>0){
            int to_copy = MIN(nb,r_write_buffer_ptr);
	    if(to_copy>0){
	      DPRINT("write %d/%d data wptr: %d >== rptr:%d full %d, at @x%x",to_copy,nb,w_write_buffer_ptr, r_write_buffer_ptr,write_full,buffer);    
	      memcpy(&write_buffer[0], (char*)buffer + byte_to_copy,to_copy);
	    }
            w_write_buffer_ptr = to_copy;
            ret+=to_copy;
        }
    } else {
        // <- write_buffer_size
        // ...
        // <- R
        /// ... to_copy
        // <- W
        // ..
        // 0
        int to_copy = MIN(nb,r_write_buffer_ptr-w_write_buffer_ptr);
	if(to_copy>0){
	  DPRINT("write %d/%d data wptr: %d < rptr:%d full %d, at @x%x",to_copy,nb,w_write_buffer_ptr, r_write_buffer_ptr,write_full,buffer);    
	  memcpy(&write_buffer[w_write_buffer_ptr], (char*)buffer,to_copy);
	  w_write_buffer_ptr += to_copy;
	  ret+=to_copy;
	}

    }

    if(w_write_buffer_ptr==write_buffer_size)
      w_write_buffer_ptr=0;
    
    if((ret>0)&&(r_write_buffer_ptr== w_write_buffer_ptr)){
        write_full=1;
    }

    DPRINT("async end wptr %d rptr %d full %d, return %d",w_write_buffer_ptr,r_write_buffer_ptr,write_full,ret);
    pthread_cond_signal(&write_cond);
    pthread_mutex_unlock(&write_mutex);


    return ret;

#endif
}



/**
 in asynchronous mode returns the number of bytes to write into the channel
 @return number of bytes available, negative on error (buffer overflow)
 */
int PosixSerialComm::byte_available_write(){
  #ifndef POSIX_WRITE_BUFFERING
  return 1;
  #else
    int ret;
    if(write_full>0)
        return write_buffer_size;
    pthread_mutex_lock(&write_mutex);
    ret = w_write_buffer_ptr - r_write_buffer_ptr;
    pthread_mutex_unlock(&write_mutex);
    ret=ret>=0?ret:ret+write_buffer_size;
    DPRINT("buffer available %d bytes",ret);
    assert(ret<write_buffer_size);
    return ret;
#endif
}

/**
 flush bytes in the write buffer
 */
void PosixSerialComm::flush_write(){
#ifndef POSIX_WRITE_BUFFERING
  return;
#else
  DPRINT("flush wptr = %d rptr=%d", w_write_buffer_ptr ,r_write_buffer_ptr);
  pthread_mutex_lock(&write_mutex);
  write_full=0;
  w_write_buffer_ptr = r_write_buffer_ptr = 0;
  pthread_mutex_unlock(&write_mutex);
#endif
}

/**
 flush bytes in the read buffer
 */
void PosixSerialComm::flush_read(){
  DPRINT("flush wptr = %d rptr=%d", w_read_buffer_ptr ,r_read_buffer_ptr);
  pthread_mutex_lock(&read_mutex);
  read_full=0;
  w_read_buffer_ptr = r_read_buffer_ptr = 0;
  pthread_mutex_unlock(&read_mutex);


}


