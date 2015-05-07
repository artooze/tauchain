C=gcc
CC=g++
CXXFLAGS=-c -std=c++1y -Wall -Wextra -W -ggdb -Wpedantic
LDFLAGS=-lcurl -lboost_system -lboost_filesystem -pthread
OBJECTS=tau.o jsonld.o rdf.o proof.o misc.o object.o cli.o
COBJECTS=prover.o

all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(C) $(COBJECTS) -o $@ 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

prover.o: prover.c prover.h
tau.o: tau.cpp cli.h rdf.h object.h parsers.h jsonld.h json_spirit.h strings.h proof.h misc.h prover.h
jsonld.o: jsonld.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
rdf.o: rdf.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
proof.o: proof.cpp proof.h misc.h rdf.h object.h parsers.h jsonld.h json_spirit.h strings.h
misc.o: misc.cpp proof.h misc.h rdf.h object.h parsers.h jsonld.h json_spirit.h strings.h
cli.o: cli.cpp proof.h misc.h rdf.h object.h jsonld.h json_spirit.h strings.h parsers.h cli.h
object.o: object.cpp object.h

debug: CXXFLAGS += -DDEBUG

debug: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS) $(COBJECTS)

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
