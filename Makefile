

OBJS.w_sqlite  = sqlite.o sqlite_wrapper.o sqlite_shell sqcpp_test
CPPFLAGS += -I. -Wall -fPIC -O3

all: $(OBJS.w_sqlite)

sqlite.o:
	$(COMPILE.c) -o $@ sqlite3.c

sqlite_wrapper.o: sqlite.o
	$(COMPILE.cc) -o $@ sqcpp.cc 

sqlite_shell: sqlite.o
	$(COMPILE.c) -o sqlite_shell.o shell.c
	$(LINK.c) -o sqlite_shell sqlite_shell.o sqlite.o -ldl -lpthread

sqcpp_test: sqlite.o sqlite_wrapper.o
	$(COMPILE.cc) -o sqcpp_test.o sqcpp_test.cc
	$(LINK.cc) -o $@ sqlite.o sqlite_wrapper.o sqcpp_test.o  -ldl -lpthread

clean: 
	$(RM) -rf *.o *~ sqcpp_test sqlite_shell
