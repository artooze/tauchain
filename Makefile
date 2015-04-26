CC=g++
CFLAGS=-c -std=c++1y -Wall -rdynamic -ggdb
LDFLAGS=-lcurl
SOURCES=tau.cpp jsonld.cpp rdf.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tau

all: $(SOURCES) $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS)


#all: jsonld.h  json_spirit.h parsers.h reasoner.h strings.h rdf.h logger.h object.h tau.o jsonld.o rdf.o
#	$(CC) tau.o jsonld.o rdf.o -lcurl -otau
#debug: jsonld.h  json_spirit.h parsers.h reasoner.h strings.h rdf.h logger.h object.h tau.o jsonld.o rdf.o
#	$(CC) tau.o jsonld.o rdf.o -lcurl -otau
#nquads: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
#	clang++ -std=c++1y tau.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -g
#ppjson: ppjson.cpp
#	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb
#
