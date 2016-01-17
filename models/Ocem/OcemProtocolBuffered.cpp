//
//  OcemProtocolBuffered.cpp
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#include "OcemProtocolBuffered.h"
#include <common/debug/core/debug.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

using namespace common::serial::ocem;

int OcemProtocolBuffered::consumeWriteCommand(int max_command) {
    int ret                            = 0;
    int consume_message_number_to_send = max_command;

    //consume write command
    while (queue_ocem_write.empty() != 0 &&
        --consume_message_number_to_send >= 0) {
        Request current_cmd;
        int     timeo = 0;
        int     size  = 0;
        int     cnt   = 0;
        queue_ocem_write.front(current_cmd);
        queue_ocem_write.pop();
        uint64_t when = common::debug::getUsTime() - current_cmd.timestamp;
        DPRINT(
            "[%d] scheduling WRITE (%d/%d/%d) , cmd queue %d, SENDING command \"%s\", timeout %d, issued %llu us ago",
            current_cmd.serial_id,
            queue_ocem_write.req_ok,
            queue_ocem_write.req_bad,
            queue_ocem_write.reqs,
            size,
            current_cmd.command_list.front().c_str(),
            current_cmd.timeo_ms,
            when);

        for(std::vector<std::string>::iterator cmd_it = current_cmd.command_list.begin();
            cmd_it != current_cmd.command_list.end();
            cmd_it++) {
            std::string cmd_msg = *cmd_it;
            if((ret = OcemProtocol::select(current_cmd.serial_id,
                                           (char *)cmd_msg.c_str(),
                                           current_cmd.timeo_ms)) > 0){
                queue_ocem_write.req_ok++;
                DPRINT("[%d] command ok queue lenght %d", current_cmd.serial_id, queue_ocem_write.size());
            }
            else {
                break;
            }
        }
    }
    return ret;
}

void *OcemProtocolBuffered::runSchedule() {
    char buffer[2048];
    DPRINT("THREAD STARTED 0x%x", pthread_self());
    run = 1;
    int ret                            = 0;
    int consume_message_number_to_send = 2;
    int consecutive_read_operation     = 2;
    while(run) {
        do {
            for(std::set<int>::iterator slv_it = slave_id.begin();
                slv_it != slave_id.end();
                slv_it++) {
                Request current_read;
                int     curr_slv_id = *slv_it;
                current_read.timestamp = common::debug::getUsTime();

                for(int idx = 0;
                    idx < consecutive_read_operation;
                    consecutive_read_operation--) {
                    ret = OcemProtocol::poll(curr_slv_id, buffer, sizeof(buffer), 1000);
                    current_read.serial_id = curr_slv_id;
                    current_read.ret       = ret;
                    if(ret > 0){
                        std::string received_msg(buffer, ret);
                        current_read.command_list.push_back(received_msg);
                        queue_ocem_read.push(current_read);
                        queue_ocem_read.req_ok++;
                        pthread_cond_signal(&queue_ocem_read.awake);
                        DPRINT("[%d] scheduling READ ( %d/%d/%d crc err %d), queue %d, ret %d data:\"%s\"",
                               curr_slv_id,
                               queue_ocem_read.req_ok,
                               queue_ocem_read.req_bad,
                               queue_ocem_read.reqs,
                               queue_ocem_read.crc_err,
                               queue_ocem_read.queue.size(),
                               ret,
                               buffer);
                    }
                    else if(ret == OCEM_POLL_ANSWER_CRC_FAILED){
                        int size = (sizeof(buffer) < (strlen(buffer) + 1)) ? sizeof(buffer) : (strlen(buffer) + 1);
                        if(size > 0){
                            std::string received_msg(buffer, size);
                            current_read.command_list.push_back(received_msg);
                            current_read.ret = size;
                        }
                        queue_ocem_read.crc_err++;
                        queue_ocem_read.req_bad++;
                    }
                    else if(ret == OCEM_NO_TRAFFIC){
                        //we have no traffic from sid so we go ahead
                        break;
                    }
                }

                //before to pass to another sid for read give time to send data
                if(queue_ocem_write.empty() != 0){consumeWriteCommand(1);}
            }
            // give time to send two data
            if(queue_ocem_write.empty() != 0){consumeWriteCommand(2);}
        }
        while ((ret > 0) && --consume_message_number_to_send);
    }
    DPRINT("EXITING SCHEDULE THREAD");
}

void *OcemProtocolBuffered::schedule_thread(void *p) {
    OcemProtocolBuffered *pnt = (OcemProtocolBuffered *)p;
    return (void *)pnt->runSchedule();
}

OcemProtocolBuffered::OcemProtocolBuffered(const char *serdev,
                                           int max_answer_size,
                                           int baudrate,
                                           int parity,
                                           int bits,
                                           int stop)
    : OcemProtocol(serdev,
                   max_answer_size,
                   baudrate,
                   parity,
                   bits,
                   stop) {
    initialized = 0;
    pthread_mutex_init(&mutex_buffer, NULL);

}

OcemProtocolBuffered::~OcemProtocolBuffered() {
    deinit();
}

int OcemProtocolBuffered::getWriteSize() {
    int result_size = 0;
    pthread_mutex_lock(&mutex_buffer);
    result_size = queue_ocem_write.size();
    pthread_mutex_unlock(&mutex_buffer);
    return result_size;
}
int OcemProtocolBuffered::getReadSize() {
    int result_size = 0;
    pthread_mutex_lock(&mutex_buffer);
    result_size     = queue_ocem_read.size();
    pthread_mutex_unlock(&mutex_buffer);
    return result_size;
}
int OcemProtocolBuffered::registerSlave(int slaveid) {
    int slaves = 0;
    pthread_mutex_lock(&mutex_buffer);
    slave_id.insert(slaveid);
    slaves     = slave_id.size();
    pthread_mutex_unlock(&mutex_buffer);
    return slaves;
}

int OcemProtocolBuffered::unRegisterAll() {
    slave_id.clear();
    return 0;
}

int OcemProtocolBuffered::unRegisterSlave(int slaveid) {
    int slaves = 0;
    pthread_mutex_lock(&mutex_buffer);
    slave_id.erase(slaveid);
    slaves = slave_id.size();
    pthread_mutex_unlock(&mutex_buffer);
    return slaves;
}

int OcemProtocolBuffered::poll(int &slave_id,
                               char *buf,
                               int size,
                               int timeo,
                               int *timeoccur) {

    if(timeoccur)*timeoccur = 0;

    DPRINT("Pool queue lenght %d ", queue_ocem_read.size());
    if(queue_ocem_read.empty()){
        if(wait_timeo(&queue_ocem_read.awake,
                      &queue_ocem_read.qmutex,
                      timeo) < 0){
            if(timeoccur)*timeoccur = 1;
            DPRINT("Timeout elapsed %d, no traffic",
                   timeo);
            return OCEM_NO_TRAFFIC;
        }
    }

    if(!queue_ocem_read.empty()){
        Request req;
        queue_ocem_read.front(req);
        queue_ocem_read.pop();
        slave_id = req.serial_id;
        DPRINT("[%d] poll queue %d returned \"%s\"", slave_id, queue_ocem_read.size(), req.command_list.front().c_str());
        memcpy(buf, req.command_list.front().c_str(), req.command_list.front().size() + 1);
        return req.ret;
    }
    return 0;
}
int OcemProtocolBuffered::wait_timeo(pthread_cond_t *cond, pthread_mutex_t *mutex_, int timeo_ms) {
    int ret;
    if(timeo_ms > 0){
        struct timespec ts;
        struct timeval  tv;
        pthread_mutex_lock(mutex_);
        gettimeofday(&tv, NULL);
        ts.tv_sec  = tv.tv_sec + timeo_ms / 1000;
        ts.tv_nsec = tv.tv_usec * 1000 + (timeo_ms % 1000) * 1000000;
        DPRINT("waiting on %x for %d", cond, timeo_ms);
        if(pthread_cond_timedwait(cond, mutex_, &ts) != 0){
            pthread_mutex_unlock(mutex_);

            return -1000;
        }
        DPRINT("exiting from wait on %x for %d", cond, timeo_ms);
        pthread_mutex_unlock(mutex_);
        return 0;
    }

    DPRINT("indefinite wait on %x", cond);
    ret = pthread_cond_wait(cond, mutex_);
    pthread_mutex_unlock(mutex_);

    DPRINT("exiting from indefinite wait on %x", cond);
    return ret;
}
int OcemProtocolBuffered::select(int &slave_id,
                                 const std::vector<std::string> &messages,
                                 int timeo,
                                 int *timeoccur) {
    int err = 0;

    if(timeoccur)*timeoccur = 0;

    // check if the command is still pending
    DPRINT("[%d] pushing command list of size  \"%s\"", slave_id, messages.size());

    Request data;
    data.command_list = messages;
    data.timeo_ms     = timeo;
    data.timestamp    = ::common::debug::getUsTime();
    queue_ocem_write.push(data);
    return data.command_list.size();

}

int OcemProtocolBuffered::select(int slave_id, const char *command, int timeo, int *timeoccur) {
    if(timeoccur)*timeoccur = 0;


    DPRINT("[%d] pushing command list of size  \"%s\"", slave_id, strlen(command));
    Request data;
    data.command_list.push_back(std::string(command, strlen(command)));
    data.timeo_ms  = timeo;
    data.timestamp = ::common::debug::getUsTime();
    queue_ocem_write.push(data);
    return strlen(command);
}

int OcemProtocolBuffered::stop() {
    int *ret;
    DPRINT("STOP THREAD 0x%x", rpid);
    run = 0;
    pthread_join(rpid, (void **)&ret);
    return 0;
}
int OcemProtocolBuffered::start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if(pthread_create(&rpid, &attr, schedule_thread, this) < 0){
        DERR("cannot create schedule_thread thread");
        return -1;
    }
    DPRINT("START THREAD 0x%x", rpid);

    usleep(10000);
}
int OcemProtocolBuffered::init() {


    if(initialized)return 0;
    int ret = OcemProtocol::init();

    initialized = 1;
    return ret;
}

int OcemProtocolBuffered::deinit() {
    int rett;
    stop();
    unRegisterAll();
    rett        = OcemProtocol::deinit();
    initialized = 0;
    return rett;
}

int OcemData::push(Request &req) {
    uint64_t tm = common::debug::getUsTime();
    last_req_time = common::debug::getUsTime();
    pthread_mutex_lock(&qmutex);
    reqs++;
    queue.push(req);
    pthread_mutex_unlock(&qmutex);
    return reqs;
}

int OcemData::pop() {
    int ret;
    pthread_mutex_lock(&qmutex);
    if(!queue.empty()){
        queue.pop();
        reqs++;
    }
    ret = queue.size();
    pthread_mutex_unlock(&qmutex);
    return ret;
}

int OcemData::size() {
    int ret;
    pthread_mutex_lock(&qmutex);
    ret = queue.size();
    pthread_mutex_unlock(&qmutex);

    return ret;
}

int OcemData::front(Request &req) {
    int ret;
    pthread_mutex_lock(&qmutex);
    if(queue.empty()){
        pthread_mutex_unlock(&qmutex);
        return -1;
    }
    req = queue.front();
    ret = queue.size();
    pthread_mutex_unlock(&qmutex);

    return ret;
}

int OcemData::empty() {
    int ret;
    pthread_mutex_lock(&qmutex);
    ret = queue.empty();

    pthread_mutex_unlock(&qmutex);

    return ret;
}
int OcemData::back(Request &req) {
    int ret;
    pthread_mutex_lock(&qmutex);
    if(queue.empty()){
        pthread_mutex_unlock(&qmutex);
        return -1;
    }
    req = queue.back();
    ret = queue.size();
    pthread_mutex_unlock(&qmutex);

    return ret;
}

OcemData::OcemData() {
    last_req_time = 0;
    req_ok        = 0;
    reqs          = 0;
    req_bad       = 0;
    crc_err       = 0;
    pthread_attr_t     attr;
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&awake, &cond_attr);
    pthread_mutex_init(&qmutex, NULL);

}