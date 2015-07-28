#ifdef IRC
#include "pstream.h"
#endif
#include "prover.h"
#include "jsonld.h"
#include "cli.h"
#include "json_spirit.h"

//so, i know some of the things Ohad's purposely moved around for optimization but idk how much is for these reasons or just for lack of organization
//fileio and format conversions seems like something to be separated from strict command-line processing
//dunno..i dont think a few short .h files can bog anything down..i would look for what they include maybe..like if it includes some boost header..especially and i think only boost is what's slowing it all down..and json_spirit , i see, well its not like our compilation time is terrible... about a couple minutes tops? on my slow computer..yeah..well..i wouldnt mind if it were faster:):) one thing i think is that it would be easier to track all these dependencies from a more logically organized structure.. when everything is optimized prematurely you don't know what's what or why//well..give it a try..youll see if you can wrestle with it:)yea.. hopefully i even clean things up in such a way to make the compile go faster :) that would be nice//yeah..if the logical structure could match the optimized structure that would be ideal... but is maybe just an ideal :) lots to figure out to determine that.
//COMMAND-LINE
/*
Definitions to match the declarations i made in cli.h
*/
bool autobt = false, _pause = false, __printkb = false, fnamebase = true, quad_in = false, nocolor = false, deref = true, shorten = false;

jsonld_options opts;
std::string test_loadn3;



//FILE-IO & CONVERSION
pobj  convert ( const json_spirit::wmValue& v );
json_spirit::wmValue convert ( obj& v );
json_spirit::wmValue convert ( pobj v );

std::shared_ptr<qdb> cmd_t::load_quads ( string fname, bool print ) {
	qdb q;
	try {
		qdb r;
		std::wistream* pis = &std::wcin;
		if (fname != L"")
			pis = new std::wifstream(ws(fname));
		std::wistream& is = *pis;
		return std::make_shared<qdb>(readqdb(is));
	} catch (std::exception& ex) {
		derr << L"Error reading quads: " << ex.what() << std::endl;
	}
	if ( print ) 
		dout << q << std::endl;

	return std::make_shared<qdb>(q);
}

pobj cmd_t::load_json ( string fname, bool print ) {
	json_spirit::wmValue v;
	if ( fname == L"" ) json_spirit::read_stream ( std::wcin, v );
	else {
		std::wifstream is ( ws(fname) );
		if (!is.is_open()) throw std::runtime_error("couldnt open file");
		if (!json_spirit::read_stream ( is, v )) throw std::runtime_error("couldnt load json");
	}
	pobj r =  ::convert ( v );
	if ( !r ) throw wruntime_error ( L"Couldn't read input." );
	if ( print ) dout << r->toString() << std::endl;
	return r;
}

pobj cmd_t::load_json ( const strings& args ) {
	return load_json ( args.size() > 2 ? args[2] : L"" );
}

pobj cmd_t::nodemap ( const strings& args ) {
	return nodemap ( load_json ( args[2] ) );
}

pobj cmd_t::nodemap ( pobj o ) {
	psomap nodeMap = make_shared<somap>();
	( *nodeMap ) [str_default] = mk_somap_obj();
	jsonld_api a ( opts );
	a.gen_node_map ( o, nodeMap );
	return mk_somap_obj ( nodeMap );
}

qdb cmd_t::toquads ( const strings& args ) {
	return toquads ( load_json ( args ) );
}

qdb cmd_t::toquads ( pobj o ) {
	jsonld_api a ( opts );
	rdf_db r ( a );
	auto nodeMap = o;
	std::map<string, pnode> lists;
	for ( auto g : *nodeMap->MAP() ) {
		if ( is_rel_iri ( g.first ) ) continue;
		if ( !g.second || !g.second->MAP() ) throw wruntime_error ( L"Expected map in nodemap." );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	return r;
}

qdb cmd_t::convert ( pobj o ) {
	return toquads ( nodemap ( expand ( o, opts ) ) );
}

qdb cmd_t::convert ( const string& s ) {
	if ( fnamebase ) opts.base = pstr ( string ( L"file://" ) + s + L"#" );
	qdb r = convert ( load_json ( s ) );
	return r;
}



//COMMANDLINE
void process_flags ( const cmds_t& cmds, strings& args ) {
	strings::iterator it;
	for ( auto x : cmds.second )
		if ( ( it = find ( args.begin(), args.end(), x.first.first ) ) != args.end() ) {
			//ok so this *essentially* loops through each of the boolean-type arguments like --nocolor and --no-deref and then Ohad just toggles the boolean:
			//I commented this out to demonstrate that the TCLAP stuff is working..ahh...so..you can eventually just delete all of this and dont deal with any lists like that(?).. well i think it would be nice to handle it this way

			//ok so we see this is commented so any change can't be coming from here
			//*x.second = !*x.second;
			args.erase ( it );
		}
}

void print_usage ( const cmds_t& cmds ) {
	dout << std::endl << L"Tau-Chain by http://idni.org" << std::endl;
	dout << std::endl << L"Usage:" << std::endl;
	dout << L"\ttau help <command>\t\tPrints usage of <command>." << std::endl;
	dout << L"\ttau <command> [<args>]\t\tRun <command> with <args>." << std::endl;
	dout << std::endl << L"Available commands:" << std::endl << std::endl;
	for ( auto c : cmds.first ) dout << tab << c.first << tab << ws(c.second->desc()) << std::endl;
	dout << std::endl << L"Available flags:" << std::endl << std::endl;
	for ( auto c : cmds.second ) dout << tab << c.first.first << tab << c.first.second << std::endl;
	dout << tab << L"--level <depth>" << tab << L"Verbosity level" << std::endl;
	dout << std::endl;
}
