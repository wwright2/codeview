# To build the module type: 
#	make
# To load the module type: 
#	make load
# To unload the module type: 
#	make unload
# To clean the directory type:
#	make clean
# 

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

MODULE := probe_do_fork
obj-m := $(MODULE).o
$(MODULE)-objs += kprobe_do_fork.o 

MODULE.o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

load:
	# load module passing 2 parameters. bYield=true and iDebugLevel=INFO_LEVEL
	sudo insmod ./$(MODULE).ko iMaxLineSize=150 iMaxNbOfLines=50 iDebugLevel=4

unload:
	sudo rmmod $(MODULE)

clean:
	-@rm -fr *.o $(MODULE)*.o $(MODULE)*.ko *.mod.* *.order *.symvers *.markers *.*~ *~ .*.cmd .tmp_versions
