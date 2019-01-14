#include "server.h"
#include "config.h"
#include <glog/logging.h>


int main(){
	google::InitGoogleLogging("http_server");
	google::SetLogDestination(google::INFO, "/home/dmsMusic/http_server");
	google::FlushLogFiles(google::INFO);
	class configParser* configptr = new configParser(std::string ("server.conf"));
	configptr->loadConfig();
	class httpServer * server = new class httpServer("0.0.0.0",12345);
	server->init(configptr->config);
	LOG(INFO) << server->check_request_valid("1544113635","b6521a86bc4cc0830370f174e1a30b11") << std::endl;
	server->start();
}
