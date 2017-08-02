//
//  OcemProtocolScheduleCFQ.cpp
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#include "OcemProtocolScheduleCFQ.h"
#include <common/debug/core/debug.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

using namespace common::serial::ocem;
// before long queue and more old
bool algo_sort_write(const OcemProtocolScheduleCFQ::qdata_t& q0, const OcemProtocolScheduleCFQ::qdata_t& q1){

	OcemProtocolScheduleCFQ::OcemData* qq0=(q0.second).second;
	OcemProtocolScheduleCFQ::OcemData* qq1=(q1.second).second;

	int size0,size1;
	size0=qq0->size();
	size1=qq1->size();
	if(size0>size1){
		return true;
	}
	if(size0==size1){

		return (qq0->old_req_time<qq1->old_req_time);
	}

	return false;
}

bool algo_sort_read(const OcemProtocolScheduleCFQ::qdata_t& q0, const OcemProtocolScheduleCFQ::qdata_t& q1){

	OcemProtocolScheduleCFQ::OcemData* qq0=(q0.second).second;
	OcemProtocolScheduleCFQ::OcemData* qq1=(q1.second).second;

	int size0,size1;
	size0=qq0->size();
	size1=qq1->size();
	if(size0<size1){
		return true;
	}
	if(size0==size1){

		return (qq0->old_req_time>qq1->old_req_time);
	}

	return false;
}


void* OcemProtocolScheduleCFQ::runSchedule(){
	ocem_queue_sorted_t::iterator i;
	char buffer[2048];
	DPRINT("[%s] THREAD STARTED 0x%p",serdev,(void*)pthread_self());
	OcemData*read_queue,*write_queue;
	uint64_t now;
	int timeo=0;
	run=1;
	while(run){

		pthread_mutex_lock(&mutex_slaves);
		ocem_queue_sorted_t   slave_queue_sorted(slave_queue.size());

		std::copy(slave_queue.begin(),slave_queue.end(),slave_queue_sorted.begin());
		//  DPRINT("PROTOCOL SCHEDULE");
		std::sort(slave_queue_sorted.begin(),slave_queue_sorted.end(),algo_sort_write);
		pthread_mutex_unlock(&mutex_slaves);

#if 0
		for(i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
			DPRINT("SHEDULE WRITE order %d",i->first);
		}
#endif

		for(i=slave_queue_sorted.begin();run && (i!=slave_queue_sorted.end());i++){
			int ret,timeo,size;
			write_queue=(i->second).second;
			size=write_queue->size();
			// handle select
			assert(write_queue);
		//	DPRINT("[%s,%d] SCHEDULING WRITE of slave# %d nslaves %d",serdev,i->first,i->first,slave_queue_sorted.size());

			if(size>0){
				//  do{
				int cnt;
				Request& cmd=write_queue->front();
				write_queue->old_req_time=cmd.timestamp;



				now=common::debug::getUsTime();
				if(write_queue->must_wait_to > now){
				//	DPRINT("[%s,%d] SELECT PAUSED still for %lld us",serdev,i->first,write_queue->must_wait_to - now);
					usleep(SLEEP_IF_INACTIVE*1000);

					continue;

				}
				write_queue->must_wait_to=0;
				uint64_t when=now-cmd.timestamp;
				if(cmd.retry){
					DPRINT("[%s,%d] scheduling a RETRY-%d WRITE (%llu/%llu/%llu) last op %llu us ago, cmd queue %d oldest req %llu ago, SENDING command \"%s\", timeout %d, issued %f s ago",serdev,i->first,cmd.retry,write_queue->req_ok,write_queue->req_bad,write_queue->reqs,now-write_queue->last_op,size,now-write_queue->old_req_time,cmd.buffer.c_str(),cmd.timeo_ms,when*1.0/1000000.0);
				} else {
					DPRINT("[%s,%d] scheduling WRITE (%llu/%llu/%llu) last op %llu us ago, cmd queue %d oldest req %llu ago, SENDING command \"%s\", timeout %d, issued %f s ago",serdev,i->first,write_queue->req_ok,write_queue->req_bad,write_queue->reqs,now-write_queue->last_op,size,now-write_queue->old_req_time,cmd.buffer.c_str(),cmd.timeo_ms,when*1.0/1000000.0);
				}
				ret=OcemProtocol::select(i->first,(char*)cmd.buffer.c_str(),10000,&timeo);
				write_queue->last_op=now;
				if(ret>0){

					write_queue->req_ok++;
					DPRINT("[%s,%d] command \"%s\" ok queue lenght %d, ret=%d timeo %d",serdev,i->first,(char*)cmd.buffer.c_str(),write_queue->size(),ret,timeo);
					size--;
					write_queue->pop();
					write_queue->must_wait_to= now + PAUSE_INTRA_SELECT*1000;

				} else {
					cmd.retry++;

					write_queue->req_bad++;
					write_queue->must_wait_to= now + PAUSE_SELECT_BUSY*1000;

					if(write_queue->nsuccessive_busy<3){
						write_queue->nsuccessive_busy++;
					} else {
						OcemProtocol::select(i->first,(char*)"RMT",10000,&timeo);
						write_queue->nsuccessive_busy=0;
					}
					DPRINT("[%s %i] command \"%s\" ERROR(req bad %lld) , ret=%d timeo=%d",serdev,i->first,(char*)cmd.buffer.c_str(),write_queue->req_bad,ret,timeo);
					if((cmd.retry<3) && (cmd.buffer != "SL") && (cmd.buffer!="SA")){
						ERR("[%s,%d] scheduled for retry command \"%s\", retries %d, req bad %lld, in queue %d",serdev,i->first,(char*)cmd.buffer.c_str(),cmd.retry,write_queue->req_bad,write_queue->size());
						//  write_queue->push(cmd);

					} else {
						write_queue->pop();
					}

				}
				//	  } while(size>0);
			}

			if((write_queue->size()<MAX_WRITE_QUEUE)){
				pthread_cond_signal(&write_queue->awake);
			}
		}
		//pthread_mutex_unlock(&write_queue->qmutex);
		// handle poll/////////////////

		std::sort(slave_queue_sorted.begin(),slave_queue_sorted.end(),algo_sort_read);
#if 0
		for(i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
			DPRINT("SHEDULE READ order %d",i->first);
		}
#endif

		for(i=slave_queue_sorted.begin();run && (i!=slave_queue_sorted.end());i++){
			int ret,timeo,size;
			int rdper=READ_PER_WRITE;
			now=common::debug::getUsTime();
	//		DPRINT("[%s,%d] SCHEDULING READ of slave #%d nslaves %d",serdev,i->first,i->first,slave_queue_sorted.size());
			read_queue=(i->second).first;
			write_queue=(i->second).second;
			assert(read_queue);
			assert(write_queue);

			if(read_queue->must_wait_to > now){
			//	DPRINT("[%s,%d] POLL PAUSED still for %lld us",serdev,i->first,read_queue->must_wait_to - now);
				usleep(SLEEP_IF_INACTIVE*1000);
				continue;
			}
			read_queue->must_wait_to=0;
			do{
				Request pol;

				pol.timestamp=now;

				ret=OcemProtocol::poll(i->first,buffer,sizeof(buffer),1000);
				read_queue->last_op=now;

				pol.ret=ret;
				if(ret>0){
					pol.buffer.assign(buffer,ret);
					read_queue->last_req_time=pol.timestamp;

					read_queue->push(pol);
					read_queue->req_ok++;
					pthread_cond_signal(&read_queue->awake);
					DPRINT("[%s,%d] scheduling READ ( %llu/%llu/%llu crc err %llu), queue %u oldest updated %llu, ret %d data:\"%s\"",serdev,i->first,read_queue->req_ok,read_queue->req_bad,read_queue->reqs,read_queue->crc_err,(unsigned)read_queue->queue.size(),now-read_queue->old_req_time,ret,buffer);
					write_queue->must_wait_to=0;
				} else if(ret==OCEM_POLL_ANSWER_CRC_FAILED){
					int size=(sizeof(buffer)<(strlen(buffer)+1))?sizeof(buffer):(strlen(buffer)+1);
					pol.buffer.assign(buffer,size);
					if(size>0){
						pol.ret=size;
					}
					read_queue->push(pol);
					read_queue->crc_err++;
					read_queue->req_bad++;
					DPRINT("[%s,%d] CRC failed crc err:%lld req bad %lld",serdev,i->first,read_queue->crc_err,read_queue->req_bad);

				} else if(ret == OCEM_NO_TRAFFIC){
					read_queue->must_wait_to= now + PAUSE_POLL_NO_DATA*1000;
					DPRINT("[%s,%d] NO traffic no pool for %d ms, wait until %lld timestamp",serdev,i->first,PAUSE_POLL_NO_DATA,now + PAUSE_POLL_NO_DATA*1000);

				} else {

					read_queue->req_bad++;

				}

			} while((ret>0)&& --rdper);
		}


	}
	pthread_mutex_unlock(&mutex_slaves);


	DPRINT("[%s] EXITING SCHEDULE THREAD",serdev);
}

void* OcemProtocolScheduleCFQ::schedule_thread(void* p){
	OcemProtocolScheduleCFQ* pnt = (OcemProtocolScheduleCFQ*)p;
	return (void*)pnt->runSchedule();
}

OcemProtocolScheduleCFQ::OcemProtocolScheduleCFQ(const char*serdev,int max_answer_size,int baudrate,int parity,int bits,int stop):OcemProtocol(serdev,max_answer_size,baudrate,parity,bits,stop){
	slaves=0;
	initialized=0;
	run=0;
	pthread_mutex_init(&mutex_buffer,NULL);
	pthread_mutex_init(&mutex_slaves,NULL);

}

OcemProtocolScheduleCFQ::~OcemProtocolScheduleCFQ(){
	deinit();

}

int OcemProtocolScheduleCFQ::getWriteSize(int slave){
	pthread_mutex_lock(&mutex_slaves);

	ocem_queue_t::iterator i=slave_queue.find(slave);
	OcemData*write_queue=(i->second).second;
	pthread_mutex_unlock(&mutex_slaves);
	return write_queue->size();
}
int OcemProtocolScheduleCFQ::getReadSize(int slave){
	pthread_mutex_lock(&mutex_slaves);
	ocem_queue_t::iterator i=slave_queue.find(slave);
	OcemData*read_queue=(i->second).first;
	pthread_mutex_unlock(&mutex_slaves);
	return read_queue->size();
}
int OcemProtocolScheduleCFQ::registerSlave(int slaveid){
	// create if not present;
	pthread_mutex_lock(&mutex_slaves);

	if(slave_queue.find(slaveid)==slave_queue.end()){



		OcemData* d= new OcemData();
		OcemData* s= new OcemData();
		DPRINT("[%s,%d] registering slave %d -> write queue @%p read_queue @%p",serdev,slaveid,slaveid,(void*)d,(void*)s);

		slave_queue.insert(std::make_pair(slaveid,std::make_pair(d,s)));
	//	slave_queue_sorted.push_back(std::make_pair (slaveid,std::make_pair(d,s)));
	} else {
		pthread_mutex_unlock(&mutex_slaves);

		DPRINT("[%s,%d] already registered slave %d",serdev,slaveid,slaveid);
		return 0;
	}
	++slaves;
	pthread_mutex_unlock(&mutex_slaves);

	return slaves;
}

int OcemProtocolScheduleCFQ::unRegisterAll(){
	ocem_queue_t::iterator i;
	for(i=slave_queue.begin();i!=slave_queue.end();i++){
		unRegisterSlave(i->first);
	}
	return 0;
}

int OcemProtocolScheduleCFQ::unRegisterSlave(int slaveid){
	ocem_queue_t::iterator i;
	stop();
	pthread_mutex_lock(&mutex_slaves);

	/*for(ocem_queue_sorted_t::iterator j=slave_queue_sorted.begin();j!=slave_queue_sorted.end();j++){
		if(j->first == slaveid){
			slave_queue_sorted.erase(j);
			break;
		}
	}*/
	i=slave_queue.find(slaveid);
	if(i==slave_queue.end()){
		ERR("cannot find slave %d",slaveid);
		pthread_mutex_unlock(&mutex_slaves);

		return -3;
	}
	DPRINT("[%s,%d] Unregistering slave %d",serdev,slaveid,slaveid);
	delete ((i->second).first);
	delete ((i->second).second);
	slave_queue.erase(i);

	if(slave_queue.size()){
		start();
	}
	if(slaves>0){
		slaves--;
	}
	pthread_mutex_unlock(&mutex_slaves);

	return slaves;
}

int OcemProtocolScheduleCFQ::poll(int slaveid,char * buf,int size,int timeo,int*timeoccur){

	//registerSlave(slaveid);
	pthread_mutex_lock(&mutex_slaves);

	ocem_queue_t::iterator i=slave_queue.find(slaveid);
	OcemData*read_queue=(i->second).first;
	pthread_mutex_unlock(&mutex_slaves);

	if(timeoccur)*timeoccur=0;

	DPRINT("[%s,%d] pool queue lenght %d ",serdev,slaveid,read_queue->size());
	bool empty;
	empty=read_queue->empty();
	if(empty){
		if(wait_timeo(&read_queue->awake,&read_queue->qmutex,timeo)<0){
			if(timeoccur)*timeoccur=1;
			DPRINT("[%s,%d] Timeout elapsed %d, no traffic",serdev,slaveid,timeo);
			return OCEM_NO_TRAFFIC;
		}
	}
	empty=read_queue->empty();

	if(!empty){
		Request req;
		req = read_queue->front();
		read_queue->pop();
		read_queue->old_req_time=req.timestamp;

		DPRINT("[%s,%d] poll queue %d returned \"%s\"",serdev,slaveid,read_queue->size(),req.buffer.c_str());
		memcpy(buf,req.buffer.c_str(),req.buffer.size()+1);
		return req.ret;
	}
	/*
	 * if((read_queue->req_ok==0)&&(read_queue->reqs-read_queue->req_ok)>=ERRORS_TOBE_FATAL){
      return -100;
    }*/
	return 0;
}
int OcemProtocolScheduleCFQ::wait_timeo(pthread_cond_t* cond,pthread_mutex_t*mutex_,int timeo_ms){
	int ret;
	if(timeo_ms>0){
		struct timespec ts;
		struct timeval tv;
		pthread_mutex_lock(mutex_);
		gettimeofday(&tv,NULL);
		ts.tv_sec=tv.tv_sec + timeo_ms/1000;
		ts.tv_nsec=tv.tv_usec*1000 + (timeo_ms%1000)*1000000;
		DPRINT("[%s] waiting on @0x%p for %d",serdev,cond,timeo_ms);
		if(pthread_cond_timedwait(cond, mutex_, &ts)!=0){
			pthread_mutex_unlock(mutex_);

			return -1000;
		}
		DPRINT("[%s] exiting from wait on 0x%p for %d",serdev,cond,timeo_ms);
		pthread_mutex_unlock(mutex_);

		return 0;
	}
	DPRINT("[%s] indefinite wait on @0x%p",serdev,cond);
	ret = pthread_cond_wait(cond, mutex_);
	pthread_mutex_unlock(mutex_);

	DPRINT("[%s] exiting from indefinite wait on @0x%p",serdev,cond);
	return ret;
}        
int OcemProtocolScheduleCFQ::select(int slaveid,char* command,int timeo,int*timeoccur){

	//registerSlave(slaveid);
	pthread_mutex_lock(&mutex_slaves);
	ocem_queue_t::iterator i=slave_queue.find(slaveid);
	OcemData*write_queue=(i->second).second;
	pthread_mutex_unlock(&mutex_slaves);

	if(timeoccur)*timeoccur=0;
	int size=write_queue->size();
	if(size>0){
		Request last_cmd;
		last_cmd=write_queue->back();
		std::string topush;
		topush.assign(command);
		if( last_cmd.buffer == topush){
			DPRINT("[%s,%d] slave %d not pushing replicated command \"%s\"",serdev,slaveid,slaveid,command);
			return strlen(command);
		}
	}

	if(size>=MAX_WRITE_QUEUE){
		DPRINT("[%s,%d] WAIT for cmd \"%s\" queue reduce %d",serdev,slaveid,command,size);
		if(wait_timeo(&write_queue->awake,&write_queue->qmutex,0)<0){
			DERR("[%s,%d] too many commands on queue %d",serdev,slaveid,size);
			/*if((write_queue->req_ok==0)&&(write_queue->reqs-write_queue->req_ok)>=ERRORS_TOBE_FATAL){
	     ERR("too many write error");
	     return -101;
	   }*/

			return 0;
		}
	}

	/* if((write_queue->req_ok==0)&&(write_queue->reqs-write_queue->req_ok)>=ERRORS_TOBE_FATAL){
      ERR("too many write error");
      return -101;
    }*/

	DPRINT("[%s,%d] pushing command \"%s\"",serdev,slaveid,command);
	Request data;
	data.buffer.assign(command);
	data.timeo_ms=timeo;
	data.timestamp=::common::debug::getUsTime();
	write_queue->last_req_time=data.timestamp;
	write_queue->push(data);
	return strlen(command);
}



int OcemProtocolScheduleCFQ::stop(){
	int* ret;
	if(run){
		DPRINT("[%s] STOPPING THREAD 0x%lx",serdev,rpid);
		run=0;
		pthread_join(rpid,(void**)&ret);
		DPRINT("[%s] STOPPED THREAD 0x%lx",serdev,rpid);
	}
	return 0;
}
int OcemProtocolScheduleCFQ::start(){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if(run==0){
		if(pthread_create(&rpid,&attr,schedule_thread,this)<0){
			DERR("cannot create schedule_thread thread");
			return -1;
		}

		DPRINT("[%s] START THREAD 0x%lx",serdev,rpid);
	}
	usleep(10000);
}
int OcemProtocolScheduleCFQ::init(){


	if(initialized)return 0;
	int ret=OcemProtocol::init();

	initialized=1;
	return ret;
}

int OcemProtocolScheduleCFQ::deinit(){
	int rett;
	stop();
	unRegisterAll();
	rett=OcemProtocol::deinit();
	initialized=0;
	return rett;
}

int OcemProtocolScheduleCFQ::OcemData::push(Request &req){
	uint64_t tm=common::debug::getUsTime();
	last_req_time=common::debug::getUsTime();
	pthread_mutex_lock(&qmutex);
	reqs++;
	queue.push(req);
	pthread_mutex_unlock(&qmutex);

	return reqs;
}

int OcemProtocolScheduleCFQ::OcemData::pop(){
	int ret;
	pthread_mutex_lock(&qmutex);
	if(!queue.empty()){
		queue.pop();
		reqs++;
	}
	ret=queue.size();
	pthread_mutex_unlock(&qmutex);

	return ret;

}
int OcemProtocolScheduleCFQ::OcemData::size(){
	int ret;
	pthread_mutex_lock(&qmutex);
	ret=queue.size();
	pthread_mutex_unlock(&qmutex);

	return ret;
}

OcemProtocolScheduleCFQ::Request& OcemProtocolScheduleCFQ::OcemData::front(){
	pthread_mutex_lock(&qmutex);
	/* if(queue.empty()){
         pthread_mutex_unlock(&qmutex);
          return -1;
      }*/
	  Request& req=queue.front();
	  pthread_mutex_unlock(&qmutex);

	  return req;
}


int OcemProtocolScheduleCFQ::OcemData::empty(){
	int ret;
	pthread_mutex_lock(&qmutex);
	ret=queue.empty();

	pthread_mutex_unlock(&qmutex);

	return ret;
}

OcemProtocolScheduleCFQ::Request& OcemProtocolScheduleCFQ::OcemData::back(){
	int ret;
	pthread_mutex_lock(&qmutex);
	/*if(queue.empty()){
         pthread_mutex_unlock(&qmutex);
          return Request();
      }*/
	Request& req=queue.back();
	pthread_mutex_unlock(&qmutex);

	return req;
}

OcemProtocolScheduleCFQ::OcemData::OcemData(){
	last_req_time=0;req_ok=0;reqs=0;req_bad=0;crc_err=0;
	old_req_time=0;
	nsuccessive_busy=0;
	must_wait_to =0;
	pthread_attr_t attr;
	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	pthread_cond_init(&awake,&cond_attr);
	pthread_mutex_init(&qmutex,NULL);



}
