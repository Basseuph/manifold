CROSS_CC_PREFIX=aarch64-linux-gnu-

CC=$(CROSS_CC_PREFIX)gcc
GPP=$(CROSS_CC_PREFIX)g++
LD=$(CROSS_CC_PREFIX)ld
RANLIB=$(CROSS_CC_PREFIX)corelib

LDFLAGS=-bstatic --unresolved-symbols=report-all

CFLAGS=-Wall -static

tar: runnable
	tar -cf systemc_test.tar systemc_test runme.sh

systemc_test:
	$(GPP) $(CFLAGS) $@.cpp -o $@

runnable: systemc_test
	chmod +x systemc_test
	
clean:
	rm -f systemc_test systemc_test.o systemc_test.tar

all:	tar deploy

deploy: systemc_test.tar
	cp systemc_test.tar ../

.PHONY: all tar clean runnable
