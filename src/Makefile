# Compiler
CXX = g++-14

# Compiler flags
CXXFLAGS = -Wall -g -std=c++23
#CXXFLAGS = -Wall -g -std=c++23 -fmodules-ts

# Target executable
TARGET = hdldb

# Source files
#SRCS = hdldb.cpp Shadow.cpp Trace.cpp Protocol.cpp
SRCS = rsp/Socket.cpp rsp/Packet.cpp test/packet_test.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule to build and run the executable
all: $(TARGET)
#all: system-headers $(TARGET)

packet_test: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm packet_test $(TARGET) $(OBJS)