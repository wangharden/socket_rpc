CXX = g++
CXXFLAGS = -g -I./include -std=c++20
PLATFORM := $(OS)
COLLECT_SOURCES = $(wildcard ./source/*.cpp)

SOURCES = $(COLLECT_SOURCES)

ifeq ($(PLATFORM),Windows_NT)
    SOURCES = $(subst /,\,$(COLLECT_SOURCES))
endif


SERVER_SOURCES = $(SOURCES) Server.cpp
SERVER_OBJECTS := $(SERVER_SOURCES:.cpp=.o)

CLIENT_SOURCES = $(SOURCES) Client.cpp
CLIENT_OBJECTS := $(CLIENT_SOURCES:.cpp=.o)

LDFLAGS = 
RM = del /Q
SERVER_EXECUTABLE = Server
CLIENT_EXECUTABLE = Client



$(warning SERVER_SOURCES is $(SERVER_SOURCES))
$(warning CLIENT_SOURCES is $(CLIENT_SOURCES))
$(warning PLATFORM is ${PLATFORM})

ifeq ($(PLATFORM),Windows_NT)
	LDFLAGS += -lws2_32
else
	RM = rm -f
endif


all: client server clean


server: $(SERVER_EXECUTABLE)

client: $(CLIENT_EXECUTABLE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SERVER_EXECUTABLE) : $(SERVER_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(CLIENT_EXECUTABLE) : $(CLIENT_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(SERVER_OBJECTS) $(CLIENT_OBJECTS)

.PHONY: clean