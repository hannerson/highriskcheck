#####
#
#####

src=$(wildcard *.cpp)
dir=$(not dir $(src))
obj=$(patsubst %.cpp,%.o,$(dir))

all:
	@echo $(src)
	@echo $(dir)
	@echo $(obj)
	@echo "end"
