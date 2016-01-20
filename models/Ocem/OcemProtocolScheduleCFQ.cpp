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
        
 
}


void* OcemProtocolScheduleCFQ::runSchedule(){
    ocem_queue_sorted_t::iterator i;
    char buffer[2048];
    DPRINT("THREAD STARTED 0x%x",pthread_self());
    OcemData*read_queue,*write_queue;
    uint64_t now;
    int timeo=0;
    run=1;
    while(run){
    //  DPRINT("PROTOCOL SCHEDULE");
      std::sort(slave_queue_sorted.begin(),slave_queue_sorted.end(),algo_sort_write);
#if 0
      for(i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
          DPRINT("SHEDULE WRITE order %d",i->first);
      }
#endif

      for(i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
	int ret,timeo,size;
	write_queue=(i->second).second;
	size=write_queue->size();
	// handle select
	if(size>0){
	//  do{
	  int cnt;
	  Request cmd;
          write_queue->old_req_time=cmd.timestamp;
          write_queue->front(cmd);
          write_queue->pop();

          now=common::debug::getUsTime();
	  uint64_t when=now-cmd.timestamp;
	  DPRINT("[%d] scheduling WRITE (%d/%d/%d) last op %llu us ago, cmd queue %d oldest req %llu ago, SENDING command \"%s\", timeout %d, issued %llu us ago",i->first,write_queue->req_ok,write_queue->req_bad,write_queue->reqs,now-write_queue->last_op,size,now-write_queue->old_req_time,cmd.buffer.c_str(),cmd.timeo_ms,when);

	  ret=OcemProtocol::select(i->first,(char*)cmd.buffer.c_str(),10000,&timeo);
          write_queue->last_op=now;
	  if(ret>0){
            
	    write_queue->req_ok++;
	    DPRINT("[%d] command \"%s\" ok queue lenght %d, ret=%d timeo %d",i->first,(char*)cmd.buffer.c_str(),write_queue->size(),ret,timeo);
	    size--;

	  } else {
	    cmd.retry++;
             

            write_queue->req_bad++;
            DPRINT("[%i] command \"%s\" ERROR(errs %d) , retry later on, ret=%d timeo=%d",i->first,(char*)cmd.buffer.c_str(),write_queue->req_bad,ret,timeo);
	    /*if(cmd.retry>2){
	      ERR("[%d] removing not working command, retries %d",i->first,cmd.retry);
	      write_queue->push(cmd);

	      size--;
	    }
	    DPRINT("[%i] command not removed, retry later on",i->first);
             */
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

      for(i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
    int ret,timeo,size;
      int rdper=READ_PER_WRITE;
	read_queue=(i->second).first;
      
	do{
          Request pol;
          now=common::debug::getUsTime();

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
	    DPRINT("[%d] scheduling READ ( %d/%d/%d crc err %d), queue %d oldest updated %llu, ret %d data:\"%s\"",i->first,read_queue->req_ok,read_queue->req_bad,read_queue->reqs,read_queue->crc_err,read_queue->queue.size(),now-read_queue->old_req_time,ret,buffer);
          } else if(ret==OCEM_POLL_ANSWER_CRC_FAILED){
              int size=(sizeof(buffer)<(strlen(buffer)+1))?sizeof(buffer):(strlen(buffer)+1);
              pol.buffer.assign(buffer,size);
              if(size>0){
                  pol.ret=size;
              }
              read_queue->push(pol);
              read_queue->crc_err++;
              read_queue->req_bad++;
          } else {
                read_queue->req_bad++;

          }
	} while((ret>0)&& --rdper);
      } 

    }   
    DPRINT("EXITING SCHEDULE THREAD");
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

}

OcemProtocolScheduleCFQ::~OcemProtocolScheduleCFQ(){
  deinit();
  
}
            
int OcemProtocolScheduleCFQ::getWriteSize(int slave){
       pthread_mutex_lock(&mutex_buffer);
       
        ocem_queue_t::iterator i=slave_queue.find(slave);
        OcemData*write_queue=(i->second).second;
        pthread_mutex_unlock(&mutex_buffer);
        return write_queue->size();
}
int OcemProtocolScheduleCFQ::getReadSize(int slave){
     pthread_mutex_lock(&mutex_buffer);
     ocem_queue_t::iterator i=slave_queue.find(slave);
     OcemData*read_queue=(i->second).first;
     pthread_mutex_unlock(&mutex_buffer);
     return read_queue->size();
}
int OcemProtocolScheduleCFQ::registerSlave(int slaveid){
    // create if not present;
    pthread_mutex_lock(&mutex_buffer);
    if(slave_queue.find(slaveid)==slave_queue.end()){
      


        OcemData* d= new OcemData();
        OcemData* s= new OcemData();
	DPRINT("registering slave %d",slaveid);
	
        slave_queue.insert(std::make_pair<int,std::pair<OcemProtocolScheduleCFQ::OcemData*,OcemProtocolScheduleCFQ::OcemData* > >(slaveid,std::make_pair<OcemProtocolScheduleCFQ::OcemData*,OcemProtocolScheduleCFQ::OcemData*>(d,s)));
        slave_queue_sorted.push_back(std::make_pair<int,std::pair<OcemProtocolScheduleCFQ::OcemData*,OcemProtocolScheduleCFQ::OcemData* > >(slaveid,std::make_pair<OcemProtocolScheduleCFQ::OcemData*,OcemProtocolScheduleCFQ::OcemData*>(d,s)));
    } else {
        pthread_mutex_unlock(&mutex_buffer);
       // DPRINT("already registered slave %d",slaveid);
       return 0; 
    }
    pthread_mutex_unlock(&mutex_buffer);
   
    return ++slaves;
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
    pthread_mutex_lock(&mutex_buffer);

    i=slave_queue.find(slaveid);
    if(i==slave_queue.end()){
        ERR("cannot find slave %d",slaveid);
         pthread_mutex_unlock(&mutex_buffer);

        return -3;
    }
     DPRINT("Unregistering slave %d",slaveid);
    stop();
    delete ((i->second).first);
    delete ((i->second).second);
    slave_queue.erase(i);
    for(ocem_queue_sorted_t::iterator i=slave_queue_sorted.begin();i!=slave_queue_sorted.end();i++){
        if(i->first == slaveid){
            slave_queue_sorted.erase(i);
        }
    }
    if(slave_queue.size()){
        start();
    }
    
    pthread_mutex_unlock(&mutex_buffer);

    return slaves--;
}

int OcemProtocolScheduleCFQ::poll(int slaveid,char * buf,int size,int timeo,int*timeoccur){
 
    //registerSlave(slaveid);
     pthread_mutex_lock(&mutex_buffer);

    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    OcemData*read_queue=(i->second).first;
    pthread_mutex_unlock(&mutex_buffer);

    if(timeoccur)*timeoccur=0;
    
    DPRINT("[%d] pool queue lenght %d ",slaveid,read_queue->size());
    bool empty;
    empty=read_queue->empty();
    if(empty){
        if(wait_timeo(&read_queue->awake,&read_queue->qmutex,timeo)<0){
           if(timeoccur)*timeoccur=1;
           DPRINT("[%d] Timeout elapsed %d, no traffic",slaveid,timeo);
           return OCEM_NO_TRAFFIC;
        }
    }
    empty=read_queue->empty();
    
    if(!empty){
        Request req;
        read_queue->front(req);
        read_queue->pop();
        read_queue->old_req_time=req.timestamp;
       
        DPRINT("[%d] poll queue %d returned \"%s\"",slaveid,read_queue->size(),req.buffer.c_str());
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
    DPRINT("waiting on %x for %d",cond,timeo_ms);
    if(pthread_cond_timedwait(cond, mutex_, &ts)!=0){
            pthread_mutex_unlock(mutex_);

            return -1000;
    }
    DPRINT("exiting from wait on %x for %d",cond,timeo_ms);
    pthread_mutex_unlock(mutex_);

    return 0;
  }
  DPRINT("indefinite wait on %x",cond);
  ret = pthread_cond_wait(cond, mutex_);
  pthread_mutex_unlock(mutex_);

  DPRINT("exiting from indefinite wait on %x",cond);
  return ret;
}        
int OcemProtocolScheduleCFQ::select(int slaveid,char* command,int timeo,int*timeoccur){

    //registerSlave(slaveid);
    pthread_mutex_lock(&mutex_buffer);
    ocem_queue_t::iterator i=slave_queue.find(slaveid);
    OcemData*write_queue=(i->second).second;
    pthread_mutex_unlock(&mutex_buffer);

    if(timeoccur)*timeoccur=0;
    int size=write_queue->size();
    if(size>0){
        Request last_cmd;
        write_queue->back(last_cmd);
        std::string topush;
        topush.assign(command);
        if( last_cmd.buffer == topush){
            DPRINT("slave %d not pushing replicated command \"%s\"",slaveid,command);
            return strlen(command); 
        }
    }
    
    if(size>=MAX_WRITE_QUEUE){
        DPRINT("[%d] WAIT for cmd \"%s\" queue reduce %d",slaveid,command,size);
        if(wait_timeo(&write_queue->awake,&write_queue->qmutex,0)<0){
           DERR("[%d] too many commands on queue %d",slaveid,size);
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

    DPRINT("[%d] pushing command \"%s\"",slaveid,command);
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

    DPRINT("STOP THREAD 0x%x",rpid);
    run=0;
    pthread_join(rpid,(void**)&ret);
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
  
  DPRINT("START THREAD 0x%x",rpid);
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
          
 int OcemProtocolScheduleCFQ::OcemData::front(Request& req){
       int ret;
      pthread_mutex_lock(&qmutex);
      if(queue.empty()){
         pthread_mutex_unlock(&qmutex);
          return -1;
      }
      req=queue.front();
      ret=queue.size();
      pthread_mutex_unlock(&qmutex);

      return ret;
 }
 
          
 int OcemProtocolScheduleCFQ::OcemData::empty(){
       int ret;
      pthread_mutex_lock(&qmutex);
      ret=queue.empty();
      
      pthread_mutex_unlock(&qmutex);

      return ret;
 }
 int OcemProtocolScheduleCFQ::OcemData::back(Request& req){
      int ret;
      pthread_mutex_lock(&qmutex);
      if(queue.empty()){
         pthread_mutex_unlock(&qmutex);
          return -1;
      }
      req=queue.back();
      ret=queue.size();
      pthread_mutex_unlock(&qmutex);

      return ret;
 }
 
 OcemProtocolScheduleCFQ::OcemData::OcemData(){
     last_req_time=0;req_ok=0;reqs=0;req_bad=0;crc_err=0;
     old_req_time=0;

     pthread_attr_t attr;
     pthread_condattr_t cond_attr;
     pthread_condattr_init(&cond_attr);
        pthread_cond_init(&awake,&cond_attr);
	pthread_mutex_init(&qmutex,NULL);
	
        

 }