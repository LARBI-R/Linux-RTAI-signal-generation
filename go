sudo /sbin/insmod /usr/realtime/modules/rtai_hal.ko
sudo /sbin/insmod /usr/realtime/modules/rtai_lxrt.ko
sudo /sbin/insmod /usr/realtime/modules/rtai_fifos.ko

sudo modprobe comedi
sudo modprobe kcomedilib


sudo /sbin/insmod /usr/realtime/modules/rtai_comedi.ko
sudo modprobe ni_pcimio


