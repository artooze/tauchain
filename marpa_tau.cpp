#include "marpa_tau.h"
#include "prover.h"
#include "object.h"
#include "cli.h"
#include "rdf.h"
#include "misc.h"

extern "C" {
#include "marpa.h"
#include "marpa_codes.c"
}

#include "lexertl/generator.hpp"
#include <iostream>
#include "lexertl/iterator.hpp"
#include "lexertl/lookup.hpp"


prover::termset ask(prover *prover, prover::termid s, const pnode p)
{
	prover::termset r = prover::termset();
	prover::termset query;
	prover::termid o_var = prover->tmpvar();
	prover::termid iii = prover->make(p, s, o_var);
	query.emplace_back(iii);
	(*prover)(query);
	
	dout << "query: "<< prover->format(query) << endl;;
	//dout << "substs: "<< std::endl;

	for (auto x : prover->substs) 
	{

		prover::subst::iterator binding_it = x.find(prover->get(o_var).p);
		if (binding_it != x.end())
		{
			r.push_back( (*binding_it).second);
			//prover->prints(x);
			//dout << std::endl;
		}
	}

	prover->substs.clear();
	prover->e.clear();
	dout << r.size() << "." << std::endl;
	return r;
}


const pnode rdfs_first=mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#first"));
const pnode rdfs_rest= mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#rest"));
const pnode rdfs_nil = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
const pnode pmatches = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#matches"));
const pnode pmustbos = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence"));



typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;
typedef std::vector<sym> syms;

class terminal
{
public:
	string name, regex;
	sym symbol;
	terminal(string name_, string regex_, sym symbol_){name = name_; regex = regex_; symbol = symbol_;}
};




struct Marpa{
	sym num_syms = 0;
	Marpa_Grammar g;
	std::vector<terminal> terminals;
	map<prover::termid, sym> done;
	prover* grmr;


	Marpa()
	{
		if (marpa_check_version (MARPA_MAJOR_VERSION ,MARPA_MINOR_VERSION, MARPA_MICRO_VERSION ) != MARPA_ERR_NONE)
			throw std::runtime_error ("marpa version...");
		Marpa_Config marpa_config;
		marpa_c_init(&marpa_config);
		g = marpa_g_new(&marpa_config);
		if (!g)
		{
			stringstream ssss("marpa_g_new: error ");
			ssss  << marpa_c_error (&marpa_config, NULL);
			throw std::runtime_error (ssss.str());
		}
		if(marpa_g_force_valued(g) < 0)
			throw std::runtime_error ("marpa_g_force_valued failed?!?!?");//bah
	}
	~Marpa()
	{
		marpa_g_unref(g);
	}


	void load_grammar(prover *_grmr, pnode root)
	{
		grmr = _grmr;
		sym start = add(grmr, grmr->make(root));
		start_symbol_set(start);
		terminals.push_back(terminal(L"comment", L"#.*", -2));
	}

	string value(prover::termid val)
	{
		return *dict[grmr->get(val).p].value;
	}

	string lexertl_literal(string s){return L"\"" + s + L"\"";}

	//create marpa symbols and rules from grammar description in rdf
	sym add(prover *grmr, prover::termid thing)
	{
		string thingv = value(thing);
		
		if (done.find(thing) != done.end())
			return done[thing];	

		if (grmr->get(thing).isstr())
		{
			dout << "itsa str"<<std::endl;
			thingv = lexertl_literal(thingv);
			for (auto t :terminals)
				if (t.regex == thingv)
					return t.symbol;		
			sym symbol = symbol_new();
			dout << "adding " << thingv << std::endl;
			terminals.push_back(terminal(thingv, thingv, symbol));
			return symbol;
		}

		dout << "adding " << thingv << ",termid:" << thing << std::endl;
	
		sym symbol = symbol_new_termid(thing);
		prover::termset bind;
	
		if((bind = ask(grmr, thing, pmatches)).size())
		{
			dout << "push_back " << thingv << std::endl;
			terminals.push_back(terminal(thingv, value(bind[0]), symbol));
		}

		// note that in the grammar file things and terminology are switched,
		// mustBeOneSequence is a sequence of lists
		
		else if ((bind = ask(grmr, thing, pmustbos)).size())
		{
			for (auto l : bind) // thing must be one of these lists
			{
				dout << "[";
				syms rhs;
				while(1)
				{
					dout << "l: ";
					dout << l;
					dout << "..." << std::endl;
			
					prover::termset first = ask(grmr, l, rdfs_first);
					if (!first.size()) break;
					rhs.push_back(add(grmr, first[0]));
				
					prover::termset rest = ask(grmr, l, rdfs_rest);
					if (!rest.size()) {dout << "wut" << std::endl; break;}//err
					l = rest[0];
				}
				dout << "]" << std::endl;
				
				rule_new(symbol, rhs);
			}
		}
		else
			dout << "nope\n";

		dout << "added "<<symbol << std::endl;
		return symbol;
	}


int check_int(int result){
	if (result == -2)
		error();
	return result;
}

void check_null(void* result){
	if (result == NULL)
		error();
}

void error_clear(){
	marpa_g_error_clear(g);
}

sym symbol_new()
{
	num_syms++;
	return check_int(marpa_g_symbol_new(g));

}

sym symbol_new_termid(prover::termid thing)
{
	return done[thing] = symbol_new();

}

rule rule_new(sym lhs, syms rhs) {
		return check_int(marpa_g_rule_new(g, lhs, &rhs[0], rhs.size()));
	}

rule sequence_new(sym lhs, sym rhs, sym separator=-1, int min=1, bool proper=false){
	return check_int(marpa_g_sequence_new(g, lhs, rhs, separator, min, proper ? MARPA_PROPER_SEPARATION : 0));
}


void print_events()
{	
	
	int count = check_int(marpa_g_event_count(g));
	dout << count << "events" << std::endl;
	if (count > 0)
	{
		Marpa_Event e;
		for (int i = 0; i < count; i++)
		{
			int etype = check_int(marpa_g_event(g, &e, i));
			dout << etype << ", " << e.t_value << std::endl;
		}
	}
	
}

void start_symbol_set(sym s){
	check_int( marpa_g_start_symbol_set(g, s) );
}

void error()
{
	
	int e = marpa_g_error(g, NULL);
	stringstream s;
	s << e << ":" << marpa_error_description[e].name << " - " << marpa_error_description[e].suggested ;
	throw(std::runtime_error(s.str()));// + ": " + errors[e]));
	
}


void parse (const string inp)
{
	
	Marpa_Recognizer r = marpa_r_new(g);
	check_null(r);
	marpa_r_start_input(r);
	

	//const std::string inp = ws(inp_);
	
	
	lexertl::wrules rules;
	lexertl::wstate_machine sm;

	dout << "terminals:\n";
	for (auto t:terminals)
	{
		dout << "(" << t.symbol << ")" << t.name << ": " << t.regex << std::endl;
		rules.push(t.regex, t.symbol);
	}
	dout << "build..\n";
	lexertl::wgenerator::build(rules, sm);

	//std::string input("abc012Ad3e4");
	
	lexertl::wsiterator iter(inp.begin(), inp.end(), sm);
	
	
	lexertl::wsiterator end;

	dout << "go..\n";
	int tok_id = -1;

	for (; iter != end; ++iter)
	{
		tok_id++;
		if(iter->id == 18446744073709551615)
			continue;
		
		sym s = iter->id;

		print_terminal(iter->id);
		dout << ", Text: '" << iter->str() << "'\n";
	
		//#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
		int err = marpa_r_alternative(r, s, tok_id+1, 1);

		if (err == MARPA_ERR_UNEXPECTED_TOKEN_ID)
		{
			std::vector<sym> expected;
			expected.resize(num_syms);
			int num_expected = check_int(marpa_r_terminals_expected(r, &expected[0]));
			dout << "expecting:" << std::endl;
			for (int i = 0; i < num_expected; i++) 
			{
				dout << " ";
				print_terminal(expected[i]);
				dout << std::endl;
			}
		}
		else if (err != MARPA_ERR_NONE)
			dout << err << ":" << marpa_error_description[err].name << " - " << marpa_error_description[err].suggested ;

		check_int(marpa_r_earleme_complete(r));
	}

    	dout << "parsing.." << std::endl;

/*
	//marpa allows variously ordered lists of ambiguous parses, we just grab the default


	Marpa_Bocage b = marpa_b_new(r, -1);
	check_null(b);
	Marpa_Order o = marpa_o_new(b);
	check_null(o);
	Marpa_Tree t = marpa_t_new(o);
	check_null(t);
	check_int(marpa_t_next(t));
	Marpa_Value marpa_v_new(t);
	check_null(v);


	//"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched, we build the "value" of our ast




	while(1)
	{
		Marpa_Step_Type st = check_int(marpa_v_step_type(v));
		if (st == MARPA_STEP_INACTIVE) break;
		switch(st)
		{
			case MARPA_STEP_TOKEN:
				sym id = marpa_v_symbol(v);
				pos p = marpa_v_token_value(v) - 1;
				stack[marpa_v_result(v)] = iter[pos];
				break;
			case MARPA_STEP_RULE:
				rule r = marpa_v_rule(v);
				

*/

}

void print_terminal(sym s)
{
	for (auto t:terminals)
		if (t.symbol == s)
		{
			dout << "(" << t.symbol << ")" << t.name;
			if (t.name != t.regex)
				dout << ": " << t.regex;
		}
}


void precompute(){
	check_int(marpa_g_precompute(g));
	print_events();
}


};


std::string load_n3_cmd::desc() const {return "load n3";}
std::string load_n3_cmd::help() const 
{
	stringstream ss("Hilfe! Hilfe!:");
	return ss.str();
}


#include <iostream>
#include <fstream>


string load(string fname)
{
	std::ifstream t(ws(fname));
 	if (!t.is_open())
		throw std::runtime_error("couldnt open file");// \"" + fname + L"\"");

	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return ws(str);
}

int load_n3_cmd::operator() ( const strings& args )
{	
 	string input = load(args[2]);

	prover prover(convert(load_json(L"n3-grammar.jsonld")));
	Marpa m;
	m.load_grammar(
		&prover, 
		//mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#objecttail")));
		mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#document")));
	m.precompute();
	
	m.parse(input);
	return 0;
}




/*
---------------------






*/


/*
resources
https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/


