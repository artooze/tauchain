#ifdef IRC
#include "pstream.h"
#endif
#include "prover.h"
#include "jsonld.h"
#include "parsers.h"
#include "cli.h"
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

std::shared_ptr<qdb> cmd_t::load_quads ( string fname, bool print ) {
	qdb q;
	bool empty = true;
	try {
		qdb r;
//#ifndef IRC
		std::wistream* pis = &std::wcin;
		if (fname != L"")
			pis = new std::wifstream(ws(fname));
		std::wistream& is = *pis;
//#else		
//		string pp = string("tail -n 0 -f /tmp/irc.freenode.net/#")+chan+"/out";
//		static istream* pis = new redi::ipstream(pp/*, redi::pstreams::pstdout | redi::pstreams::newpg*/);
//		cerr<<pp<<endl;
//#endif			
//		istream& is = *pis;
		string line;
		std::wstringstream ss;
		while (!is.eof()) {
			getline(is, line);
			trim(line);
			empty = false;
			if (!line.size() || line == L"\n")
				continue;
			static string magic = L"botau: ", lvl = L"level ", fin = L"fin.";
#ifdef IRC
			auto pos = line.find(magic);
			if (pos == string::npos)
				continue;
			line = line.substr(pos + magic.size());
#endif
			trim(line);
			if (startsWith(line, lvl)) {
				try {
				line = line.substr(lvl.size());
				int n = 0;
				while (std::iswdigit(line[++n]));
				level = std::stoi(line.substr(0, n));
				dout << "level changed to " << level << std::endl;
				} catch (std::exception& ex) { derr<<ex.what()<<std::endl; }
				catch (...) {}
				continue;
			}
//			cout << line;
			if (endsWith(line, fin)) {
				string x = line.substr(0, line.size() - fin.size());
				trim(x);
				ss << x;
				break;
//				return empty ? 0 : std::make_shared<qdb>(readqdb(ss));
			}
			ss << line;
		}
#ifndef IRC		
		if (fname != L"")
			delete pis;
#endif			
		return empty ? 0 : std::make_shared<qdb>(readqdb(ss));
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
		if (!is.is_open())
			throw std::runtime_error("couldnt open file");// \"" + fname + L"\"");
		if (!json_spirit::read_stream ( is, v ))
			throw std::runtime_error("couldnt load json");
	}
	pobj r =  jsonld::convert ( v );
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
	jsonld::jsonld_api a ( opts );
	a.gen_node_map ( o, nodeMap );
	return mk_somap_obj ( nodeMap );
}

qdb cmd_t::toquads ( const strings& args ) {
	return toquads ( load_json ( args ) );
}

qdb cmd_t::toquads ( pobj o ) {
	jsonld::jsonld_api a ( opts );
	rdf_db r ( a );
	auto nodeMap = o;
	for ( auto g : *nodeMap->MAP() ) {
		if ( jsonld::is_rel_iri ( g.first ) ) continue;
		if ( !g.second || !g.second->MAP() ) throw wruntime_error ( L"Expected map in nodemap." );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	//	dout << "Converting: " << std::endl << o->toString() << std::endl;
	//	dout << "Converted: " << std::endl << r << std::endl;
	return r;
}

qdb cmd_t::convert ( pobj o ) {
	return toquads ( nodemap ( jsonld::expand ( o, opts ) ) );
}

qdb cmd_t::convert ( const string& s, bool debugprint ) {
	if ( fnamebase ) opts.base = pstr ( string ( L"file://" ) + s + L"#" );
	if ( debugprint ) dout << L" Converting: " << s;
	qdb r = convert ( load_json ( s ) );
	if ( debugprint ) dout << L" Converted: " << r << std::endl;
	return r;
}

void process_flags ( const cmds_t& cmds, strings& args ) {
	strings::iterator it;
	for ( auto x : cmds.second )
		if ( ( it = find ( args.begin(), args.end(), x.first.first ) ) != args.end() ) {
			*x.second = !*x.second;
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
	dout << std::endl;
}
