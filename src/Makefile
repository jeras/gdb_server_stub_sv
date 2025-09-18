# Compiler
CXX = g++-14

# Compiler flags
CXXFLAGS = -Wall -g -std=c++23
#CXXFLAGS = -Wall -g -std=c++23 -fmodules-ts

# Target executable
TARGET = hdldb

# Source files
SRCS = hdldb.cpp Shadow.cpp Trace.cpp Protocol.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule to build and run the executable
all: $(TARGET)
#all: system-headers $(TARGET)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm $(TARGET) $(OBJS)