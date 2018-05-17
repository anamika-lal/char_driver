obj-m = my_char_drv.o

all:
	make -C /lib/modules/4.8.0-36-generic/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/4.8.0-36-generic/build/ M=$(PWD) clean
	
