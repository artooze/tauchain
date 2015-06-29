#include <codecvt>

#include "marpa.h"
#include "prover.h"
//#include "marpa.h"//not yet

#include <unicode/utypes.h>
#include <unicode/regex.h>
#include "object.h"
#include "cli.h"
#include "rdf.h"
#include "misc.h"

UErrorCode        status    = U_ZERO_ERROR;
UnicodeString    stringToTest = "Find the abc in this string";
RegexMatcher *matcher = new RegexMatcher("abc+", 0, status);

prover::termset ask(prover *prover, prover::termid s, const pnode p)
{
	prover::termset r = prover::termset();
	prover::termset query;
	prover::termid o_var = prover->tmpvar();
	prover::termid iii = prover->make(p, s, o_var);
	query.emplace_back(iii);
	(*prover)(query);
	
	dout << "query: "<< prover->format(query) << endl;;
	dout << "substs: "<< std::endl;

	for (auto x : prover->substs) 
	{
		//prover->prints(x);
		//dout << std::endl;

		prover::subst::iterator binding_it = x.find(prover->get(o_var).p);
		if (binding_it != x.end())
		{
			r.push_back( (*binding_it).second);
		}
	}

	prover->substs.clear();
	return r;
}


const pnode rdfs_first=mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#first"));
const pnode rdfs_rest= mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#rest"));
const pnode rdfs_nil = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
const pnode pmatches = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#matches"));
const pnode pmustbos = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence"));

/*typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;*/
typedef size_t pos;
typedef int sym;
typedef std::vector<sym> syms;
typedef int rule;
struct Marpa{
	uint num_syms = 0;
	//Marpa_Grammar g;
	map<string, sym> regexes;
	map<prover::termid, sym> done;
	map<pos,size_t> lengths;
	prover* grmr;

	void load_grammar(prover *_grmr, pnode root)
	{
		grmr = _grmr;
		sym start = add(grmr, grmr->make(root));
		start_symbol_set(start);
	}


	sym sym4thing(prover::termid thing)
	{
	}
		

	//create marpa symbols and rules from grammar description in rdf
	sym add(prover *grmr, prover::termid thing)
	{
		if (done.find(thing) != done.end())
			return done[thing];
		
		sym symbol = symbol_new(thing);
		
		dout << "is it a mustBeOneSequence?" << std::endl;
		
		prover::termset bind;

		// note that in the grammar file things and terminology are switched,
		// mustBeOneSequence is a sequence of lists
		
		if ((bind = ask(grmr, thing, pmustbos)).size())
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
			
					prover::termset xx = ask(grmr, l, rdfs_first);
					if (!xx.size()) break;
					
					rhs.push_back(add(grmr, xx[0]));
				
					xx = ask(grmr, l, rdfs_rest);
					if (!xx.size()) break;//err
					l = xx[0];
				}
				dout << "]" << std::endl;
				
				rule_new(symbol, rhs);
			}
		}
		else if((bind = ask(grmr, thing, pmatches)).size())
		{
			regexes[dstr(grmr->get(bind[0]).p)] = symbol;
		}
		else
			dout << "nope\n";
	}


int check_int(int result){
	if (result < 0)//== -2)
		error();
	return result;
}

void check_null(void* result){
	if (result == NULL)
		error();
}

void error_clear(){
	//marpa_g_error_clear(g);
}

sym symbol_new(prover::termid thing)
{
	num_syms++;
	done[thing] = 5;//check_int(marpa_g_symbol_new(g));
	return done[thing];
}

rule rule_new(sym lhs, syms rhs) {
		return 5;//check_int(marpa_g_rule_new(g, lhs, rhs, rds.size()));
	}

rule sequence_new(sym lhs, sym rhs, sym separator=-1, int min=1, bool proper=false){
	return 5;//check_int(marpa_g_sequence_new(g, lhs, rhs, separator, min, proper ? MARPA_PROPER_SEPARATION : 0));
}

void precompute(){
	//check_int(marpa_g_precompute(g));
	print_events();
}

void print_events()
{	
	/*
	int count = check_int(marpa_g_event_count(g));
	dout << '%s events'%count
	if (count > 0)
	{
		Marpa_Event e;
		for (int i = 0; i < count; i++)
		{
			int etype = check_int(marpa_g_event(g, &e, i));
			dout << etype << ", " << e.t_value;
		}
	}
	*/
}

void start_symbol_set(sym s){
	//check_int( marpa_g_start_symbol_set(g, s) );
}

void error()
{
	/*
	int e = marpa_g_error(g, NULL);
	throw(new exception(e + ": " + errors[e]));
	*/
}


// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

void parse (string inp)
{
	/*
	Marpa_Recognizer r = marpa_r_new(g);
	check_null(r);
	marpa_r_start_input(r);
	*/
	
	//this loop tokenizes(/lexes/scans) the input stream and feeds the tokens to marpa
	//just find the longest match?
	for(auto pos : inp)
	{
		
		//icu::RegexMatcher m,m2;
		sym s;
		for (auto r : regexes) {
		
		
			std::string ss = wstring_to_utf8(r.first);
			icu::UnicodeString sss (ss);

//			boost::regex_search (pos, inp.end(), m2, r.first);
//			boost::regex_search (inp, m2, r.first);
/*			if (!m.valid or m.length > m2.length) {
				m = m2;
				s = r.second;
			}*/
		}
		//if (!m.valid) throw std::runtime_error("no match");
		
		//p += m.length();
		

		/*
		//#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
		int err = marpa_r_alternative(r, s, pos, 1);

		if (err == MARPA_ERR_UNEXPECTED_TOKEN_ID)
		{
			sym[sym_names.size()] expected;
			num_expected = check_int(marpa_r_terminals_expected(r, sym));
			dout << "expecting:";
			//for (int i = 0; i < num_expected; i++)
			dout << endl;
			error();
		}
		else if (r != MARPA_ERR_NONE)
			error();

		check_int(marpa_r_earleme_complete(r));
		lengths[pos] = m.lenght();
		*/
	}
	

}



};


std::string load_n3_cmd::desc() const {return "load n3";}
std::string load_n3_cmd::help() const 
{
	stringstream ss("Hilfe! Hilfe!:");
	return ss.str();
}
int load_n3_cmd::operator() ( const strings& args )
{	
	prover prover(convert(load_json(L"n3-grammar.jsonld")));
	Marpa m;
	m.load_grammar(
		&prover, 
		mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#objecttail")));
		//mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#document")));
	m.precompute();
	m.parse(args[2]);
	return 0;
}




/*
---------------------





	Grammar()
	{
		if (marpa_check_version (MARPA_MAJOR_VERSION ,MARPA_MINOR_VERSION, MARPA_MICRO_VERSION ) != MARPA_ERR_NONE)
			throw runtime_error ("marpa version...");
		Marpa_Config marpa_config;
		marpa_c_init(&marpa_config);
		g = marpa_g_new(marpa_config);
		if (!g)
			throw runtime_error ("marpa_g_new: error %d" % marpa_c_error (&marpa_config, NULL));
		if(lib.marpa_g_force_valued(s.g) < 0)
			throw runtime_error ("marpa_g_force_valued failed?!?!?");
	}
	~Grammar()
	{
		marpa_g_unref(g);
	}







*/


/*
resources
https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/




/*



	//marpa allows variously ordered lists of ambiguous parses,
	we just grab the default


	Marpa_Bocage b = marpa_b_new(r, -1);
	check_null(b);
	Marpa_Order o = marpa_o_new(b);
	check_null(o);
	Marpa_Tree t = marpa_t_new(o);
	check_null(t);
	check_int(marpa_t_next(t));
	Marpa_Value marpa_v_new(t);
	check_null(v);



	//"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched, we build the ast with our "values"


	while(1)
	{
		Marpa_Step_Type st = check_int(marpa_v_step(v));
		if (st == MARPA_STEP_INACTIVE) break;
		switch(st)
		{
			case MARPA_STEP_TOKEN:
				sym id = marpa_v_symbol(v);

				obj x;
				x["name"] = names[id];

				pos p = marpa_v_token_value(v);
				stack[marpa_v_result(v)] = input[p:lengths[p]]

				break;
			case MARPA_STEP_RULE:

*/