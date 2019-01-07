#ifndef _MYSQLPOOL_H_
#define _MYSQLPOOL_H_
#include <stdio.h>
#include <iostream>
#include "objectPool2.h"
#include <string>
#include "mysqlconn.h"

using namespace mysqlconn;

class mysqlPool{
	public:
		//mysqlPool(size_t num, size_t max_num, std::string host, unsigned int port, std::string user, std::string passwd, std::string db, std::string charset);
		mysqlPool(size_t num, size_t max_num, std::string host, unsigned int port, std::string user, std::string passwd, std::string db, std::string charset):num(num),max_num(max_num),host(host),port(port),user(user),passwd(passwd),db(db),charset(charset) {
			sqlpool = new objectPool<mysqlconnector>;
			sqlpool->init(num, max_num, host, port, user, passwd, db, charset);
		}
		~mysqlPool() {
			//std::cout << "mysqlPool desctruct before" << std::endl;
			if(sqlpool){
				delete sqlpool;
			}
			LOG(INFO) << "mysqlPool desctruct" << std::endl;
		}
		std::shared_ptr<mysqlconnector> get_connection(){
			return sqlpool->get();
		}
	private:
		objectPool<mysqlconnector> *sqlpool;
		std::string host;
		std::string user;
		std::string passwd;
		unsigned int port;
		std::string charset;
		std::string db;
		size_t num;
		size_t max_num;
};
#endif
