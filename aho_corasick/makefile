###########
###20190113
###########

CXX=/usr/local/gcc-4.8/bin/g++
CXXFLAGS=-g -Wall -c -std=c++11
LDFLAGS=-L/usr/local/lib -L/usr/lib64/mysql/ -L/usr/local/lib64/
LIBS=-lpthread -lglog -lcrypto -levent -ljsoncpp -lglog -lmysqlclient

AR = ar
ARFLAGS = -crs

srcs=$(wildcard *.cpp)
objs=$(patsubst %.cpp,%.o,$(srcs))

target=test

$(target):$(objs) $(objh)
	$(CXX) $^ -o $@
$(objs):%.o:%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(target) *.o *.a
