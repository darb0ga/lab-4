obj-m := vtfs.o
vtfs-objs := vtfs_main.o vtfs_super.o vtfs_inode.o vtfs_dir.o vtfs_store.o vtfs_file.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf .cache
