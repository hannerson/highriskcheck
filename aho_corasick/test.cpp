#include "aho_corasick.hpp"
#include <iostream>


int main(){
	aho_corasick::trie trie;
	trie.insert("hers");
	trie.insert("his");
	trie.insert("she");
	trie.insert("he");
	trie.insert("中国");
	trie.insert("日本");
	auto result = trie.parse_text("中国万岁，日本无理取闹");
	for(auto ret:result){
		std::cout << ret.get_keyword() << std::endl;
	}
}
