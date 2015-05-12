//#include tau.h
#include "marpa.h"

const string pmatches = "http://www.w3.org/2000/10/swap/grammar/bnf#matches";
const string pmbos = "http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence";

typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;
typedef size_t pos;

struct Marpa{
	uint num_syms = 0;
	Marpa_Grammar g;
	map<sym,regex> regexes;
	map<sym,string> names;
	map<pos,size_t> lengths;
	
	parse (filename, grammar)
	{

		sym start = add(grammar["document"]);
		start_symbol_set(start);
		precompute();
		Marpa_Recognizer r = marpa_r_new(g);
		check_null(r);
		marpa_r_start_input(r);



		



		inp = read(filename)
		for(pos p = 0; p != inp.size();;)
		{
		
			//lexing:just find the longest match?
			regex:match m
			sym s;
			for (auto r = regexes.begin(); r != regexes.end(); r++)
				m2 = r.second.match(inp)
				if !m.valid or m.length > m2.length
					m = m2
					s = r.first
					
			if !m.valid die brutally;
			
			int err = marpa_r_alternative(r, s, pos, 1);
			lengths[pos] = m.lenght();
			
			#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
			if (r == MARPA_ERR_UNEXPECTED_TOKEN_ID)
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
		}





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
					
			
			

	}





	//create marpa symbols and rules from grammar description in rdf
	sym add(obj x)
	{
		s = symbol_new();
		if (x.has(pmbos))
			//mbos is supposed to be a list of lists
			for i in x[pmbos]
				rule_new(x, add_list(i))
		elif x.has(pmatches)
			regexes[s] = x[pmatches];
		return s;
	}

	add_list(list x)
	{
		s = symbol_new();
		r = rule_new(s, [add(i) for i in x])
		return s
	}

	
	
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

	void check_int(int result){
		if (result < 0)//== -2)
			error();
		return result;
	}

	void check_null(int result){
		if (result == NULL)
			error();
		return result;
	}

	void error_clear(){
		marpa_g_error_clear(g);
	}

	sym symbol_new()
	{
		num_syms++;
		return check_int(marpa_g_symbol_new(g));
	}

	rule_id rule_new(sym lhs, syms rhs) {
            return check_int(marpa_g_rule_new(g, lhs, rhs, rds.size()));
        }

	rule_id sequence_new(sym lhs, sym rhs, sym separator=-1, int min=1, bool proper=false){
		return check_int(marpa_g_sequence_new(g, lhs, rhs, separator, min,
		    proper ? MARPA_PROPER_SEPARATION : 0));
	}

	void precompute(s):
		check_int(marpa_g_precompute(g));
		print_events();
	}

	void print_events()
	{
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
	}

	void start_symbol_set(sym_id s){
		check_int( marpa_g_start_symbol_set(g, s) );
	}

	void error(){
		int e = marpa_g_error(g, NULL);
		throw(new exception(e + ": " + errors[e]));
	}
}



string load_n3_cmd::desc() const {
	return "load n3";
}
string load_n3_cmd::help() const {
	stringstream ss ( "Usage:" );
	ss << endl << "\ttau expand [JSON-LD input filename]";
	ss << endl << "\ttau expand [JSON-LD input filename] [JSON-LD output to compare to]";
	ss << endl << "If input filename is unspecified, reads from stdin." << endl;
	return ss.str();
}
int load_n3_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 4 ) {
		dout << help();
		return 1;
	}
	pobj grammar = load_json ( "n3.jsonld" );
	#ifdef marpa
	Marpa m;
	m.add(grammar["document"]);
	//...
	#endif
	return 0;
}



	
/*
resources
https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/