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
    run=1;
    while(run){
    }
}

 void* OcemProtocolBuffered::schedule_thread(void* p){
    OcemProtocolBuffered* pnt = (OcemProtocolBuffered*)p;
    return (void*)pnt->runSchedule();
}

OcemProtocolBuffered::OcemProtocolBuffered(const char*serdev,int max_answer_size,int baudrate,int parity,int bits,int stop):OcemProtocol(serdev,max_answer_size,baudrate,parity,bits,stop){
    slaves=0;
}

OcemProtocolBuffered::~OcemProtocolBuffered(){
}
            
int OcemProtocolBuffered::registerSlave(int slaveid){
    // create if not present;
    if(slave_queue.find(slaveid)==slave_queue.end()){
        OcemData* d= new OcemData();
        OcemData* s= new OcemData();
        DPRINT("registering slave %d",slaveid);
        slave_queue.insert(std::make_pair<int,std::pair<OcemData*,OcemData* > >(slaveid,std::make_pair<OcemData*,OcemData*>(d,s)));
    } else {
        DPRINT("already registered slave %d",slaveid);
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
    pthread_mutex_lock(&schedule_mutex);
    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    std::string data;
    data.assign(buf,size);
    (i->second).first->pushRequest(data,timeo);
    if(timeoccur)*timeoccur=0;
    pthread_mutex_unlock(&schedule_mutex);
    return 0;
}
           
int OcemProtocolBuffered::select(int slaveid,char* command,int timeo,int*timeoccur){
    pthread_mutex_lock(&schedule_mutex);
    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);

    std::string data;
    data.assign(command);
    (i->second).second->pushRequest(data,timeo);
    if(timeoccur)*timeoccur=0;
    pthread_mutex_unlock(&schedule_mutex);

    return 0;
}
            
int OcemProtocolBuffered::init(){
     pthread_attr_t attr;
    pthread_condattr_t cond_attr;
    int ret=OcemProtocol::init();
    
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&awake,&cond_attr);

    pthread_mutex_init(&schedule_mutex,NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    if(pthread_create(&rpid,&attr,schedule_thread,this)<0){
      DERR("cannot create schedule_thread thread");
      return -1;
    }
    
    return ret;
}

int OcemProtocolBuffered::deinit(){
    int* ret;
    unRegisterAll();
    run=0;
    pthread_join(rpid,(void**)&ret);
    return OcemProtocol::deinit();
}

  int OcemData::pushRequest(std::string& buf, uint32_t timeo){
    uint64_t tm=common::debug::getUsTime();
    common::serial::ocem::Request rq;
    rq.buffer=buf;
    rq.timestamp=common::debug::getUsTime();
    last_req_time=rq.timestamp;
    reqs++;
    queue.push(rq);  
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