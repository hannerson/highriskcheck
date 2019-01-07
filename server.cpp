#include "server.h"
#include <iostream>
#include <thread>
#include <mutex>
#include "config.h"
#include <openssl/md5.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <unistd.h>
#include <stdlib.h>
#include <exception>
#include <string.h>
#include <string>
#include <set>
#include <event.h>
#include <json/json.h>

std::unordered_map<std::string, server_func> map_server_func;

const struct{
	std::string uri;
	void (httpServer::*func)(struct evhttp_request *);
} request_process_config[] = {
	{"/test",&httpServer::test_process},
	{"/echo",&httpServer::echo_process},
	{"/highriskcheck",&httpServer::highrisk_process},
	{"/other",&httpServer::other_process},
};


int httpServer::init(std::unordered_map<std::string,std::string> config){
	try{
		if(config.count("threadnum") > 0){
			threadnum = atoi(config["threadnum"].c_str());
		}else{
			return -1;
		}

		if(config.count("queue_max_size") > 0){
			queue_max_size = atoi(config["queue_max_size"].c_str());
		}else{
			return -1;
		}

		if(config.count("secret_key") > 0){
			secret_key = config["secret_key"];
		}else{
			return -1;
		}

		if(config.count("db_ip") > 0){
			db_ip = config["db_ip"];
		}else{
			return -1;
		}

		if(config.count("db_user") > 0){
			db_user = config["db_user"];
		}else{
			return -1;
		}

		if(config.count("db_passwd") > 0){
			db_passwd = config["db_passwd"];
		}else{
			return -1;
		}

		if(config.count("db_name") > 0){
			db_name = config["db_name"];
		}else{
			return -1;
		}

		if(config.count("db_charset") > 0){
			db_charset = config["db_charset"];
		}else{
			return -1;
		}

		if(config.count("db_port") > 0){
			db_port = atoi(config["db_port"].c_str());
		}else{
			return -1;
		}

		if(config.count("db_pool_num") > 0){
			db_pool_num = atoi(config["db_pool_num"].c_str());
		}else{
			return -1;
		}

		if(config.count("db_pool_maxnum") > 0){
			db_pool_maxnum = atoi(config["db_pool_maxnum"].c_str());
		}else{
			return -1;
		}

		if(config.count("redis_ip") > 0){
			redis_ip = config["redis_ip"];
		}else{
			return -1;
		}

		if(config.count("redis_port") > 0){
			redis_port = atoi(config["redis_port"].c_str());
		}else{
			return -1;
		}

		//init mysql pool
		conn_pool = new mysqlPool(db_pool_num,db_pool_maxnum,db_ip,db_port,db_user,db_passwd,db_name,db_charset);
		//init process function
		map_server_func.emplace("/test",&httpServer::test_process);
		map_server_func.emplace("/echo",&httpServer::echo_process);
		map_server_func.emplace("/highriskcheck",&httpServer::highrisk_process);
		map_server_func.emplace("/other",&httpServer::other_process);

		if(index_builder()<0){
			return -1;
		}
	}
	catch(char * e){
	}
	return 0;
}

int httpServer::stop(){
	std::lock_guard<std::mutex> lock(isstop_mutex);
	isstop = false;
	return 0;
}

bool httpServer::check_request_valid(std::string timestamps, std::string md5sum){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned char md[16];
	char buf[33] = {'\0'};
	char tmp[3] = {'\0'};
	std::string secret(secret_key);
	std::string src = secret + "_" + timestamps;
	std::string md5str;
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, src.c_str(), src.length());
	MD5_Final(md,&ctx);
	for(int i=0; i<16; i++){
		sprintf(tmp, "%02x", md[i]);
		strcat(buf,tmp);
	}
	//std::cout << buf << std::endl;
	if(md5sum.compare(buf) == 0){
		return true;
	}else{
		return false;
	}
}

int httpServer::start(){
	struct event_base *base = event_base_new();
	if(base == nullptr){
		LOG(ERROR) << "create event base failed" << std::endl;
		return -1;
	}
	struct evhttp *http_server = evhttp_new(base);
	if(!http_server){
		LOG(ERROR) << "create event http failed" << std::endl;
		return -1;
	}
	int ret = evhttp_bind_socket(http_server,http_addr.c_str(),port);
	if(ret != 0){
		LOG(ERROR) << "bind socket failed" << std::endl;
		return -1;
	}

	evhttp_set_gencb(http_server, http_generic_handler, this);

	//start threadworker
	//
	for(auto i=0; i<threadnum; i++){
		threadpool.emplace_back(std::thread(&httpServer::thread_worker,this));
		LOG(INFO) << "create thread "<< i << " success" << std::endl;
	}
	//set timer event and timeout
	struct event *ev_timeout = event_new(NULL,-1, 0, NULL, NULL);
	event_assign(ev_timeout, base, -1, EV_TIMEOUT | EV_PERSIST, event_timeout, NULL);
	event_add(ev_timeout, NULL);

	evhttp_set_timeout(http_server,5);
	event_base_dispatch(base);
	evhttp_free(http_server);
}

void httpServer::http_generic_handler(struct evhttp_request *request, void *args){
	class httpServer *me = (class httpServer*) args;
	//std::cout << (void*) request << std::endl;
	//const char* uri = evhttp_request_get_uri(request);
	//std::cout << "uri is " << uri << std::endl;
	me->add_task(request);
	LOG(INFO) << "add task" << std::endl;
	//std::cout << "add task" << std::endl;
	//sleep(3);
}

void httpServer::add_task(struct evhttp_request *request){
	std::unique_lock<std::mutex> lock(queue_mutex);
	m_not_full.wait(lock, [this]{return isstop || not_full();});
	if(isstop){
		return;
	}
	task_queue.emplace(request);
	m_not_empty.notify_all();
}

void httpServer::get_task(struct evhttp_request **request){
	std::unique_lock<std::mutex> lock(queue_mutex);
	m_not_empty.wait(lock, [this]{return isstop || not_empty();});
	if(isstop){
		return;
	}
	*request = task_queue.front();
	task_queue.pop();
	m_not_full.notify_all();
}

bool httpServer::not_full(){
	bool full = task_queue.size() >= queue_max_size;
	if(full){
		LOG(INFO) << "task queue is full:" << queue_max_size << std::endl;
	}
	return !full;
}

bool httpServer::not_empty(){
	bool empty = task_queue.empty();
	if(empty){
		LOG(INFO) << "task queue is empty" << std::endl;
	}
	return !empty;
}

int httpServer::thread_worker(){
	while(!isstop){
		struct evhttp_request *request;
		get_task(&request);
		//std::cout << (void*) request << std::endl;
		LOG(INFO) << "get task ok" << std::endl;
		//std::cout << "get task ok " << std::this_thread::get_id() << std::endl;
		//process request
		const char* uri = evhttp_request_get_uri(request);
		if(uri == nullptr){
			LOG(INFO) << "uri is null" << std::endl;
			//std::cout << "uri is null" << std::endl;
			struct evbuffer *retbuff = nullptr;
			retbuff = evbuffer_new();
			if(retbuff == nullptr){
				LOG(ERROR) << "evbuffer create failed" << std::endl;
				continue;
			}
			Json::Value result;
			Json::FastWriter writer;
			result["status"] = "success";
			result["msg"] = "uri is null";
			std::string return_str = writer.write(result);
			int ret = evbuffer_add_printf(retbuff, return_str.c_str());
			if(ret == -1){
				LOG(ERROR) << "evbuffer printf failed" << std::endl;
				continue;
			}
			evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
			evbuffer_free(retbuff);
		}
		std::cout << "uri is " << uri << std::endl;
		//std::string s_uri = evhttp_uri_parse(uri);
		//std::size_t pos = s_uri.find("?");
		struct evhttp_uri *decoded = nullptr;
		decoded = evhttp_uri_parse(uri);
		if(decoded == nullptr){
			std::cout << "error" << std::endl;
			LOG(INFO) << "evhttp_uri_parse error" << std::endl;
			evhttp_send_error(request, HTTP_BADREQUEST, 0);
			continue;
		}
		std::string uri_path = evhttp_uri_get_path(decoded);
		free(decoded);
		/*if(pos != std::string::npos){
			uri_path = s_uri.substr(0,pos);
		}else{
			uri_path = uri;
		}*/

		
		if(uri_path == ""){
			LOG(INFO) << "uri is empty" << std::endl;
			std::cout << "uri is empty" << std::endl;
			struct evbuffer *retbuff = nullptr;
			retbuff = evbuffer_new();
			if(retbuff == nullptr){
				LOG(ERROR) << "evbuffer create failed" << std::endl;
				continue;
			}
			Json::Value result;
			Json::FastWriter writer;
			result["status"] = "success";
			result["msg"] = "uri is empty";
			std::string return_str = writer.write(result);
			int ret = evbuffer_add_printf(retbuff, return_str.c_str());
			if(ret == -1){
				LOG(ERROR) << "evbuffer printf failed" << std::endl;
				continue;
			}
			evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
			evbuffer_free(retbuff);
		}else{
			//process request
			if(map_server_func.find(uri_path) != map_server_func.end()){
				(this->*map_server_func[uri_path])(request);
			}else{
				(this->*map_server_func["/other"])(request);
			}
		}
	}
}

void httpServer::test_process(struct evhttp_request *request){
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	result["status"] = "success";
	result["msg"] = "just test page!";
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::echo_process(struct evhttp_request *request){
	const char* uri = evhttp_request_get_uri(request);
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	Json::Value data(Json::arrayValue);
	result["status"] = "success";
	result["msg"] = "uri is " + std::string(uri);
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::other_process(struct evhttp_request *request){
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	Json::Value data(Json::arrayValue);
	result["status"] = "success";
	result["msg"] = "wrong access,kidding me?";
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

//uri:/highriskcheck?timestamp=12345&pkey=dfjkdfkdlfdk&music=aa&artist=bb&album=cc
void httpServer::highrisk_process(struct evhttp_request *request){
	const char* uri = evhttp_request_get_uri(request);
	std::cout << uri << "  -------  " << std::endl;
	struct evhttp_uri *decoded = nullptr;
	decoded = evhttp_uri_parse(uri);

	if(decoded == nullptr){
		std::cout << "error" << std::endl;
		LOG(INFO) << "evhttp_uri_parse error" << std::endl;
		evhttp_send_error(request, HTTP_BADREQUEST, 0);
		return;
	}

	Json::Value result;
	Json::FastWriter writer;
	Json::Value data(Json::arrayValue);

	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}

	if(evhttp_request_get_command(request) != EVHTTP_REQ_GET){
		result["status"] = "fail";
		result["msg"] = "just support get method";
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}
	char *query = (char*)evhttp_uri_get_query(decoded);
	if(query == nullptr){
	}
	//evhttp_parse_query_str(query, params);
	char* decode_uri = evhttp_decode_uri(uri);
	std::cout << decode_uri << "  -------  " << std::endl;
	struct evkeyvalq params;
	//evhttp_parse_query(decode_uri,&params);
	evhttp_parse_query_str(query, &params);
	std::cout << "----" << std::endl;
	const char* temp = nullptr;
	std::string timestamp,pkey,music,artist,album;

	temp = evhttp_find_header(&params,"timestamp");
	if(temp){
		timestamp = temp;
		std::cout << timestamp << std::endl;
	}
	temp = evhttp_find_header(&params,"pkey");
	if(temp){
		pkey = temp;
		std::cout << pkey << std::endl;
	}

	temp = evhttp_find_header(&params,"music");
	if(temp){
		music = temp;
		std::cout << music << std::endl;
	}

	temp = evhttp_find_header(&params,"artist");
	if(temp){
		artist = temp;
		std::cout << artist << std::endl;
	}

	temp = evhttp_find_header(&params,"album");
	if(temp){
		album = temp;
		std::cout << album << std::endl;
	}

	free(decode_uri);

	if(timestamp == "" || pkey == ""){
		result["status"] = "fail";
		result["msg"] = "parameter is missing";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}
	//check valid
	if(!check_request_valid(timestamp,pkey)){
		result["status"] = "fail";
		result["msg"] = "valid check failed";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}

	if(music == "" && album == "" && artist == ""){
		result["status"] = "fail";
		result["msg"] = "music-album-artist must not all empty";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}
	//check in unordermap
	Json::Reader reader;
	Json::Value tempjson;
	std::string toparse;
	std::string allkey = remove_space_punct(music) + remove_space_punct(artist) + remove_space_punct(album);
	std::string artist_key = remove_space_punct(artist);
	std::string album_key = remove_space_punct(artist) + remove_space_punct(album);
	std::string album_key2 = remove_space_punct(album);
	std::string music_key = remove_space_punct(music) + remove_space_punct(artist);
	std::string music_key2 = remove_space_punct(music);
	std::set<std::string> album_vec;
	album_vec.emplace(album_key);
	album_vec.emplace(album_key2);
	std::set<std::string> music_vec;
	music_vec.emplace(music_key);
	music_vec.emplace(allkey);
	music_vec.emplace(music_key2);
	for(auto key:music_vec){
		//check in keywords tree:music
		{
			std::lock_guard<std::mutex> lock(index_music_mutex);
			auto got = index_tree_music.find(key);
			if(got != index_tree_music.end()){
				toparse = index_tree_music[key];
			}
		}
		if(toparse != ""){
			if(!reader.parse(toparse.c_str(),tempjson)){
				result["status"] = "fail";
				result["msg"] = "json parse music failed";
				result["data"] = data;
				std::string return_str = writer.write(result);
				int ret = evbuffer_add_printf(retbuff, return_str.c_str());
				if(ret == -1){
					LOG(ERROR) << "evbuffer printf failed" << std::endl;
					return;
				}
				evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
				evbuffer_free(retbuff);
				return;
			}else{
				data.append(tempjson);
			}
		}
	}
	//check in keywords tree:album
	for(auto key:album_vec){
		toparse = "";
		{
			std::lock_guard<std::mutex> lock(index_album_mutex);
			auto got = index_tree_album.find(key);
			if(got != index_tree_album.end()){
				toparse = index_tree_album[key];
			}
		}
		if(toparse != ""){
			if(!reader.parse(toparse.c_str(),tempjson)){
				result["status"] = "fail";
				result["msg"] = "json parse album failed";
				result["data"] = data;
				std::string return_str = writer.write(result);
				int ret = evbuffer_add_printf(retbuff, return_str.c_str());
				if(ret == -1){
					LOG(ERROR) << "evbuffer printf failed" << std::endl;
					return;
				}
				evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
				evbuffer_free(retbuff);
				return;
			}else{
				data.append(tempjson);
			}
		}
	}
	//check in keywords tree:artist
	toparse = "";
	{
		std::lock_guard<std::mutex> lock(index_artist_mutex);
		auto got = index_tree_artist.find(artist_key);
		if(got != index_tree_artist.end()){
			toparse = index_tree_artist[artist_key];
		}
	}
	if(toparse != ""){
		if(!reader.parse(toparse.c_str(),tempjson)){
			result["status"] = "fail";
			result["msg"] = "json parse artist failed";
			result["data"] = data;
			std::string return_str = writer.write(result);
			int ret = evbuffer_add_printf(retbuff, return_str.c_str());
			if(ret == -1){
				LOG(ERROR) << "evbuffer printf failed" << std::endl;
				return;
			}
			evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
			evbuffer_free(retbuff);
			return;
		}else{
			data.append(tempjson);
		}
	}

	//key words check
	trie_mutex.lock();
	auto trie_result = index_trie.parse_text(allkey);
	trie_mutex.unlock();
	for(auto ret:trie_result){
		toparse = "";
		std::string tmp = ret.get_keyword();
		trie_mutex.lock();
		auto got = index_tree_keyword.find(allkey);
		if(got != index_tree_keyword.end()){
			toparse = index_tree_keyword[allkey];
		}
		trie_mutex.unlock();
		if(toparse != ""){
			if(!reader.parse(toparse.c_str(),tempjson)){
				result["status"] = "fail";
				result["msg"] = "json parse keyword failed";
				result["data"] = data;
				std::string return_str = writer.write(result);
				int ret = evbuffer_add_printf(retbuff, return_str.c_str());
				if(ret == -1){
					LOG(ERROR) << "evbuffer printf failed" << std::endl;
					return;
				}
				evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
				evbuffer_free(retbuff);
				return;
			}else{
				data.append(tempjson);
			}
		}
	}
	if(data.size() > 0){
		result["ret"] = "hit";
	}else{
		result["ret"] = "miss";
	}
	result["status"] = "success";
	result["msg"] = "OK";
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::event_timeout(int fd, short events, void *args){
}

int httpServer::index_builder(){
	std::shared_ptr<mysqlconnector> conn = conn_pool->get_connection();
	if(conn == nullptr){
		//error
		return -1;
	}
	//music index
	std::string sql = "select name,artist,album,type,cate,source from hf_huangfan_music";
	int cnt = conn->execute(sql);
	if(cnt > 0){
		mysqlresult *res = conn->fetchall();
		mysqlrow *row;
		while((row = res->next())){
			std::string name = row->get_value("name");
			std::string artist = row->get_value("artist");
			std::string album = row->get_value("album");
			std::string type = row->get_value("type");
			std::string cate = row->get_value("cate");
			std::string source = row->get_value("source");
			//json process
			Json::Value input;
			Json::Value result;
			Json::FastWriter writer;
			input["music"] = name;
			input["artist"] = artist;
			input["album"] = album;
			input["type"] = type;
			input["cate"] = cate;
			input["source"] = source;

			std::string key = remove_space_punct(name) + remove_space_punct(artist) + remove_space_punct(album);

			std::string input_string = writer.write(input);
			//lock
			std::lock_guard<std::mutex> lock(index_music_mutex);
			//insert map
			index_tree_music.emplace(key, input_string);
		}
		res->free();
	}
	//album index
	sql = "select artist,album,type,cate,source from hf_huangfan_album";
	cnt = conn->execute(sql);
	if(cnt > 0){
		mysqlresult *res = conn->fetchall();
		mysqlrow *row;
		while((row = res->next())){
			std::string artist = row->get_value("artist");
			std::string album = row->get_value("album");
			std::string type = row->get_value("type");
			std::string cate = row->get_value("cate");
			std::string source = row->get_value("source");
			//json process
			Json::Value input;
			Json::Value result;
			Json::FastWriter writer;
			input["artist"] = artist;
			input["album"] = album;
			input["type"] = type;
			input["cate"] = cate;
			input["source"] = source;

			std::string key = remove_space_punct(artist) + remove_space_punct(album);

			std::string input_string = writer.write(input);
			//lock
			std::lock_guard<std::mutex> lock(index_album_mutex);
			//insert map
			index_tree_album.emplace(key, input_string);
		}
		res->free();
	}
	//artist index
	sql = "select name,type,cate,source from hf_huangfan_artist";
	cnt = conn->execute(sql);
	if(cnt > 0){
		mysqlresult *res = conn->fetchall();
		mysqlrow *row;
		while((row = res->next())){
			std::string artist = row->get_value("name");
			std::string type = row->get_value("type");
			std::string cate = row->get_value("cate");
			std::string source = row->get_value("source");
			//json process
			Json::Value input;
			Json::Value result;
			Json::FastWriter writer;
			input["artist"] = artist;
			input["type"] = type;
			input["cate"] = cate;
			input["source"] = source;

			std::string key = remove_space_punct(artist);

			std::string input_string = writer.write(input);
			//lock
			std::lock_guard<std::mutex> lock(index_artist_mutex);
			//insert map
			index_tree_artist.emplace(key, input_string);
		}
		res->free();
	}

	//keywords index
	sql = "select name,type,cate,source from hf_huangfan_keyword";
	cnt = conn->execute(sql);
	if(cnt > 0){
		mysqlresult *res = conn->fetchall();
		mysqlrow *row;
		while((row = res->next())){
			std::string name = row->get_value("name");
			std::string type = row->get_value("type");
			std::string cate = row->get_value("cate");
			std::string source = row->get_value("source");
			//json process
			Json::Value input;
			Json::Value result;
			Json::FastWriter writer;
			input["keyword"] = name;
			input["type"] = type;
			input["cate"] = cate;
			input["source"] = source;

			std::string key = remove_space_punct(name);

			std::string input_string = writer.write(input);
			//lock
			std::lock_guard<std::mutex> lock(trie_mutex);
			//insert map
			index_trie.insert(key);
			index_tree_keyword.emplace(key, input_string);
		}
		res->free();
	}
	return 0;
}
