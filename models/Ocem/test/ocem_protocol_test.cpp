//
//  test_serial.cpp
// 
//
//
//
//


#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <common/serial/models/Ocem/OcemProtocolBuffered.h>
#include <common/debug/core/debug.h>
#include <boost/regex.hpp>
#include <string>
#ifdef CHAOS
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#endif
#define DEFAULT_TIMEOUT 10000
using boost::regex;

static char* convToUpper(char*str){
  char *b = str;
  char *tmp=str;
  if(str==NULL) return NULL;
  while((*str!=0)){
    *tmp=toupper(*str);
    tmp++,str++;
  }
  *tmp=0;
  return b;
}


static void printRawCommandHelp(){
    std::cout<<"\tSELECT <OCEMID> <CMD>      : perform a select"<<std::endl;
    std::cout<<"\tPOLL <OCEMID>: perform a poll and dump result"<<std::endl;
    std::cout<<"\tLOOP <OCEMID> <#times>: perform a loop of polls and dump result"<<std::endl;

    std::cout<<"\tHELP               : this help"<<std::endl;
    std::cout<<"\tQUIT               : quit program"<<std::endl;
  
}

#define PARSE(OP,params) if((!strcmp(tok[0],OP))&& (toks==params))
void raw_test(common::serial::ocem::OcemProtocol*oc){
  char stringa[1024];
  char buf[1024];
  char *tok[16];
  int toks=0;
  if(oc->init()!=0 ){
    printf("## cannot initialize protocol\n");
    return;
  }
  printRawCommandHelp();
  while(gets(stringa)){
      uint64_t tm;
      char *t=stringa;
      boost::smatch match;
      toks=0;
      convToUpper(t);
      tok[0]=strtok(stringa," ");
      while((toks<sizeof(tok))&&(tok[toks]!=0)) {
	tok[++toks]=strtok(NULL," ");
      } 
      if(toks>0){
      	int ret;
	int id;
        char *op=tok[0];
	uint64_t tot_time;
	int timeout=0;
	PARSE("SELECT",3){

	  id=atoi(tok[1]);
	  tm = common::debug::getUsTime();
	  ret=oc->select(id,(char*)tok[2],5000,&timeout);
	  tot_time=common::debug::getUsTime()-tm;
	  if(ret<0){
	    printf("## [%llu us] error sending ret:%d, timeout :%d\n",tot_time,ret,timeout);
	  } else {
	    printf("[%llu us] done ret %d\n",tot_time,ret);
          }
	  continue;
	}
	PARSE("POLL",2){
	  id=atoi(tok[1]);
	  *buf=0;
	  tm = common::debug::getUsTime();
	  
	  ret=oc->poll(id,buf,sizeof(buf),5000,&timeout);
	  tot_time=common::debug::getUsTime()-tm;
	  printf("[%llu us] ret:%d timeout %d \"%s\" \n",tot_time,ret,timeout,buf);
	  continue;
	  
	}
	PARSE("LOOP",3){
	    int rets,retp,tims,timp;
	    int error_s=0,error_p=0,timeout_p=0,timeout_s=0;
	    uint64_t tot=0;
	  id=atoi(tok[1]);
	  int looptimes=atoi(tok[2]);
	  int cnt=looptimes;
	  int byte_recv=0;
	  int busy=0;
	  int nodata=0;
	  tm = common::debug::getUsTime();
	  while(cnt--){

	    rets=oc->select(id,"COR",1000,&tims);
	    retp=oc->poll(id,buf,sizeof(buf),1000,&timp);
	    if((retp!=common::serial::ocem::OcemProtocol::OCEM_SLAVE_BUSY)&&(retp!=common::serial::ocem::OcemProtocol::OCEM_NO_TRAFFIC) && (retp<0)){
	      error_p++;
	    } else if(retp==common::serial::ocem::OcemProtocol::OCEM_SLAVE_BUSY){
	      busy++;
	    } else if(retp==common::serial::ocem::OcemProtocol::OCEM_NO_TRAFFIC){
	      nodata++;
	    } else if(retp>0){
	      byte_recv+=retp;
	    }
	    if(rets<0){
	      error_p++;
	    }

          }
          tot= common::debug::getUsTime()-tm;
          printf("* done in %llu us %f cycle, error poll %d (busy %d, no data %d, payload %d) error select %d timeout pool %d timeout select %d \n",tot,tot*1.0/looptimes,error_p,busy,nodata,byte_recv,error_s,timeout_p,timeout_s);
	  continue;
	}
	PARSE("QUIT",1){
	  return;
	}
	PARSE("HELP",1){
	  printRawCommandHelp();
	  return;
	}
      }
      printf("## syntax error in \"%s\" [%d toks]\n",stringa,toks);
  }
}

static int check_for_char(){
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0,&fds);
  struct timeval tm;
  tm.tv_sec = 0;
  tm.tv_usec = 0;

  select(1,&fds,0,0,&tm);
  return FD_ISSET(0,&fds);
  
}

int main(int argc, char *argv[])
{

    std::string ver;
    std::string dev;
 
#ifdef CHAOS
    
      chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption("dev,d", po::value<std::string>(&dev), "The serial device /dev/ttyxx");
      
      chaos::ui::ChaosUIToolkit::getInstance()->init(argc, argv);


#else
    boost::program_options::options_description desc("options");
    
  desc.add_options()("help", "help");
  // put your additional options here
  desc.add_options()("dev", boost::program_options::value<std::string>(), "serial device where the ocem is attached");
     if(vm.count("dev")==0){
        std::cout<<"## you must specify a valid device"<<desc<<std::endl;
     return -1;
    }
   slave_id = vm["id"].as<int>();
   dev = vm[dev].as<std::string>();
   if(vm.count("interactive"))
       interactive=true;
#endif
  //////
 
    common::serial::ocem::OcemProtocol* oc= new common::serial::ocem::OcemProtocol(dev.c_str());
    oc->init();
   raw_test(oc);
   delete oc;
  printf("* OK\n");
  return 0;
}

