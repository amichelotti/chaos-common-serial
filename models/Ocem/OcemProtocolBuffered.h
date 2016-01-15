//
//  OcemProtocolBuffered.h
//  serial
//
//  Created by andrea michelotti on 10/9/13.
//  Copyright (c) 2013 andrea michelotti. All rights reserved.
//

#ifndef __serial__OcemProtocolBuffered__
#define __serial__OcemProtocolBuffered__

#ifdef OCEM_PROTOCOL_BUFFER_DEBUG
#define DEBUG
#endif
#include <common/debug/core/debug.h>

#include <iostream>
#include "OcemProtocol.h"
#include <map>
#include <queue>
#include <set>
#include <vector>

#include <>

#define MAX_WRITE_QUEUE 10
#define MAX_READ_QUEUE 32
#define READ_PER_WRITE 1
#define ERRORS_TOBE_FATAL 8
namespace common {
namespace serial {
namespace ocem {

//! is the atomic request
struct Request {
  int serial_id;
  std::vector<std::string> command_list;
  uint64_t timestamp;
  uint32_t timeo_ms;
  int32_t retry;
  int32_t ret;
  Request() :
      serial_id(0),
      timeo_ms(0),
      timestamp(0),
      retry(0),
      ret(0) { }

  int hash_data() {

  }
};

//!queue that held the commadn for every serial id
struct OcemData {
  std::queue<Request> queue;
  //! is the set that contains all the ashes for all queued datapack
  std::set<std::string> request_hashes;
  std::string second_command;
  int last_req_index = 0;
  // number of protocol errors
  int protocol_errors = 0;
  // timestamp of last request
  uint64_t last_req_time = 0;
  // average request time
  uint64_t avg_req_time = 0;
  // average time to accomplish th request
  uint64_t done_req_time = 0;
  // number of request accomplished ok
  uint64_t req_ok = 0;
  // number of request accomplished ok
  uint64_t req_bad = 0;
  uint64_t crc_err = 0;
  // number of total request
  uint64_t reqs = 0;
  pthread_mutex_t qmutex;
  pthread_cond_t awake;
  OcemData();
  int pop();
  int size();
  int empty();
  int push(Request &req);
  int front(Request &req);
  int back(Request &req);
  int has_hash(const std::string& hash);
};
class OcemProtocolBuffered:
    public OcemProtocol {
  OcemData queue_ocem_read;
  OcemData queue_ocem_write;
  pthread_mutex_t mutex_buffer;
  std::set<int> slave_id;
  static void *schedule_thread(void *);
  pthread_t rpid;
  int run;
  int initialized;
  int wait_timeo(pthread_cond_t *cond, pthread_mutex_t *mutex, int timeo_ms);
 public:

  OcemProtocolBuffered (const char *serdev,
                        int max_answer_size = 8192,
                        int baudrate = 9600,
                        int parity = 0,
                        int bits = 8,
                        int stop = 1);
  ~OcemProtocolBuffered();

  int getWriteSize();
  int getReadSize();
  int registerSlave(int slaveid);
  int unRegisterAll();
  int unRegisterSlave(int slaveid);
  void *runSchedule();

  /**
   perform a poll request toward the given slave
   @param slave slave address
   @param buf the returning buffer
   @param size max size of the returning buffer
   @param timeo timeout in milliseconds (0 indefinite wait)
   @param timeoccur return 1 if a timeout occured
   @return the number of characters read or negative for error
   */
  int poll(int& slave_id,
           char *buf,
           int size,
           int timeo = 1000,
           int *timeoccur = 0);

  /**
   perform a select request toward the given slave
   @param slave slave address
   @param command a zero terminated string containing the command
   @param timeo timeout in milliseconds (=0 indefinite wait)
   @param timeoccur return 1 if a timeout occured
   @return the number of characters of the command sent or negative for error
   */
  int select(int slave_id,
             const char *command,
             int timeo = 1000,
             int *timeoccur = 0);

  /**
   Perform the forwarding of one or more message in an atomic way
   */
  int select(int &slave_id,
             const std::vector<std::string>& messages,
             int timeo,
             int *timeoccur);

  int init();
  int start();
  int stop();
  int deinit();

};
};
};
};

#endif /* defined(__serial__OcemProtocolBuffered__) */
