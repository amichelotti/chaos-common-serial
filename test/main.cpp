//
//  main.cpp
// created automatically
//
//
//
//


#include "serial.h"

using namespace common::serial;
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
static const boost::regex parse_arg("(.+),(\\d+),(\\d),(\\d),(\\d)");
#define BIG_BUFFER_SIZE 1024*1024
/**
   test of byte_available
   and read conditioned to that value
 */
int test1(int size, unsigned *wptr,unsigned*rptr,PosixSerialComm* comm){
  int ret=0;
  for(int cnt=0;cnt<size;cnt++){
    int count=0,trx=0,aval;;
    wptr[cnt] = cnt;
    memset(rptr,0,(cnt+1)*4);
    printf("starting test for %d bytes\n",(cnt+1)*4);
    do {
      
      if(trx<(cnt+1)*4){
	ret= comm->write((char*)wptr + trx,(cnt+1)*4);
	DPRINT("%d] write %d/%d bytes\n",cnt , ret,(cnt+1)*4);
	if(ret>0){
	  trx+=ret;
	}
      }
      
      while((aval=comm->byte_available_read())<=0){
	usleep(10000);
      }
      DPRINT("%d] available %d bytes for read\n",cnt,aval);
      ret= comm->read((char*)rptr + count, aval);
      DPRINT("%d] read %d/%d bytes tot:%d/%d\n",cnt,ret , aval,count,(cnt+1)*4);
      if(ret!=aval){
	printf("## error reading expected %d bytes arrived %d\n",aval,ret);
	return -3;
      }
      if(ret>0){
	count+=ret;
      }
      
    } while(count<(cnt+1)*4);
    printf("testing %d data\n",cnt+1);
    for(int cntt=0;cntt<cnt+1;cntt++){
      if(rptr[cntt]!=wptr[cntt]){
	DPRINT("## Test 1 data error [%d] value %d expected %d\n",cntt,rptr[cntt],wptr[cntt]);
	return -2;
      }
	    
    }
  }
  return 0;
}
/**
    classic use of read blocking with timeout
 */
int test2(int size,unsigned *wptr,unsigned*rptr,int timeo,PosixSerialComm* comm){
  int ret=0;  
  for(int cnt=0;cnt<size;cnt++){
    int byte_recv;
    wptr[cnt] = cnt;
    DPRINT("%d] Test 2 write %d bytes\n",cnt , (cnt+1)*4);
    ret= comm->write((void*)wptr,(cnt+1)*4);
    memset(rptr,0,cnt*4);
    if(ret!=(cnt+1)*4){
      printf("## Test 2 error writing [%d] ret: %d\n",cnt,ret);
      return -1;
    }
    byte_recv=0;
    do{
      ret= comm->read((char*)rptr + byte_recv, size*4-byte_recv,timeo);
      if(ret>0){
	byte_recv+=ret;
	DPRINT("%d] read %d bytes tot %d\n",cnt , ret,byte_recv);
      } else {
	DPRINT("%d] read TIMEOUT %d\n",cnt , ret);
      }

    } while(byte_recv<(cnt+1)*4);

    for(int cntt=0;cntt<cnt+1;cntt++){
      if(rptr[cntt]!=wptr[cntt]){
	DPRINT("## Test 2 data error [%d] value %d expected %d\n",cntt,rptr[cntt],wptr[cntt]);
	return -2;
      }
	   
    }
	 
  }
  return 0;
}

/**
   BIG BUFFER TRX / RX
 */
int test3(int cycles,PosixSerialComm* comm){
  char * bigw = (char *) malloc(BIG_BUFFER_SIZE);
  char * bigr = (char *) malloc(BIG_BUFFER_SIZE);
  int randsize;
  int cnt;
  int retw,retr;
  srand(time(NULL));
  if(bigr==0 || bigw==0){
    printf("## cannot allocate big buffers\n");
    return -1;
  }
  while(cycles--){
    int trx,rx;
    memset(bigr,0,BIG_BUFFER_SIZE);
    randsize=(BIG_BUFFER_SIZE/2) + (rand()*1.0/RAND_MAX)* (BIG_BUFFER_SIZE/2);
    printf(" starting Test with size %d\n",randsize);
    for(cnt=0;cnt<BIG_BUFFER_SIZE;cnt++){
      bigw[cnt] = rand();
    }
    trx=0;
    rx =0;
    
    do{
      DPRINT("TRY TO WRITE %d bytes [tot %d] \n",randsize-trx,trx);
      retw = comm->write(&bigw[trx],randsize-trx,100);

      if(retw>0){
	trx+=retw;
	DPRINT("DONE WRITE %d/%d \n",retw,randsize-trx);
      }
      DPRINT("TRY TO READ %d [tot %d]\n",randsize-rx,rx);
      retr = comm->read_async(&bigr[rx],randsize-rx);
      if(retr>0){
	rx+=retr;
	DPRINT("DONE READ %d/%d \n",retr,randsize-rx);
      }
      
    } while((trx<randsize)||(rx<randsize));
    
    printf("%d] checking data\n",cycles);
    for(cnt=0;cnt<randsize;cnt++){
      if(bigw[cnt]!=bigr[cnt]){
	printf("# cycle %d data error at [%d] read %d expected %d\n",cycles,cnt,bigw[cnt],bigr[cnt]);
	return -1;
      }
    }
    printf("%d] OK\n",cycles);
  }
    
  
  return 0;
}
int main(int argc, char *argv[])
{

  int ret;
  int cycles=100;
  boost::smatch match;
  boost::program_options::options_description desc("options");
  unsigned*wptr,*rptr;
  int bufsize=8192;
  desc.add_options()("help", "help");
  desc.add_options()("cycles", boost::program_options::value<int>(),"test cycles and memory allocated");
  // put your additional options here
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
  std::string param = vm["dev"].as<std::string>();

  if(vm.count("cycles")){
    cycles = vm["cycles"].as<int>();
  }
  if(vm.count("buf")){
    bufsize = vm["buf"].as<int>();
  }

  if(!regex_match(param,match,parse_arg)){
    std::cout<<"## bad parameter specification:"<<param<<" match:"<<match[0]<<std::endl;
    return -2;
  }
  std::string dev = match[1];
  std::string baudrate = match[2];
  std::string parity = match[3];
  std::string bits = match[4];
  std::string stop = match[5];
  PosixSerialComm* comm=new PosixSerialComm(dev,atoi(baudrate.c_str()),atoi(parity.c_str()),atoi(bits.c_str()),atoi(stop.c_str()));
  if(comm){
    if((ret= comm->init())!=0){
      std::cout<<"## error during initialization:"<<ret<<std::endl;
      return -1;
    }
    wptr = (unsigned*)malloc(cycles*4);
    rptr = (unsigned*)malloc(cycles*4);
    
      if(test1(cycles,wptr,rptr,comm) == 0){
      printf("Test 1 success\n");
    } else {
      return -1;
    }
      
    if(test2(cycles,wptr,rptr,10,comm) == 0){
      printf("Test 2 success\n");
    } else {
      return -1;
    }

    if(test3(cycles,comm) == 0){
      printf("Test 3 success\n");
    } else {
      return -3;
    }

  } else {
    printf("## cannot allocate object\n");
    return -4;
  }
  // test your libserial library here

  return 0;
}
