//
//  OcemProtocolBuffered.cpp
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#include "OcemProtocolBuffered.h"
#include <common/debug/core/debug.h>
using namespace common::serial::ocem;



void* OcemProtocolBuffered::runSchedule(){
    ocem_queue_t::iterator i;
    char buffer[2048];
    DPRINT("THREAD STARTED 0x%x",pthread_self());
    OcemData*read_queue,*write_queue;
    run=1;
    while(run){

      for(i=slave_queue.begin();i!=slave_queue.end();i++){
	int ret,timeo,size;
	write_queue=(i->second).second;

	pthread_mutex_lock(&write_queue->mutex);
	size=write_queue->queue.size();
	DPRINT("[%d] scheduling WRITE, cmd queue %d",i->first);
	// handle select
	if(!write_queue->queue.empty()){
	  int cnt;

	  Request& cmd=write_queue->queue.front();
	  uint64_t when=common::debug::getUsTime()-cmd.timestamp;
	  
	  DPRINT("[%d] SENDING command %s, timeout %d, issued %llu us ago",i->first,cmd.buffer.c_str(),cmd.timeo_ms,when);
	  ret=OcemProtocol::select(i->first,(char*)cmd.buffer.c_str(),cmd.timeo_ms);

	  write_queue->reqs++;
	  if(ret>0){
	    write_queue->queue.pop();
	    write_queue->req_ok++;
	    DPRINT("[%d] command ok queue lenght %d",i->first,write_queue->queue.size());
	  } else {
	    cmd.retry++;
	    if(cmd.retry>2){
	      ERR("[%d] removing not working command, retries %d",i->first,cmd.retry);
	      write_queue->queue.pop();
	    }
	    DPRINT("[%i] command not removed, retry later on",i->first);
	  }
	}
	
	if((size == MAX_WRITE_QUEUE) && (write_queue->queue.size()<MAX_WRITE_QUEUE)){
	  pthread_cond_signal(&write_queue->awake);
	}
	pthread_mutex_unlock(&write_queue->mutex);
      }
      
      for(i=slave_queue.begin();i!=slave_queue.end();i++){
	int ret,timeo,rdper=READ_PER_WRITE;
	read_queue=(i->second).first;
	DPRINT("[%d] scheduling READ, queue %d",i->first,read_queue->queue.size());
      // handle poll

	pthread_mutex_lock(&read_queue->mutex);
	do{
	  ret=OcemProtocol::poll(i->first,buffer,sizeof(buffer),1000);
	  if(ret>0){
	    Request pol;
	    pol.buffer.assign(buffer,ret);
	    pol.timestamp=common::debug::getUsTime();
	    read_queue->pushRequest(pol);
	    DPRINT("[%d] received queue %d data \"%s\", ret %d",i->first,read_queue->queue.size(),buffer,ret);
	  }
	} while((ret>0)&& --rdper);
	pthread_mutex_unlock(&read_queue->mutex);
      }

    }
    DPRINT("EXITING SCHEDULE THREAD");
}

 void* OcemProtocolBuffered::schedule_thread(void* p){
    OcemProtocolBuffered* pnt = (OcemProtocolBuffered*)p;
    return (void*)pnt->runSchedule();
}

OcemProtocolBuffered::OcemProtocolBuffered(const char*serdev,int max_answer_size,int baudrate,int parity,int bits,int stop):OcemProtocol(serdev,max_answer_size,baudrate,parity,bits,stop){
    slaves=0;
    initialized=0;
}

OcemProtocolBuffered::~OcemProtocolBuffered(){
  deinit();
}
            
int OcemProtocolBuffered::registerSlave(int slaveid){
    // create if not present;
    if(slave_queue.find(slaveid)==slave_queue.end()){
      pthread_attr_t attr;
      pthread_condattr_t cond_attr;
      pthread_condattr_init(&cond_attr);




        OcemData* d= new OcemData();
        OcemData* s= new OcemData();
	pthread_cond_init(&d->awake,&cond_attr);
	pthread_cond_init(&s->awake,&cond_attr);
	pthread_mutex_init(&d->mutex,NULL);
	pthread_mutex_init(&s->mutex,NULL);
        DPRINT("registering slave %d",slaveid);
	
        slave_queue.insert(std::make_pair<int,std::pair<OcemData*,OcemData* > >(slaveid,std::make_pair<OcemData*,OcemData*>(d,s)));
    } else {
       // DPRINT("already registered slave %d",slaveid);
       return 0; 
    }
    
   
    return ++slaves;
}

int OcemProtocolBuffered::unRegisterAll(){
 ocem_queue_t::iterator i;
 for(i=slave_queue.begin();i!=slave_queue.end();i++){
     unRegisterSlave(i->first);
 }
 return 0;
}

int OcemProtocolBuffered::unRegisterSlave(int slaveid){
    ocem_queue_t::iterator i;
    
    i=slave_queue.find(slaveid);
    if(i==slave_queue.end()){
        ERR("cannot find slave %d",slaveid);
        return -3;
    }
     DPRINT("Unregistering slave %d",slaveid);

    delete ((i->second).first);
    delete ((i->second).second);
    slave_queue.erase(i);
    return slaves--;
}

int OcemProtocolBuffered::poll(int slaveid,char * buf,int size,int timeo,int*timeoccur){
 
    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    if(timeoccur)*timeoccur=0;
    OcemData*read_queue=(i->second).first;
    DPRINT("[%d] pool queue lenght %d ",slaveid,read_queue->queue.size());
    pthread_mutex_lock(&read_queue->mutex);

    if(!read_queue->queue.empty()){
        Request req;
        read_queue->popRequest(req);
        DPRINT("[%d] poll queue %d returned \"%s\"",slaveid,read_queue->queue.size(),req.buffer.c_str());
        memcpy(buf,req.buffer.c_str(),req.buffer.size()+1);
	pthread_mutex_unlock(&read_queue->mutex);
        return req.buffer.size();
    }
    pthread_mutex_unlock(&read_queue->mutex);

    return 0;
}
  int OcemProtocolBuffered::wait_timeo(pthread_cond_t* cond,pthread_mutex_t*mutex,int timeo_ms){
  int ret;
  if(timeo_ms>0){
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    ts.tv_sec=tv.tv_sec + timeo_ms/1000;
    ts.tv_nsec=tv.tv_usec*1000 + (timeo_ms%1000)*1000000;
    DPRINT("waiting on %x for %d",cond,timeo_ms);
    if(pthread_cond_timedwait(cond, mutex, &ts)!=0){
            return -1000;
    }
    DPRINT("exiting from wait on %x for %d",cond,timeo_ms);
    return 0;
  }
  DPRINT("indefinite wait on %x",cond);
  ret = pthread_cond_wait(cond, mutex);
  DPRINT("exiting from indefinite wait on %x",cond);
  return ret;
}        
int OcemProtocolBuffered::select(int slaveid,char* command,int timeo,int*timeoccur){

    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    if(timeoccur)*timeoccur=0;
    OcemData*write_queue=(i->second).second;
    pthread_mutex_lock(&write_queue->mutex);
    #if 0
    if(write_queue->queue.size()>0){
        Request& last_cmd=->queue.back();
        std::string topush;
        topush.assign(command);
        if( last_cmd.buffer == topush){
            DPRINT("slave %d not pushing replicated command \"%s\"",slaveid,command);
            pthread_mutex_unlock(&schedule_write_mutex);

            return strlen(command); 
        }
    }
#endif
    if(write_queue->queue.size()>=MAX_WRITE_QUEUE){
        DPRINT("[%d] WAIT for cmd queue reduce %d",slaveid,(i->second).second->queue.size());
        if(wait_timeo(&write_queue->awake,&write_queue->mutex,0)<0){
           pthread_mutex_unlock(&write_queue->mutex);
           DERR("[%d] too many commands on queue %d",slaveid,write_queue->queue.size());
           return 0;
        }
    }
    DPRINT("[%d] pushing command \"%s\"",slaveid,command);
    Request data;
    data.buffer.assign(command);
    data.timeo_ms=timeo;
    data.timestamp=::common::debug::getUsTime();
    write_queue->pushRequest(data);
    pthread_mutex_unlock(&write_queue->mutex);

    return strlen(command);
}
            
int OcemProtocolBuffered::init(){
  pthread_attr_t attr;

    if(initialized)return 0;
    int ret=OcemProtocol::init();
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    if(pthread_create(&rpid,&attr,schedule_thread,this)<0){
      DERR("cannot create schedule_thread thread");
      return -1;
    }
    initialized=1;
    return ret;
}

int OcemProtocolBuffered::deinit(){
    int* ret;
    int rett;
    unRegisterAll();
    run=0;
    pthread_join(rpid,(void**)&ret);
    rett=OcemProtocol::deinit();
    initialized=0;
    return rett;
}

  int OcemData::pushRequest(Request &req){
    uint64_t tm=common::debug::getUsTime();
    last_req_time=common::debug::getUsTime();
    reqs++;
    queue.push(req);  
    return reqs;
  }
  
  int OcemData::popRequest(Request& req){
      int ret;
      if(queue.empty()){
          return -1;
      }
      req=queue.front();
      queue.pop();
      reqs--;
      return queue.size();
      
  }
