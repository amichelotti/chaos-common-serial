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
            int ret,timeo;
            read_queue=(i->second).first;
            write_queue=(i->second).second;
            DPRINT("- scheduling slave %d, cmd queue %d, output queue %d",i->first,write_queue->queue.size(),read_queue->queue.size());

            // handle select
            if(!write_queue->queue.empty()){
               
                pthread_mutex_lock(&schedule_write_mutex);

                Request& cmd=write_queue->queue.front();
                uint64_t when=common::debug::getUsTime()-cmd.timestamp;
                DPRINT("- slave %d sending command %s, timeout %d, issued %llu us ago",i->first,cmd.buffer.c_str(),cmd.timeo_ms,when);
                ret=OcemProtocol::select(i->first,(char*)cmd.buffer.c_str(),cmd.timeo_ms);

                write_queue->reqs++;
                if(ret>0){
                    write_queue->queue.pop();
                    write_queue->req_ok++;
                } else {
                    cmd.retry++;
                    if(cmd.retry>2){
                        ERR("removing not working command, retries %d",cmd.retry);
                        write_queue->queue.pop();
                    }
                    DPRINT("command not removed, retry later on");
                }
                pthread_mutex_unlock(&schedule_write_mutex);

            }
            
            // handle poll
            ret=OcemProtocol::poll(i->first,buffer,sizeof(buffer),1000);

            if(ret>0){
               DPRINT("- received from slave %d, data \"%s\", ret %d",i->first,buffer,ret);

               pthread_mutex_lock(&schedule_read_mutex);
               Request pol;
               pol.buffer.assign(buffer,ret);
               pol.timestamp=common::debug::getUsTime();
               read_queue->pushRequest(pol);
               pthread_mutex_unlock(&schedule_read_mutex);

            }
        }
    }
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
    pthread_mutex_lock(&schedule_read_mutex);
    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    if(timeoccur)*timeoccur=0;

    OcemData*read_queue=(i->second).first;
    DPRINT("slave %d queue lenght %d ",slaveid,read_queue->queue.size());

    if(!read_queue->queue.empty()){
        Request req;
        read_queue->popRequest(req);
        DPRINT("slave %d returning \"%s\"",slaveid,req.buffer.c_str());
        memcpy(buf,req.buffer.c_str(),req.buffer.size()+1);
        pthread_mutex_unlock(&schedule_read_mutex);

        return req.buffer.size();
    }
    pthread_mutex_unlock(&schedule_read_mutex);
    return 0;
}
           
int OcemProtocolBuffered::select(int slaveid,char* command,int timeo,int*timeoccur){
    pthread_mutex_lock(&schedule_write_mutex);
    registerSlave(slaveid);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    DPRINT("slave %d pushing command \"%s\"",slaveid,command);
    Request data;
    data.buffer.assign(command);
    data.timeo_ms=timeo;
    data.timestamp=::common::debug::getUsTime();
    (i->second).second->pushRequest(data);
    if(timeoccur)*timeoccur=0;
    pthread_mutex_unlock(&schedule_write_mutex);

    return 0;
}
            
int OcemProtocolBuffered::init(){
     pthread_attr_t attr;
    pthread_condattr_t cond_attr;
    if(initialized)return 0;
    int ret=OcemProtocol::init();
    
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&awake,&cond_attr);

    pthread_mutex_init(&schedule_read_mutex,NULL);
    pthread_mutex_init(&schedule_write_mutex,NULL);

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
