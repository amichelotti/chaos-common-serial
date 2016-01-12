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
    std::cout<<"\tPOLL <OCEMID> : perform a poll and dump result"<<std::endl;
    std::cout<<"\tHELP               : this help"<<std::endl;
    std::cout<<"\tQUIT               : quit program"<<std::endl;
  
}
void raw_test(common::serial::ocem::OcemProtocol*oc){
  char stringa[1024];
  boost::regex cmd_match("^(\\w+) (\\d+)(\\s+(.+)|)");
  if(oc->init()!=0 ){
    printf("## cannot initialize protocol\n");
    return;
  }
  printRawCommandHelp();
  while(gets(stringa)){
      uint64_t tm;
      char *t=stringa;
      boost::smatch match;
      convToUpper(t);

      tm = common::debug::getUsTime();
      if(boost::regex_match(std::string(t),match,cmd_match,boost::match_perl)){
	int ret;
          std::string op=match[1];
	std::string ids=match[2];
	int id = atoi(ids.c_str());
	if(op == "SELECT"){
	  int timeout=0;
	  std::string cmd=match[4];
	  ret=oc->select(id,(char*)cmd.c_str(),5000,&timeout);
	  if(ret<0){
	    printf("## error sending ret:%d, timeout :%d\n",ret,timeout);
	  }
	} else if(op == "POLL"){
	  int timeout=0;
	  char buf[1024];
	  *buf=0;
	  ret=oc->poll(id,buf,sizeof(buf),5000,&timeout);
	  if(ret<0){
	    printf("## error polling ret:%d, timeout %d\n",ret,timeout);
	  } else {
	    //char outbuf[1024];
	    //oc->decodeBuf(buf,outbuf,sizeof(outbuf));
	    printf("[%d] '%s'\n",ret,buf);
	  }
	}
      } else if(!strcmp(t,"QUIT")){
	return;
      } else if(!strcmp(t,"HELP")){
	printRawCommandHelp();
	return;
      }
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
 
    common::serial::ocem::OcemProtocolBuffered* oc= new common::serial::ocem::OcemProtocolBuffered(dev.c_str());
    oc->init();
   raw_test(oc);
   delete oc;
  printf("* OK\n");
  return 0;
}

