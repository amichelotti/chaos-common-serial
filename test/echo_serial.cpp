//
//  echo_serial.cpp
// 
// Andrea Michelotti
// echo over a serial line
//
//


#include "common/serial/serial.h"

using namespace common::serial;
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
static const boost::regex parse_arg("(.+),(\\d+),(\\d),(\\d),(\\d)");
#define BUFFER_SIZE 16384
int main(int argc, char *argv[])
{

  int ret;
  
  int bufsize = 8192;
  boost::smatch match;
  boost::program_options::options_description desc("options");


  desc.add_options()("help", "help");

  desc.add_options()("dev", boost::program_options::value<std::string>(), "serial dev parameters </dev/ttySxx>,<baudrate>,<parity>,<bits>,<stop>");
  desc.add_options()("buf", boost::program_options::value<int>(), "internal buffer");    
  //////
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc,argv, desc),vm);
  boost::program_options::notify(vm);
    
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }
  if(vm.count("dev")==0){
    std::cout<<"## you must specify parameters:"<<desc<<std::endl;
    return -1;
  }
  if(vm.count("buf")){
    bufsize = vm["buf"].as<int>();
  }
  std::string param = vm["dev"].as<std::string>();


  if(!regex_match(param,match,parse_arg)){
    std::cout<<"## bad parameter specification:"<<param<<" match:"<<match[0]<<std::endl;
    return -2;
  }
  std::string dev = match[1];
  std::string baudrate = match[2];
  std::string parity = match[3];
  std::string bits = match[4];
  std::string stop = match[5];
  int comm = popen_serial(bufsize,dev.c_str(),atoi(baudrate.c_str()),atoi(parity.c_str()),atoi(bits.c_str()),atoi(stop.c_str()));
  //  PosixSerialComm* comm=new PosixSerialComm(dev,atoi(baudrate.c_str()),atoi(parity.c_str()),atoi(bits.c_str()),atoi(stop.c_str()));
  if(comm>=0){
    char buffer[BUFFER_SIZE];

    while(1){
      int timeout=0;
      ret=  LVread_serial(comm,buffer,BUFFER_SIZE,-1,&timeout);
      if(timeout>0){
	  printf("Read TIMEOUT ret = %d\n",ret);
      }
      if(ret>0){
	DPRINT("received %d bytes\n",ret);
	if(!strncmp(buffer,"*quit*",6)){
	  ret =  LVwrite_serial(comm,(void*)"*quit*",6,-1,&timeout);
	  pclose_serial(comm);
	  return 0;
	}
	ret=  LVwrite_serial(comm,buffer,ret,1000,&timeout);
	if(ret>0){
	  DPRINT("sent %d bytes\n",ret);
	} 
	if(timeout>0){
	  printf("Write Timeout ret = %d\n",ret);
	}
	
      }
    }
  
    pclose_serial(comm);
  }
  // test your libserial library here

  return 0;
}
