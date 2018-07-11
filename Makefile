SOURCES = ftptest.cpp SocketDelegate.cpp FtpServer.cpp
OBJS = $(SOURCES:%.cpp=%.o)

DEBUGFLAG =
CFLAGS = -std=c++11 -Wno-unused-value ${DEBUGFLAG}
LDFLAGS= -pthread

ftptest:$(OBJS)
		clang++ -o$@ ${OBJS} ${LDFLAGS}
ftptest.o:FtpServer.o SocketDelegate.o

FtpServer.o:SocketDelegate.o

$(OBJS):%.o:%.cpp
		clang++ -c $< ${CFLAGS}

clean:
		-rm ${OBJS} ftptest 
