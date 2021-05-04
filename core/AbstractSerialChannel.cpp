#include "AbstractSerialChannel.h"
#include <string.h>
namespace common {
  namespace serial {
     int AbstractSerialChannel::read(void *buffer,int maxnb,const char*isanyof,int ms_timeo,int*timeout_arised){
            int t_arised=0;
            int cnt=0;
            char*bufp=(char*)buffer;
            if(timeout_arised){
                *timeout_arised=0;
            }
            while(cnt<maxnb) {
                char buf;
                if(read(&buf,1,ms_timeo,&t_arised)>0){
                    bufp[cnt++]=buf;
                }
                if(t_arised ){
                    if(timeout_arised){
                        *timeout_arised=t_arised;
                        return cnt;
                    }
                }
                if(isanyof ){
                    for(int i=0;i<strlen(isanyof);i++){
                        if(isanyof[i]==buf){
                            return cnt;
                        }
                    }
                    
                }
            }
            return cnt;
        }
    
  }
};

