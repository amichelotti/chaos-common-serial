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
#include <common/serial/core/SerialChannelFactory.h>
using namespace common::serial::ocem;

OcemProtocol::OcemProtocol(common::serial::AbstractSerialChannel_psh chan):serial(chan),max_answer_size(4096){
	DPRINT("[%s] constructor channel use count %ld",chan->getUid().c_str(),chan.use_count());
};

/*
OcemProtocol::OcemProtocol(const char*_serdev,int max,int _baudrate,int _parity,int _bits, int _stop):serdev(_serdev),max_answer_size(max),baudrate(_baudrate),parity(_parity),bits(_bits),stop(_stop)
{
	DPRINT("Creating dev %s, max answer %d , baudrate %d, parity %d, bits %d, stop %d",serdev,max_answer_size,baudrate,parity,bits,stop);
	serial = new PosixSerialComm(std::string(_serdev),_baudrate,_parity,_bits,_stop,max,max);

}
*/
OcemProtocol::~OcemProtocol(){
	DPRINT("OcemProtocol destroy");

	deinit();
	common::serial::SerialChannelFactory::removeChannel(serial);

}

int OcemProtocol::init(){
	int ret =0;
	DPRINT("OcemProtocol init '%s'",serial->getUid().c_str());
	ChaosLockGuard l(chanmutex);

	if(serial.get()){
		if(serial->init()!=0){
			ret = OCEM_CANNOT_OPEN_DEVICE;
			ERR("cannot open/initialize device %s",serial->getUid().c_str());
		}
		return ret;
	} else{
		throw std::logic_error("channel not created correctly");
	}
	/*else {
		serial =new PosixSerialComm(std::string(serial->getUid().c_str()),baudrate,parity,bits,stop,max_answer_size,max_answer_size);
		DPRINT("OcemProtocol Creating new PosixSerial Comm handler x%x",serial);
		if(serial){
			if(serial->init()!=0){
				ERR("cannot open OCEM device %s",serial->getUid().c_str());
				ret= OCEM_CANNOT_OPEN_DEVICE;
			}
			return ret;
		}
	}*/
	return ret;
}
int OcemProtocol::deinit(){
	DPRINT( "deinitializing base protocol");
	if((serial.use_count()<=2)){
		DPRINT( "deinitializing channel '%s' %p",serial->getUid().c_str(),serial.get());
		serial->deinit();
	} else {
		DPRINT( "cannot deinitialize channel '%s' %p is use %ld",serial->getUid().c_str(),serial.get(),serial.use_count());
	}
	return 0;
}

int OcemProtocol::sendAck(int ackType,int timeo,const char* cmd){
	char buf = ackType;
	int ret;
	int timeocc=0;
	DPRINT("[%s] sending ACK:\"%d\"",cmd,ackType);
	if((ret = serial->write(&buf,1,timeo))>0){
		if (ackType==ACK){
			DPRINT("[%s] waiting ACK from slave",cmd);

			if((ret= serial->read(&buf,1,timeo,&timeocc))>0){
				int rett;
				rett =(buf==EOT)?0:OCEM_EOT_MISSING_FROM_SLAVE;
				if(rett == 0){
					DPRINT("[%s] OK received EOT from slave",cmd);
				} else {
					ERR("[%s] no EOT from slave received:\"%d\"",cmd,buf);

				}
				return rett;
			} else {
				ERR("[%s] no ack received in %d ms, return ret %d, timeout %d",cmd,timeo,ret,timeocc);
				return OCEM_NO_ACK_FROM_SLAVE;

			}
		}
	} else {
		ERR("[%s] error writing ACK \"%d\"",cmd,ackType);
		return OCEM_ERROR_WRITING_ACK;
	}
	return 0;
}
int OcemProtocol::check_and_extract(char *buffer, char *protbuf, int size){
	unsigned char *pntd= (unsigned char*)buffer;
	unsigned char *pnts= (unsigned char*)protbuf;
	unsigned char crc=0;
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
			DPRINT("crc check ok, message length %d",cnt-1);
			return cnt-1;
		} else {
			ERR("crc check FAILED, calc 0x%x, expected 0x%x",crc&0xff,pnts[cnt+1]&0xff);
			/// DISABLED FOR TEST!!
			return OCEM_POLL_ANSWER_CRC_FAILED;
			//return cnt -1;
		}
	} else {
		ERR(" no ETX matched size msg %d",cnt);
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
		ERR("[%s,%d] invalid slave id %d",serial->getUid().c_str(),slave,slave);
		return OCEM_BAD_SLAVEID;
	}
	ChaosLockGuard l(chanmutex);
	DPRINT("[%s,%d] performing poll request slave %d, timeout %d ms",serial->getUid().c_str(),slave,slave,timeo);

	if(timeoccur)*timeoccur=0;


	bufreq[0]=ENQ;
	bufreq[1]=slave+ 0x40;
	/**
     potential error
    if(serial->byte_available_read()>0){
    }
	 */
	serial->flush_read();
	if((ret= serial->write(bufreq,2,timeo,&timeow))!=2) {
		ERR("[%s,%d] error writing slave %d ret %d, timeocc %d ",serial->getUid().c_str(),slave,slave,ret,timeow);
		if(timeoccur)*timeoccur=timeow;

		return OCEM_WRITE_FAILED;
	}

	DPRINT("[%s,%d] waiting answer from %d",serial->getUid().c_str(),slave,slave);
	ret = waitAck(timeo);
	if(ret == EOT){
		DPRINT("[%s,%d] slave %d says NO TRAFFIC",serial->getUid().c_str(),slave,slave);
		//usleep(100000); // sleep
		return OCEM_NO_TRAFFIC;
	} else if(ret == STX){
		int slave_rec;
		int found=0;
		tot = 0;
		//get the rest of the message
		DPRINT("[%s,%d] something (%d bytes), receiving...",serial->getUid().c_str(),slave,serial->byte_available_read());
		while((tot<max_answer_size)&&(found==0)&&((ret=serial->read(&tmpbuf[tot],1,timeo,&timeor))>0)){
			if(tmpbuf[tot]==ETX){
				DPRINT("[%s,%d] termination found %d characters",serial->getUid().c_str(),slave,tot);
				found++;
			}
			tot++;
		}
		//read crc
		if(found==1){
			//read CRC
			DPRINT("[%s,%d] still %d bytes",serial->getUid().c_str(),slave,serial->byte_available_read());
			if((ret=serial->read(&tmpbuf[tot],1,timeo,&timeor))<0){
				DERR("[%s,%d] missing CRC",serial->getUid().c_str(),slave);
				serial->flush_read();
				sendAck(EOT, timeo);

				return OCEM_MALFORMED_POLL_ANSWER;
			}
		} else {
			DERR("[%s,%d] missing terminator",serial->getUid().c_str(),slave);
			serial->flush_read();
			sendAck(EOT, timeo);
			return OCEM_MALFORMED_POLL_ANSWER;

		}
		slave_rec= tmpbuf[0] - 0x40;
		if(timeoccur)*timeoccur=timeor;
		DPRINT("[%s,%d] received %d bytes from %d",serial->getUid().c_str(),slave,tot,slave_rec);
		/* if(ret<0){
            ERR(" error reading message from slave %d ret %d, timeocc %d ",slave,ret,timeor);

            return OCEM_READ_FAILED;
        }*/
		if(tmpbuf[0]!=bufreq[1]){
			ERR("[%s,%d] Answer from slave %d (%d) instead of %d, byte read %d",serial->getUid().c_str(),slave,slave_rec,tmpbuf[0],slave,tot);
			serial->flush_read();

			sendAck(EOT, timeo);
			return OCEM_UNEXPECTED_SLAVE_ANSWER;
		}


		DPRINT("[%s,%d] valid terminator found crc 0x%x, checking and extracting message",serial->getUid().c_str(),slave,tmpbuf[tot]&0xff);
		if((ret= check_and_extract(buf,&tmpbuf[0], size<tot?size:tot))>0){
			int retack;
			DPRINT("[%s,%d] slave %d says:\"%s\"",serial->getUid().c_str(),slave,slave,buf);
			retack=sendAck(ACK, timeo);
			if(retack==0)
				return ret;
			return retack;
		} else {
			ERR("[%s,%d] Answer contains errors, slave %d says:\"%s\", ret %d",serial->getUid().c_str(),slave,slave,buf,ret);
			//send busy
			sendAck(EOT, timeo);
			//sendAck(ACK, timeo)
			serial->flush_read();
			return ret;
		}

	} else if (ret==ACK){
		DPRINT("[%s,%d] ACK received",serial->getUid().c_str(),slave);
		ret=0;
	} else {

		ERR("[%s,%d] no answer from slave %d within %d ms, timeouccur %d",serial->getUid().c_str(),slave,slave,timeo,timeor);
	}
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

	DPRINT("%s",msg.str().c_str());
	if(outbuf){
		strncpy(outbuf,msg.str().c_str(),size);
	}

}

int OcemProtocol::build_cmd(int slave,char*protbuf,const char* cmd){
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
	//	DPRINT("[%s,%d] \"%s\" msg size %d crc x%x",serial->getUid().c_str(),slave,cmd,cnt,crc);


	return cnt;
}

int OcemProtocol::waitAck(int timeo){
	int timeor=0;
	char tmpbuf;
	int ret;
	//  DPRINT("[%s] waiting ack from slave",serial->getUid().c_str());
	if((ret=serial->read(&tmpbuf,sizeof(tmpbuf),timeo,&timeor))<0) {
		ERR("[%s] error reading from slave ret %d, timeocc %d ",serial->getUid().c_str(),ret,timeor);
		return OCEM_READ_FAILED;
	}

	// DPRINT("[%s] received %d [x%x] bytes answer, timeoccur %d",serial->getUid().c_str(),ret,tmpbuf,timeor);

	if(ret == 0){
		ERR("[%s] no answer from slave within %d ms, timeoccur %d",serial->getUid().c_str(),timeo,timeor);
		return OCEM_NO_ACK_FROM_SLAVE;
	}

	if(ret==1){
		if((tmpbuf==EOT) || (tmpbuf==ACK )){
			//        DPRINT("[%s] slave returned \"%s\"",serial->getUid().c_str(),(tmpbuf==ACK)?"ACK":"BUSY");
			return tmpbuf;
		}
		if(tmpbuf==STX){
			//    DPRINT("[%s] slave returned STX, wants to talk",serial->getUid().c_str());
			return tmpbuf;
		}
		if(tmpbuf==NAK){
			//  DPRINT("[%s] slave returned NAK",serial->getUid().c_str());
			return tmpbuf;
		}
		serial->flush_read();
		ERR("[%s] malformed answer \"%d\"",serial->getUid().c_str(),tmpbuf);
	}
	serial->flush_read();
	return OCEM_UNEXPECTED_SLAVE_ANSWER;
}

int OcemProtocol::select(int slave,const char* command,int timeo,int*timeoccur){
	char tmpbuf[max_answer_size];
	char bufreq[2];
	int timeow=0;
	int ret;
	if(slave<0 || slave >31){
		ERR("invalid slave id %d",slave);
		return OCEM_BAD_SLAVEID;
	}
	ChaosLockGuard l(chanmutex);

	DPRINT("[%s,%d] performing select request to send command \"%s\" to slave %d timeout %d ms",serial->getUid().c_str(),slave,command,slave,timeo);

	bufreq[0]=ENQ;
	bufreq[1]=slave+ 0x60;

	serial->flush_write();
	serial->flush_read();

	if((ret= serial->write(bufreq,sizeof(bufreq),timeo,&timeow))!=2) {
		ERR("[%s,%d] error writing slave %d ret %d, timeocc %d ",serial->getUid().c_str(),slave,slave,ret,timeow);
		if(timeoccur)*timeoccur=timeow;
		return OCEM_WRITE_FAILED;
	}

	if((ret=waitAck(timeo))!=ACK){

		//      usleep(100000); // sleep

		if(ret == NAK){
			ERR("[%s,%d] slave not ready, sent NACK on selection",serial->getUid().c_str(),slave);
			return OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE;
		} else if(ret== EOT){
			DPRINT("[%s,%d] slave busy",serial->getUid().c_str(),slave);
			return OCEM_SLAVE_BUSY;
		} else if(ret== STX){
			char buf;
			int rr;
			int count=0;
			int t;
			DPRINT("[%s,%d] slave is answering with an unexpected buffer",serial->getUid().c_str(),slave);
			while(((rr=serial->read(&buf,1,timeo,&t))>0)&& (buf!=ETX))count++;
			DPRINT("[%s,%d] read %d characters",serial->getUid().c_str(),slave,count);
		} else {
			ERR("[%s,%d] slave completely unexpected answer on selection",serial->getUid().c_str(),slave);
		}
		serial->flush_read();
		return OCEM_UNEXPECTED_SLAVE_ANSWER;
	}

	DPRINT("[%s,%d] slave %d is waiting a command",serial->getUid().c_str(),slave,slave);
	ret = build_cmd(slave,tmpbuf,command);
	if((ret>2) && (ret<max_answer_size)){
		int rett;
		if((rett= serial->write(tmpbuf,ret,timeo,&timeow))!=ret) {
			ERR("[%s,%d] error writing slave %d ret %d, timeocc %d ",serial->getUid().c_str(),slave,slave,rett,timeow);
			if(timeoccur)*timeoccur=timeow;
			return OCEM_WRITE_FAILED;
		}

		DPRINT("[%s,%d] command \"%s\" sent.... waiting answer to the command from %d",serial->getUid().c_str(),slave,command,slave);

		if((rett=waitAck(timeo))!=ACK){
			//	  usleep(100000); // sleep
			if(rett == NAK){
				ERR("[%s,%d] slave cannot accept message %d",serial->getUid().c_str(),slave,OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE);
				return OCEM_SLAVE_CANNOT_UNDERSTAND_MESSAGE;
			} else if(rett== EOT){
				DPRINT("[%s,%d] slave busy",serial->getUid().c_str(),slave);
				return OCEM_SLAVE_BUSY;
			}
			ERR("[%s,%d] slave unexpected answer :%d",serial->getUid().c_str(),slave,rett);
			// serial->flush_read();
			return OCEM_UNEXPECTED_SLAVE_ANSWER;

			return rett;
		}
		DPRINT("[%s,%d] command sent successfully to slave %d",serial->getUid().c_str(),slave,slave);
		return ret;
	}
	ERR("[%s,%d] bad select command",serial->getUid().c_str(),slave);

	return OCEM_BAD_SELECT_COMMAND;
}

