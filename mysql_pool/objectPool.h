#ifndef _OBJECTPOOL_H_
#define _OBJECTPOOL_H_
#include <iostream>
#include <list>
#include <set>
#include <queue>
#include <memory>
#include <assert.h>

template <typename T>
class objectPool{
	public:
		objectPool(){}
		~objectPool(){
			std::lock_guard<std::mutex> lock(free_mutex);
			for(auto i=free_list.begin();i!=free_list.end();i++){
				free_object(*i);
			}
			for(auto i=used_list.begin();i!=used_list.end();i++){
				free_object(*i);
			}
		}
		template <typename ... Args >
		int init(size_t num, int max_num, Args && ... args){
			initial_num = num;
			assert(initial_num >= 4)
			max_num = max_num;
			used_list.clear();
			free_list.clear();
			for(int i=0; i<initial_num; i++){
				free_list.emplace_back(new T(std::forward<Args>(args)...));
			}
		}
		T *t get(){
			std::lock_guard<std::mutex> lock(free_mutex);
			if(free_list.size() == 0 && used_list.size() < max_sum){
				extent_list(initial_num/2);
			}else if(free_list.size() == 0 && used_list.size() >= max_sum){
				return nullptr;
			}
			T *t = free_list.front();
			free_list.erase(free_list.begin());

			used_list.emplace_back(t);

			return t;
		}

		void put_back(T *t){
			std::lock_guard<std::mutex> lock(free_mutex);
			for(auto i=used_list.begin(); i!=used_list.end(); i++){
				if(t == *i){
					used_list.erase(i);
					free_list.emplace_back(t);
					return;
				}
			}
		}
	private:
		template <typename ... Args >
		int extent_list(size_t num, Args && ... args){
			for(int i=0; i<num; i++){
				free_list.emplace_back(new T(std::forward<Args>(args)...));
			}
		}

		void free_object(T *t){
			delete t;
		}

	private:
		//used list, free list
		std::list<T *t> used_list;
		std::list<T *t> free_list;
		//mutex multi threads
		std::mutex free_mutex;
		int initial_num;
		int max_num;
};
#endif

#define _OBJECTPOOL_TEST_
#ifdef _OBJECTPOOL_TEST_

int main(){
}

#endif
