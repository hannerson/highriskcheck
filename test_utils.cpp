#include "utils.h"
#include <iostream>
#include <algorithm>

int main(){
	std::string src = "I'm a good people.";
	//src.erase(std::remove_if(src.begin(),src.end(),isspace),src.end());
	std::string dest = remove_space_punct(src);
	std::cout << src << std::endl;
	std::cout << dest << std::endl;
}
