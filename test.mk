# See LICENSE file for copyright and license details
TEST = test/queue test/socketmap

ifeq ($(COVERAGE),1)
CFLAGS+=-DUNIT_TESTING -fprofile-arcs -ftest-coverage
LDLIBS+=--coverage
endif

$(TEST): CFLAGS+=-g -DUNIT_TESTING
$(TEST): % : %.o
$(TEST): LDLIBS+=-lcmocka
test/queue test/socketmap: src/workqueue.o
