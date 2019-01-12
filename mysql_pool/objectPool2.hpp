#ifndef _OBJECTPOOL_H_
#define _OBJECTPOOL_H_
#include <iostream>
#include <list>
#include <set>
#include <queue>
#include <memory>
#include <assert.h>
#include <thread>
#include <mutex>
#include <glog/logging.h>

template <typename T>
class objectPool{
	public:
		objectPool():free_object(false){}
		~objectPool(){free_object = true;}
		template <typename ... Args>
		int init(size_t num, size_t max_num, Args && ... args){
			init_num = num;
			cur_num = num;
			assert(init_num >= 4);
			max_num = max_num;
			std::cout << cur_num << ":" << max_num << std::endl;
			for(int i=0; i<init_num; i++){
				user_object_list.emplace_back(std::shared_ptr<T>(new T(std::forward<Args>(args)...), [this](T *t){
					auto_create_ptr(t);
				}));
			}
		}

		void auto_create_ptr(T *t){
			/*if(t){
				delete t;
				t = nullptr;
			}*/
			if(free_object){
				if(t){
					//std::cout << "delete before" << std::endl;
					delete t;
					t = nullptr;
					//LOG(INFO) << "delete t" << std::endl;
				}
			}else{
				std::lock_guard<std::mutex> lock(list_mutex);
				LOG(INFO) << "insert list" << std::endl;
				LOG(INFO) << t << std::endl;
				user_object_list.emplace_back(std::shared_ptr<T>(t));
			}
		}

		std::shared_ptr<T> get(){
			std::lock_guard<std::mutex> lock(list_mutex);
			/*if(cur_num >= max_num){
				std::cout << cur_num << ":" << max_num << std::endl;
				return nullptr;
			}*/
			if(!user_object_list.empty()){
				LOG(INFO) << "object ok" << std::endl;
				auto ptr = user_object_list.front();
				user_object_list.remove(ptr);
				return ptr;
			}
		}

	private:
		template <typename ... Args>
		void expand_list(size_t num, Args && ... args){
			for(int i=0; i<num; i++){
				user_object_list.emplace_back(std::shared_ptr<T>(new T(std::forward<Args>(args)...), [this](T *t){
					auto_create_ptr(t);
				}));
			}
		}
	private:
		objectPool(const objectPool&);
		objectPool& operator = (const objectPool&);
		std::list<std::shared_ptr<T>> user_object_list;
		std::mutex list_mutex;
		bool free_object;
		int init_num;
		int cur_num;
		int max_num;
};

#endif
