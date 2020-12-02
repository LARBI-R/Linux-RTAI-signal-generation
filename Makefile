# Remplacer foo par le fichier objet (.o) compilé
obj-m	:= Squelet.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)
EXTRA_CFLAGS := -I/usr/realtime/include -I/usr/include/ -I/usr/local/include  -D__IN_RTAI__ -ffast-math -mhard-float


default:
	$(MAKE) -lcomedi -lm -C $(KDIR) SUBDIRS=$(PWD) modules
