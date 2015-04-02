# Declaration of variables
CC = clang++
CC_FLAGS = -std=c++1y
LD_FLAGS = -lcurl

# File names
EXEC = run
SOURCES = main.cpp #$(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(LD_FLAGS) $(OBJECTS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)

testJsonLdUrl: # JsonLdUrl.h testJsonLdUrl.cpp
	clang++-3.5 -std=c++11 -I/usr/include/ -L/usr/lib/i386-linux-gnu testJsonLdUrl.cpp JsonLdUrl.h 
	clang++-3.5 -std=c++11 -L/usr/lib/i386-linux-gnu testJsonLdUrl.o -o testJsonLdUrl
