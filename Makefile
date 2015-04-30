CC=g++
CXXFLAGS=-c -std=c++1y -Wall -rdynamic -Wextra -W -O2
LDFLAGS=-lcurl
OBJECTS=tau.o jsonld.o rdf.o reasoner.o misc.o

all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

tau.o: tau.cpp cli.h reasoner.h parsers.h jsonld.h json_spirit.h object.h \
 strings.h rdf.h misc.h
jsonld.o: jsonld.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
rdf.o: rdf.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
reasoner.o: reasoner.cpp reasoner.h parsers.h jsonld.h json_spirit.h \
 object.h strings.h rdf.h misc.h
misc.o: misc.cpp reasoner.h parsers.h jsonld.h json_spirit.h object.h \
 strings.h rdf.h misc.h

debug: CXXFLAGS += -DDEBUG

debug: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
ubi-tau: $(OBJECTS) ubi/client.o
	$(CC) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS) ubi/client.o

ppjson: ppjson.cpp
	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb
