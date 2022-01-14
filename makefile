.PHONY : tc ts clean install uninstall

all: tc ts

tc:
	cd tc; make; cd ..

ts:
	cd ts; make; cd ..


clean:
	cd tc; make clean; cd ..
	cd ts; make clean; cd ..

install:
	sudo cp bin/tc /usr/sbin
	sudo cp bin/ts /usr/sbin

uninstall:
	sudo rm /usr/sbin/tc /usr/sbin/ts

