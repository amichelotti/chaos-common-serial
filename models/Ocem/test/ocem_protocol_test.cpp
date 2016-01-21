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
int send_command(common::serial::ocem::OcemProtocol*oc,int id,char*_cmd){
    char stringa[256];
    int ret;
    uint64_t tm,tot;
    int timeout=0;
    sprintf(stringa,"%s",_cmd);
    tm = common::debug::getUsTime();
     ret=oc->select(id,stringa,1000,&timeout);
     tot=common::debug::getUsTime()-tm;
     if(ret>0){
	printf("[%llu] in %lld OK ret:%d timeout %d \"%s\" \n",tm,tot,ret,timeout,_cmd);
     } else {
         printf("[%llu]in %lld ## BAD ret:%d timeout %d \"%s\" \n",tm,tot,ret,timeout,_cmd);
     }
     return ret;
    
}
static int poll_wrapped(common::serial::ocem::OcemProtocol*oc,int id,char*buf,int size,int*timp,int* error_p,int*busy,int*nodata,int*byte_recv){
    int retp;
    retp=oc->poll(id,buf,size,1000,timp);
    DPRINT("returned %d",retp);
    switch(retp){
        case common::serial::ocem::OcemProtocol::OCEM_NO_TRAFFIC:
                  (*nodata)++;
                  return retp;
                  break;
        case common::serial::ocem::OcemProtocol::OCEM_SLAVE_BUSY:
            (*busy)++;
              return retp;

            break;
            
    }
    if(retp>0){
	      (*byte_recv)+=retp;
         
    } else {
        (*error_p)++;
    }
    
        return retp;  
    
}
static void printRawCommandHelp(){
    std::cout<<"\tSELECT <OCEMID> <CMD>      : perform a select"<<std::endl;
    std::cout<<"\tPOLL <OCEMID>: perform a poll and dump result"<<std::endl;
    std::cout<<"\tLOOP <OCEMID> <#times>: perform a loop of polls and dump result"<<std::endl;
    std::cout<<"\tSETCURRENT <OCEMID> <current> <max current> <sensibility>: set current"<<std::endl;
    std::cout<<"\tINIT               : initialize "<<std::endl;
    std::cout<<"\tHELP               : this help"<<std::endl;
    std::cout<<"\tQUIT               : quit program"<<std::endl;
  
}

#define PARSE(OP,params) if((!strcmp(tok[0],OP))&& (toks==params))


void raw_test(common::serial::ocem::OcemProtocol*oc){
  char stringa[1024];
  char buf[1024];
  char *tok[16];
  int toks=0;
   int byte_recv=0;
   int busy=0;
   int nodata=0;
   int tims=0;
   uint64_t now;
  if(oc->init()!=0 ){
    printf("## cannot initialize protocol\n");
    return;
  }
  printRawCommandHelp();
  while(gets(stringa)){
      uint64_t tm,tot;
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
	  send_command(oc,id,tok[2]);
	  continue;
	}
	PARSE("POLL",2){
	  id=atoi(tok[1]);
	  *buf=0;
	  tm = common::debug::getUsTime();
	  
	  ret=oc->poll(id,buf,sizeof(buf),5000,&timeout);
	  tot_time=common::debug::getUsTime()-tm;
	  printf("[%llu] in %lld us ret:%d timeout %d \"%s\" \n",tm,tot_time,ret,timeout,buf);
	  continue;
	  
	}
         PARSE("INIT",1){
         }
           
        PARSE("SETCURRENT",5){
            char cmd[256];
            int rets,tims;
           id=atoi(tok[1]);
           float curr=atof(tok[2]);
           float maxcurr=atof(tok[3]);
           float sense=atof(tok[4]);
           int val=((curr/maxcurr)*65535.0);
           int vals=((sense/maxcurr)*65535.0);
	   int error_s=0,error_p=0,timeout_p=0,timeout_s=0;
           int cnt=0,cntt=0;
           printf("* setting current threashold \n");
           if(vals==0){
               vals=4095;
               sprintf(cmd,"TH I0 %.5d",vals);
           } else{
            sprintf(cmd,"TH I0 %.5d",vals);
           }
           if(send_command(oc,id,cmd)<0)continue;

           snprintf(cmd,sizeof(cmd),"SP %.7d",val);
           printf("* setting current at %f (max %f) [0x%x] \"%s\"\n",curr,maxcurr,val,cmd);

           if(send_command(oc,id,cmd)<0)continue;

           if(send_command(oc,id,"STR")<0)continue;
           nodata=0;
           busy=0;
           error_p=0;
           tims=0;
           byte_recv=0;
           int retry=0;
           while(1){
               //printf("[%lld] polling busy:%d nodata:%d, other errors:%d, tot bytes:%d\n",tot,buf,busy,nodata,error_p,byte_recv);
               tm = common::debug::getUsTime();
               if(vals==4095){
                    if(send_command(oc,id,"COR")<0)continue;

               }
               if((cntt%1000)==0){
                    printf("[%lld] polling  %d, busy:%d nodata:%d, other errors:%d, tot bytes:%d\n",tm,cntt,busy,nodata,error_p,byte_recv);
               }
             
               rets=poll_wrapped(oc,id,buf,sizeof(buf),&tims,&error_p,&busy,&nodata,&byte_recv);
               now = common::debug::getUsTime();
               tot=now-tm;
              
               cntt++;
               if(rets>0){
                   printf("[%lld] in %lld \"%s\" busy:%d nodata:%d, other errors:%d, tot bytes:%d\n",now,tot,buf,busy,nodata,error_p,byte_recv);
                   cnt++;
               } else {
                   retry++;
               }
               if((retry>1000)){
                   printf("[%lld] in %lld  exiting from loop busy:%d nodata:%d, other errors:%d, tot bytes:%d\n",now,tot,busy,nodata,error_p,byte_recv);
                   break;
               }
           }

        }
	PARSE("LOOP",3){
	    int rets,retp,timp;
	    int error_s=0,error_p=0,timeout_p=0,timeout_s=0;
	  id=atoi(tok[1]);
	  int looptimes=atoi(tok[2]);
	  int cnt=looptimes;
	 
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

