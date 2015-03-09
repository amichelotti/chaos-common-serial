//
//  echo_serial.cpp
// 
// Andrea Michelotti
// echo over a serial line
//
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <common/serial/pserial.h>
#include <common/debug/debug.h>

using namespace common::serial;

#define BUFFER_SIZE 8192
int main(int argc, char *argv[])
{

  int ret;
  int opt;
  int bufsize = BUFFER_SIZE;
  char *dev=NULL;
  int baudrate=115200;
  int parity=0;
  int stop=1;
  int bits=8;
  bool hw =false;
  while((opt=getopt(argc,argv,"d:b:p:s:w:f"))!=-1){
    switch(opt){
    case 'd':
      dev = optarg;
      break;
    case 'b':
      baudrate = atoi(optarg);
      break;
    case 'p':
      parity = atoi(optarg);
      break;
    case 's':
      stop= atoi(optarg);
      break;
    case 'w':
      bits= atoi(optarg);
      break;
    case 'f':
      hw= true;
      break;
      
    default:
      printf("Usage is: %s <-d serial dev> [-b baudrate] [-p parity] [-s stop] [-w bits] [-f]\n-f: enable control flow HW\n",argv[0]);

      return 0;
    }

  }

  if(dev==NULL){
    printf("## you must specify a valid device \n");
    return -1;
  }

  printf("* dev: %s baudrate:%d parity:%d bits:%d stop:%d control flow hw:%s\n",dev,baudrate,parity,bits,stop,hw?"NO":"YES");
  
  int comm = popen_serial(bufsize,dev,baudrate,parity,bits,stop,hw);
  //  PosixSerialComm* comm=new PosixSerialComm(dev,atoi(baudrate.c_str()),atoi(parity.c_str()),atoi(bits.c_str()),atoi(stop.c_str()));
  if(comm>=0){
    char buffer[BUFFER_SIZE];

    while(1){
      int timeout=0;
      int bytes=pread_serial_count(comm);
      if(bytes<=0){
	usleep(1);
	continue;
      }
      ret=  LVread_serial(comm,buffer,bytes,-1,&timeout);
      if(timeout>0){
	printf("Read TIMEOUT ret = %d\n",ret);
      }
      if(ret>0){
	printf("received %d bytes\n",ret);
	if(!strncmp(buffer,"*quit*",6)){
	  ret =  LVwrite_serial(comm,(void*)"*quit*",6,-1,&timeout);
	  pclose_serial(comm);
	  return 0;
	}
	ret=  LVwrite_serial(comm,buffer,ret,1000,&timeout);
	if(ret>0){
	  printf("sent %d bytes\n",ret);
	} 
	if(timeout>0){
	  printf("Write Timeout ret = %d\n",ret);
	}
	
      }
    }
  
    pclose_serial(comm);
  }
  // test your libserial library here

  return 0;
}
