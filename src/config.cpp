#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include "config.h"
#include <glog/logging.h>

int configParser::loadConfig(){
	std::ifstream infile(fileconfig.c_str());
	if(!infile){
		LOG(ERROR) << "file open:" << fileconfig << " failed." << std::endl;
		return -1;
	}
	std::string instr;
	while(getline(infile, instr)){
		int index = 0;
		//del ' '
		if(!instr.empty()){
			while((index=instr.find(' ',index)) != std::string::npos){
				instr.erase(index,1);
			}
		}

		if((index=instr.find('#')) != std::string::npos){
			instr = instr.substr(0,index);
		}
		if(instr == "" || instr.find("#") == 0){
			continue;
		}
		index = instr.find('=');
		if(index == std::string::npos){
			continue;
		}
		config.emplace(instr.substr(0,index),instr.substr(index+1));
	}
	infile.close();
}

//#define _CONFIG_TEST_
#ifdef _CONFIG_TEST_
int main(){
	google::InitGoogleLogging("http_server");
	google::SetLogDestination(google::INFO, "/home/dmsMusic/http_server");
	class configParser* configptr = new configParser(std::string ("test.conf"));
	configptr->loadConfig();
	std::cout << configptr->config["threadnum"] << std::endl;
	LOG(INFO) << "file config:" << configptr->fileconfig << std::endl;
	std::cout << configptr->config["database"] << std::endl;
	LOG(INFO) << configptr->config["database"] << std::endl;
}
#endif
