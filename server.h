#ifndef _SEVER_H_
#define _SEVER_H_
#include <event2/util.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <vector>
#include <condition_variable>
#include "config.h"
#include "aho_corasick/aho_corasick.hpp"
#include <glog/logging.h>
#include "mysql/mysqlPool.h"
#include "utils.h"
#include <json/json.h>

class httpServer{
	public:
		httpServer(std::string http_addr, int port):port(port),http_addr(http_addr){}
		~httpServer(){
			delete conn_pool;
		}
		int init(std::unordered_map<std::string,std::string> config);
		bool check_request_valid(std::string timestamps, std::string md5sum);
		void test_process(struct evhttp_request *request);
		void highrisk_process(struct evhttp_request *request);
		void other_process(struct evhttp_request *request);
		void echo_process(struct evhttp_request *request);
		static void http_generic_handler(struct evhttp_request *request, void *args);
		static void event_timeout(int fd, short events, void *args);
		int thread_worker();
		int start();
		int stop();
		int index_builder();
	private:
		bool not_full();
		bool not_empty();
		void get_task(struct evhttp_request **request);
		void add_task(struct evhttp_request *request);
	private:
		int threadnum;
		std::string secret_key;
		std::string http_addr;
		int port;
		bool isstop;
		std::mutex isstop_mutex;
		std::queue<struct evhttp_request*> task_queue;
		std::mutex queue_mutex;
		int queue_max_size;
		std::vector<std::thread> threadpool;
		std::condition_variable_any m_not_empty;
		std::condition_variable_any m_not_full;
		std::mutex index_music_mutex;
		std::mutex index_album_mutex;
		std::mutex index_artist_mutex;
		std::unordered_map< std::string,std::string > index_tree_music;
		std::unordered_map< std::string,std::string > index_tree_album;
		std::unordered_map< std::string,std::string > index_tree_artist;
		std::mutex trie_mutex;
		aho_corasick::trie index_trie;
		std::unordered_map< std::string,std::string > index_tree_keyword;
		mysqlPool *conn_pool;
		std::string db_ip;
		std::string db_user;
		std::string db_name;
		std::string db_passwd;
		std::string db_charset;
		unsigned int db_port;
		unsigned int db_pool_num;
		unsigned int db_pool_maxnum;
		std::string redis_ip;
		unsigned int redis_port;
};

typedef void (httpServer::*server_func)(struct evhttp_request *);

#endif
