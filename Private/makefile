CXX = g++

SONAME = Cacus.so

OTHERSTUFF = -Wl
CXXFLAGS = -std=gnu++17 -msse2 -m32 -fpermissive -fno-strict-aliasing -O2 -Wall -fvisibility=hidden -I. -I../Cacus
LDFLAGS = -shared -m32 -rdynamic -lc -ldl -pthread -lpthread -Wl,-z,defs -Wl,-soname,$(SONAME)
OUTPUT = $(SONAME)
LIBS = 

SRCS = BaseDir.cpp	\
	Cacus.cpp	\
	CacusOutputDevice.cpp	\
	CacusString.cpp	\
	CircularBuffer.cpp	\
	DebugCallback.cpp	\
	Thread.cpp	\
	Time.cpp	\
	Ticker.cpp	\
	CacusField.cpp	\
	COutputDevicePrintf.cpp	\
	Parsers.cpp	\
	StackUnwinder.cpp	\
	Unicode.cpp	\
	URI.cpp	\
	NetworkSocket.cpp	\
	IPv6.cpp

OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: $(OUTPUT)

#Link
$(OUTPUT) : $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)
	trip --strip-all --discard-all $(OUTPUT)

#Compile
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) -o $@ $< > $*.lst


#Generate dependancies
%.d : %.cpp
	@echo $(@:%.d=%.o) $@ : \\ > $@
	@/bin/sh -ec '$(CXX) -MM $(CXXFLAGS) $< | \
		sed -e '\''s/.*://'\'' >> $@'


#include $(SRCS:.cpp=.d)

.PHONY: clean
clean:
	-${RM} -f ${OBJS} $(SRCS:.cpp=.lst)
	-${RM} -f ${OBJS} $(SRCS:.cpp=.d)

