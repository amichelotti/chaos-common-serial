#include <common/misc/driver/SerialChannelFactory.h>

int main(int argc,char** argv){
	if(argc<2){
		std::cout<<"## you must specify a valid JSON"<<std::endl;
		return -1;
	}

	try {
		common::misc::driver::AbstractSerialChannel_psh channel=common::misc::driver::SerialChannelFactory::getChannelFromJson(std::string(argv[1]));
		channel->init();

		while(1){
			char buffer[1024];
			std::cout<<"send:";
			std::cin>>buffer;

			int ret=channel->write(buffer,strlen(buffer)+1,10000);
			if(ret>0){
				int received=5;
				std::cout<<"waiting data...\n";
			//	while((received=channel->byte_available_read())==0);
				//std::cout<<"received "<<received<<" ...\n";
				ret=channel->read(buffer,sizeof(buffer),10000);
				if(ret){
					std::string answ=buffer;
					std::cout<<"Received "<<ret<<" bytes :\""<<buffer<<"\"\n";
				} else {
					std::cout<<"## an error occurred reading:"<<ret<<"\n";
				}

			} else {
				std::cout<<"## an error occurred writing:"<<ret<<"\n";
			}

		}
	} catch (std::logic_error e){
		std::cout<<"## error opening channel:"<<e.what();

		return -1;
	}


	return 0;
}
