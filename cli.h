#ifndef __CLI_H__
#define __CLI_H__

#include <string>
#include <vector>
#include "rdf.h"

using namespace jsonld;

typedef vector<string> strings;

extern bool fnamebase;
extern jsonld::jsonld_options opts;

class cmd_t {
public:
	virtual string desc() const = 0;
	virtual string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;

	pobj load_json ( string fname = "", bool print = false );
	pobj load_json ( const strings& args );
	pobj nodemap ( const strings& args );
	pobj nodemap ( pobj o );
	qdb toquads ( const strings& args );
	qdb toquads ( pobj o );
	qdb convert ( pobj o );
	qdb convert ( const string& s, bool bdebugprint = false);
};

typedef pair<map<string, cmd_t*>, map<pair<string, string>, bool*>>  cmds_t;

void print_usage ( const cmds_t& cmds );
void process_flags ( const cmds_t& cmds, strings& args );

#endif
