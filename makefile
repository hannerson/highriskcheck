##########
# 20190113
##########

exclude_dirs=include bin jsoncpp glog
dirs := $(shell find . -maxdepth 1 -type d)
dirs := $(basename $(patsubst ./%,%,$(dirs)))
dirs := $(filter-out $(exclude_dirs),$(dirs))

AR = ar
ARFLAGS = -crs

cur_dir=$(shell pwd)
all_files=$(wildcard *)
subdirs:=$(dirs)

all:
	@for n in $(subdirs);do $(MAKE) -C $$n; done


.PHONY clean:
clean:
	-rm $(obj)
