//
//  OcemProtocol.cpp
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#include "OcemProtocol.h"
#include <sstream>
#include <unistd.h>
#include <string.h>
using namespace common::serial;

OcemProtocol::OcemProtocol(const char*_serdev,int max,int _baudrate,int _parity,int _bits, int _stop):serdev(_serdev),max_answer_size(max),baudrate(_baudrate),parity(_parity),bits(_bits),stop(_stop)
{
  DPRINT("Creating dev %s, max answer %d , baudrate %d, parity %d, bits %d, stop %d\n",serdev,max_answer_size,baudrate,parity,bits,stop);
  serial = new PosixSerialComm(std::string(_serdev),_baudrate,_parity,_bits,_stop,max,max);
    
}

OcemProtocol::~OcemProtocol(){
    deinit();
}

int OcemProtocol::init(){
  int ret =0; 
  pthread_mutex_init(&serial_chan_mutex, NULL);
  pthread_mutex_lock(&serial_chan_mutex);
  if(serial){
    if(serial->init()!=0){
      ret = OCEM_CANNOT_OPEN_DEVICE;	
    }
    pthread_mutex_unlock(&serial_chan_mutex);
    return ret;
  } else {
    serial =new PosixSerialComm(std::string(serdev),baudrate,parity,bits,stop,max_answer_size,max_answer_size);
    DPRINT("OcemProtocol Creating new PosixSerial Comm handler x%x\n",serial);
    if(serial){
      if(serial->init()!=0){
	DERR("cannot open OCEM device\n");
	ret= OCEM_CANNOT_OPEN_DEVICE;
      }
      pthread_mutex_unlock(&serial_chan_mutex);
      return ret;
    }
  }
  pthread_mutex_unlock(&serial_chan_mutex);
  return OCEM_CANNOT_INITALIZE;
}
int OcemProtocol::deinit(){
    if(serial){
      delete serial;
    }
    serial = NULL;
    return 0;
}

int OcemProtocol::sendAck(int ackType,int timeo){
    char buf = ackType;
    int ret;
    int timeocc=0;
    DPRINT("sending ACK:\"%d\"\n",ackType);
    if((ret = serial->write(&buf,1,timeo))>0){
        if (ackType==ACK){
             DPRINT("waiting ACK from slave\n");

             if((ret= serial->read(&buf,1,timeo,&timeocc))>0){
                 int rett;
                 rett =(buf==EOT)?0:OCEM_EOT_MISSING_FROM_SLAVE;
                 if(rett == 0){
                     DPRINT("OK received EOT from slave\n");
                 } else {
                     DERR("no EOT from slave received:\"%d\"\n",buf);

                 }
                 return rett;
             } else {
                 DERR("no ack received in %d ms, return ret %d, timeout %d\n",timeo,ret,timeocc);
                 return OCEM_NO_ACK_FROM_SLAVE;
                 
             }
         }
    } else {
        DERR("error writing ACK \"%d\"\n",ackType);
        return OCEM_ERROR_WRITING_ACK;
    }
    return 0;
}
int OcemProtocol::check_and_extract(char *buffer, char *protbuf, int size){
    char *pntd= buffer;
    char *pnts= protbuf;
    char crc=0;
    int cnt=0;
    *pntd=0;
    while((cnt<size) && (cnt<max_answer_size)&& (pnts[cnt]!=ETX)){
      crc^=pnts[cnt];
      if(cnt>0){
	*pntd++=pnts[cnt];
      }
      cnt++;
    }
    *pntd=0;
    if(pnts[cnt]==ETX){
      crc^=pnts[cnt];
      crc|=0x80;
      if(pnts[cnt+1] == crc){
	DPRINT("crc check ok, message length %d\n",cnt-1);
	return cnt-1;
      } else {
	DERR("crc check FAILED, calc 0x%x, expected 0x%x\n",crc,pnts[cnt+1]);
	return OCEM_POLL_ANSWER_CRC_FAILED;
	
      }
    } else {
      DERR(" no ETX mactched size msg %d\n",cnt);
      return OCEM_NO_ETX_MATCHED;
      
    }
return 0;
}
int OcemProtocol::poll(int slave,char * buf,int size,int timeo,int*timeoccur){
    char bufreq[4];
    int timeor=0,timeow=0;
    int ret;
    char tmpbuf[max_answer_size];
    int tot;
    if(slave<0 || slave >31){
        DERR("invalid slave id %d\n",slave);
        return OCEM_BAD_SLAVEID;
    }
    pthread_mutex_lock(&serial_chan_mutex);
    DPRINT(" performing poll request slave %d, timeout %d ms\n",slave,timeo);
    bufreq[0]=ENQ;
    bufreq[1]=slave+ 0x40;
    /**
     potential error
    if(serial->byte_available_read()>0){
    }
     */
    if((ret= serial->write(bufreq,2,timeo,&timeow))!=2) {
        DERR(" error writing slave %d ret %d, timeocc %d \n",slave,ret,timeow);
        if(timeoccur)*timeoccur=timeow;
        pthread_mutex_unlock(&serial_chan_mutex);

        return OCEM_WRITE_FAILED;
    }
    
    DPRINT(" waiting answer from %d\n",slave);
    ret = waitAck(timeo);
    if(ret == EOT){
        DPRINT("slave %d says NO TRAFFIC\n",slave);
        pthread_mutex_unlock(&serial_chan_mutex);
	usleep(100000); // sleep
        return OCEM_NO_TRAFFIC;
    } else if(ret == STX){
        int slave_rec;
	int fetch=1;
        // answer with data
        tot = 0;
        //get the rest of the message
        DPRINT("slave %d has something, receiving...\n",slave);
        while((tot<max_answer_size)&&((ret=serial->read(&tmpbuf[tot],1,timeo,&timeor))>0)&&(fetch<2)){
	  if(tmpbuf[tot]==ETX){
	    DPRINT("termination found %d characters\n",tot+1);
	    fetch++;
	  }
	  tot++;	    
        }
        slave_rec= tmpbuf[0] - 0x40;
        if(timeoccur)*timeoccur=timeor;
        DPRINT("received %d bytes from %d [%d] (expected %d[%d]) , \n",tot,slave_rec,tmpbuf[0],slave,bufreq[1]);
        if(ret<0){
            DERR(" error reading message from slave %d ret %d, timeocc %d \n",slave,ret,timeor);
            pthread_mutex_unlock(&serial_chan_mutex);

            return OCEM_READ_FAILED;
        }
        if(tmpbuf[0]!=bufreq[1]){
            DERR("Answer from slave %d (%d) instead of %d, byte read %d\n",slave_rec,tmpbuf[0],slave,tot);
            sendAck(EOT, timeo);
            pthread_mutex_unlock(&serial_chan_mutex);
            return OCEM_UNEXPECTED_SLAVE_ANSWER;
        }
        
        if(tmpbuf[tot-1]==ETX){
	  DPRINT("valid terminator found crc x%x, checking and extracting message\n",tmpbuf[tot]);
            if((ret= check_and_extract(buf,&tmpbuf[0], size<tot?size:tot))>0){
                int retack;
                DPRINT("slave %d says:\"%s\"\n",slave,buf);
                retack=sendAck(ACK, timeo);
                pthread_mutex_unlock(&serial_chan_mutex);
                if(retack==0)
                    return ret;
                return retack;
            } else {
                DERR("Answer contains errors, slave %d says:\"%s\", ret %d\n",slave,buf,ret);
                //send busy
                sendAck(EOT, timeo);
                pthread_mutex_unlock(&serial_chan_mutex);

                return ret;
            }
        } else {
            DERR("missing termination character reading message, byte read %d\n",tot);
            //send busy
            sendAck(EOT, timeo);
            pthread_mutex_unlock(&serial_chan_mutex);

            return OCEM_MALFORMED_POLL_ANSWER;

        }
    }
    
    DERR("no answer from slave %d within %d ms, timeouccur %d\n",slave,timeo,timeor);
    pthread_mutex_unlock(&serial_chan_mutex);
    return ret;
    
}

void OcemProtocol::decodeBuf(char*buf,char*outbuf,int size){
  int stat=0;
  int cnt;
  std::stringstream msg;
  char stringa[1024];
  *stringa=0;
  for(cnt=0;(cnt<max_answer_size) && (stat < 4);cnt++){
    if(buf[cnt]==STX){
      msg<< "<STX>";
      stat++;
    } else if(buf[cnt]==ETX){
       msg<< "<ETX>";
      stat++;
    } else  if( stat==1){
      msg<<"<"<<std::hex<<int(buf[cnt])<<">";
      stat++;
    } else  if( stat==2){
      msg.put(buf[cnt]);
    } else if(stat == 3){
      msg <<"<"<<std::hex<<int(buf[cnt])<<">";
      stat++;
    }
  }

  DPRINT("%s\n",msg.str().c_str());
  if(outbuf){
    strncpy(outbuf,msg.str().c_str(),size);
  }

}

int OcemProtocol::build_cmd(int slave,char*protbuf,char* cmd){
  int cnt,cntt;
    char crc=0;
    protbuf[0]=STX;
    protbuf[1] = slave + 0x60;
    for(cnt=0,cntt=0;(cnt<max_answer_size) && cmd&& (cmd[cntt]!=0);cnt++){
        if(cnt>1){
            protbuf[cnt] = cmd[cntt];
	    cntt++;
        }
        if(cnt>0)
	  crc^=protbuf[cnt];
    }
    protbuf[cnt++] = ETX;
    crc^=ETX|0x80;
    protbuf[cnt++] = crc;
    DPRINT("msg size %d crc x%x\n",cnt,crc);


    return cnt;
}

int OcemProtocol::waitAck(int timeo){
    int timeor=0;
    char tmpbuf;
    int ret;
    DPRINT(" waiting ack from slave\n");
    if((ret=serial->read(&tmpbuf,sizeof(tmpbuf),timeo,&timeor))<0) {
        DERR(" error reading from slave ret %d, timeocc %d \n",ret,timeor);
        return OCEM_READ_FAILED;
    }

    DPRINT("received %d [x%x] bytes answer, timeoccur %d\n",ret,tmpbuf,timeor);
    
    if(ret == 0){
        DERR("no answer from slave within %d ms, timeoccur %d\n",timeo,timeor);
        return OCEM_NO_ACK_FROM_SLAVE;
    }
    
    if(ret==1){
        if((tmpbuf==EOT) || (tmpbuf==ACK )){
            DPRINT("slave returned \"%s\"\n",(tmpbuf==ACK)?"ACK":"BUSY");
            return tmpbuf;
        }
        if(tmpbuf==STX){
            DPRINT("slave returned STX, wants to talk\n");
            return tmpbuf;
        }
	if(tmpbuf==NAK){
	  DPRINT("slave returned NAK\n");
	  return tmpbuf;
	}
        DERR("malformed answer \"%d\"\n",tmpbuf);
    }
    
    return OCEM_UNEXPECTED_SLAVE_ANSWER;
}

int OcemProtocol::select(int slave,char* command,int timeo,int*timeoccur){
    char tmpbuf[max_answer_size];
    char bufreq[2];
    int timeow=0;
    int ret;
    if(slave<0 || slave >31){
        DERR("invalid slave id %d\n",slave);
        return OCEM_BAD_SLAVEID;
    }
    pthread_mutex_lock(&serial_chan_mutex);
    DPRINT(" performing select request slave %d timeout %d ms\n",slave,timeo);

    bufreq[0]=ENQ;
    bufreq[1]=slave+ 0x60;


    if((ret= serial->write(bufreq,sizeof(bufreq),timeo,&timeow))!=2) {
      DERR(" error writing slave %d ret %d, timeocc %d \n",slave,ret,timeow);
      if(timeoccur)*timeoccur=timeow;
      pthread_mutex_unlock(&serial_chan_mutex);
      return OCEM_WRITE_FAILED;
    }
    
    if((ret=waitAck(timeo))!=ACK){

      //      usleep(100000); // sleep

      if(ret == NAK){
	DERR("slave not ready, sent NACK on selection\n");
	pthread_mutex_unlock(&serial_chan_mutex);
	return OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE;
      } else if(ret== EOT){
	DPRINT("slave busy\n");
	pthread_mutex_unlock(&serial_chan_mutex);
	return OCEM_SLAVE_BUSY;
      }
      DERR("slave unexpected answer on selection\n");
      pthread_mutex_unlock(&serial_chan_mutex);
      return OCEM_UNEXPECTED_SLAVE_ANSWER;
    }
    
    DPRINT("slave %d is waiting a command\n",slave);
    ret = build_cmd(slave,tmpbuf,command);
    if((ret>2) && (ret<max_answer_size)){
        int rett;
        if((rett= serial->write(tmpbuf,ret,timeo,&timeow))!=ret) {
            DERR(" error writing slave %d ret %d, timeocc %d \n",slave,rett,timeow);
            if(timeoccur)*timeoccur=timeow;
            pthread_mutex_unlock(&serial_chan_mutex);

            return OCEM_WRITE_FAILED;
        }
        
        DPRINT(" command \"%s\" sent to slave %d\n",command,slave);
        DPRINT("waiting answer to the command from %d\n",slave);
        
        if((rett=waitAck(timeo))!=ACK){
	  //	  usleep(100000); // sleep
	  if(ret == NAK){
	    DERR("slave cannot accept message %d\n",OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE);
	    pthread_mutex_unlock(&serial_chan_mutex);
	    return OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE;
	  } else if(ret== EOT){
	    DPRINT("slave busy\n");
	    pthread_mutex_unlock(&serial_chan_mutex);
	    return OCEM_SLAVE_BUSY;
	  }
	  DERR("slave unexpected answer\n");
	  pthread_mutex_unlock(&serial_chan_mutex);
	  return OCEM_UNEXPECTED_SLAVE_ANSWER;

	  return rett;
        }
        pthread_mutex_unlock(&serial_chan_mutex);
        DPRINT("command sent successfully to slave %d\n",slave);
        return ret;
    }
    DERR("bad select command\n");
    pthread_mutex_unlock(&serial_chan_mutex);

    return OCEM_BAD_SELECT_COMMAND;
}

