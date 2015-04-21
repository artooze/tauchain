// c++ port of euler (EYE) js reasoner
#include <deque>
#include "parsers.h"

bool DEBUG = true;
#define trace(x) if(DEBUG) { x }

struct pred_t {
	string pred;
	vector<pred_t> args;

	ostream& write ( ostream& o ) const {
		o << pred;
		if ( args.size() ) {
			o << "(";
			for ( auto a = args.cbegin();; ) {
				a->write ( o );
				if ( ++a != args.cend() ) o << ", ";
				else return o << ")";
			}
		}
		return o;
	}

	operator string() const {
		stringstream ss;
		write ( ss );
		return ss.str();
	}
};

int _indent = 0;

string indent() {
	stringstream ss;
	for ( int i = 0; i < _indent; i++ ) ss << "##";
	return ss.str();
}

typedef map<string, pred_t> env_t;
typedef std::shared_ptr<env_t> penv_t;

ostream& operator<< ( ostream& o, env_t const& r ) {
	o << "env of size " << r.size() << "{";
	for ( auto rr : r ) o << rr.first << ": " << ( string ) rr.second << "; ";
	return o << "}";
}

struct rule_t {
	pred_t head;
	vector<pred_t> body;
	operator string() const {
		stringstream o;
		o << ( string ) head << " :- ";
		for ( auto b : body ) o << ( string ) b << "; ";
		return o.str();
	}
};

struct rule_env {
	rule_t src;
	penv_t env;
	operator string() const {
		stringstream o;
		o << ( string ) src << " {" << *env << "} ";
		return o.str();
	}
};
typedef vector<rule_env> ground_t;
typedef std::shared_ptr<ground_t> pground_t;

pground_t aCopy ( pground_t f ) {
	pground_t r = make_shared<ground_t>();
	*r = *f;
	return r;
}

ostream& operator<< ( ostream& o, ground_t const& g ) {
	for ( auto gg : g ) o << "_" << ( string ) gg << "_";
	return o;
}

struct proof_trace_item {
	rule_t rule;
	int src, ind; // source of what, index of what?
	std::shared_ptr<proof_trace_item> parent;
	std::shared_ptr<env_t> env;
	std::shared_ptr<ground_t> ground;
	operator string() const {
		stringstream o;
		o << "<<" << ( string ) rule << src << "," << ind << "(";
		if ( parent ) o << ( string ) ( *parent );
		o << ") {env:" << ( *env ) << "}[[ground:" << ( *ground ) << "]]";
		return o.str();
	}
};
typedef std::shared_ptr<proof_trace_item> ppti;

int builtin ( pred_t, proof_trace_item ) {
	/*
	 * 1:it unified
	 * -1:no such builtin
	 * 0:it didnt unify
	 */
	trace ( cout << "NO BEES yet PLZ!" << endl; )
	return -1;
}
typedef map<string, vector<rule_t>> evidence_t;

pred_t evaluate ( pred_t t, penv_t env );
bool unify ( pred_t s, penv_t senv, pred_t d, penv_t denv, bool f );

pground_t gnd = make_shared<ground_t>();

bool prove ( pred_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	/*
    * please write an outline of this thing:)
    */
	int step = 0;
	deque<ppti> queue;
	ppti s = make_shared<proof_trace_item> ( proof_trace_item { {goal, {goal}}, 0, 0, 0, make_shared<env_t>(), gnd } ); //TODO: don't deref null parent ;-)//done?
	queue.emplace_back ( s );
	//queue.push_back(s);
	trace ( cout << "Goal: " << ( string ) goal << endl; )
	while ( queue.size() > 0 ) {
		trace ( cout << "=======" << endl; )
		ppti c = queue.front();
		trace ( cout << "  c: " << ( string ) ( *c ) << endl; )
		queue.pop_front();
		pground_t g = aCopy ( c->ground );
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) {
			trace ( cout << "TIMEOUT!" << endl; )
			return true;
		}
		trace (
		    cout << "c.ind: " << c->ind << endl;
		    cout << "c.rule.body.size(): " << c->rule.body.size() << endl; //in step 1, rule body is goal
		)
		// all parts of rule body succeeded...(?)
		if ( (size_t)c->ind >= c->rule.body.size() ) {
			if ( !c->parent ) {
				trace ( cout << "no parent!" << endl; )
				for ( size_t i = 0; i < c->rule.body.size(); i++ ) {
					pred_t t = evaluate ( c->rule.body[i], c->env );
					rule_t tmp = {t, {{ "GND", {}}} };
					trace ( cout << "Adding evidence for " << ( string ) t.pred << ": " << ( string ) tmp << endl; )
					evidence[t.pred].push_back ( tmp );
				}
				continue;
			}
			trace ( cout << "Q parent: "; )
			if ( c->rule.body.size() != 0 ) g->push_back ( {c->rule, c->env} );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->parent->rule, c->parent->src, c->parent->ind, c->parent->parent, c->parent->env, g} );
			unify ( c->rule.head, c->env, r->rule.body[r->ind], r->env, true );
			r->ind++;
			trace ( cout << ( string ) ( *r ) << endl; )
			queue.push_back ( r );
			continue;
		}
		trace ( cout << "Done q" << endl; )
		pred_t t = c->rule.body[c->ind];
		size_t b = builtin ( t, *c );
		if ( b == 1 ) {
			g->emplace_back ( rule_env { { evaluate ( t, c->env ), vector<pred_t>() }, make_shared<env_t>() } );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->rule, c->src, c->ind, c->parent, c->env, g} );
			r->ind++;
			queue.push_back ( r );
			continue;
		} else if ( b == 0 ) { // builtin didnt unify
			continue;
		} // else there is no such builtin, continue...

		trace ( cout << "Checking cases..." << endl; )
		if ( cases.find ( t.pred ) == cases.end() ) {
			trace ( cout << "No Cases(no such predicate)!" << endl; )
			continue;
		}
		size_t src = 0;
		//for each rule with the predicate we are trying to prove...
		for ( rule_t rl : cases[t.pred] ) {
			src++;
			pground_t g = aCopy ( c->ground );
			trace ( cout << "Check rule: " << ( string ) rl << endl; )
			if ( rl.body.size() == 0 ) g->push_back ( { rl, make_shared<env_t>() } ); //its a conditionless fact
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {rl, ( int ) src, 0, c, make_shared<env_t>(), g} );// why already here and not later?
			//rl could imply our rule...
			if ( unify ( t, c->env, rl.head, r->env, true ) ) {
				ppti ep = c;
				while ( ( ep = ep->parent ) ) {
					trace ( cout << "  ep.src: " << ep->src << endl << "  c.src: " << c->src << endl; )
					if ( ep->src == c->src && unify ( ep->rule.head, ep->env, c->rule.head, c->env, false ) ) {
						trace ( cout << "  ~~ match ~~ " << endl; )
						break;
					}
					trace ( cout << "  ~~  ~~  ~~" << endl; )
				}
				if ( !ep ) {
					trace ( cout << "Adding to queue: " << ( string ) ( *r ) << endl << flush; )
					queue.push_front ( r );
				} else trace ( cout << "didn't reach top" << endl; )
					trace ( cout << "Done euler loop" << endl; )
				} else trace ( cout << "No loop here" << endl; )
			}
		trace ( cout << "done rule checks, looping" << endl; )
	}
	return false;
}

bool unify ( pred_t s, penv_t senv, pred_t d, penv_t denv, bool f ) {
	trace ( cout << indent() << "Unify:" << endl << flush << indent() << "  s: " << ( string ) s << " in " << ( *senv ) << endl << flush;
	        cout << indent() << "  d: " << ( string ) d << " in " << ( *denv ) << endl << flush; )
	if ( s.pred[0] == '?' ) {
		trace ( cout << indent() << "  source is var" << endl << flush; )
		try {
			pred_t sval = evaluate ( s, senv );
			_indent++;
			bool r = unify ( sval, senv, d, denv, f );
			_indent--;
			return r;
		} catch ( ... ) {
			if ( _indent ) _indent--;
			trace ( cout << indent() << " Match(free var)!" << endl; )
			return true;
		}
	}
	if ( d.pred[0] == '?' ) {
		trace ( cout << indent() << "  dest is var" << endl << flush; )
		try {
			pred_t dval = evaluate ( d, denv );
			trace ( _indent++; )
			bool b = unify ( s, senv, dval, denv, f );
			trace ( _indent--; )
			return b;
		} catch ( ... ) {
			( *denv ) [d.pred] = evaluate ( s, senv );
			trace ( _indent--; )
			trace ( cout << indent() << " Match!(free var)" << endl; )
			return true;
		}
	}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		trace ( cout << indent() << "  Comparison:" << endl << flush; )
		for ( size_t i = 0; i < s.args.size(); i++ ) {
			trace ( _indent++; )
			if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) {
				trace ( _indent--; cout << indent() << "    " << ( string ) s.args[i] << " != " << ( string ) d.args[i] << endl << flush; )
				return false;
			}
			trace ( _indent--; cout << indent() << "    " << ( string ) s.args[i] << " == " << ( string ) d.args[i] << endl << flush; )
		}
		trace ( cout << indent() << "  Equal!" << endl << flush; )
		return true;
	}
	trace ( cout << indent() << " No match" << endl << flush; )
	return false;
}

pred_t evaluate ( pred_t t, penv_t env ) {
	trace ( cout << indent() << "Eval " << ( string ) t << " in " << ( *env ) << endl; )
	if ( t.pred[0] == '?' ) {
		trace ( cout << "(" << ( string ) t << " is a var..)" << endl; );
		auto it = env->find ( t.pred );
		if ( it != env->end() ) return evaluate ( it->second, env );
		else throw 0;
	} else if ( t.args.size() == 0 ) return t;
	vector<pred_t> n;
	for ( size_t i = 0; i < t.args.size(); ++i ) {
		try {
			n.push_back ( evaluate ( t.args[i], env ) );
		} catch ( ... ) {
			n.push_back ( { t.args[i].pred, vector<pred_t>() } );
		}
	}
	return {t.pred, n};
}

inline pred_t mk_res ( string r ) { return {r, {}}; }

void funtest() {
	evidence_t evidence, cases;
	pred_t Socrates = mk_res ( "Socrates" ), Man = mk_res ( "Man" ), Mortal = mk_res ( "Mortal" ), Morrtal = mk_res ( "Morrtal" ), Male = mk_res ( "Male" ), _x = mk_res ( "?x" ), _y = mk_res ( "?y" );
	cases["a"].push_back ( {{"a", {Socrates, Male}}, {}} );
	cases["a"].push_back ( {{"a", {_x, Mortal}}    , { {"a", {_x, Man}}, } } );
	cases["a"].push_back ( {
		{"a", {_x, Man}},
		{ {"a", {_x, Male}}, }
	} );

	bool p = prove ( {"a", {_y, Mortal}}, -1, cases, evidence );
	cout << "Prove returned " << p << endl;
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		cout << endl << "---" << endl;
	}
	cout << "QED! <- this means the proof is done, in leet mathspeak" << endl;
}

pred_t triple(const string& s, const string& p, const string& o) { return pred_t{ p, { { s, {}}, { o, {}}}}; };
pred_t triple(const jsonld::quad& q){ return triple(q.subj->value, q.pred->value, q.object->value); };

int main ( int argc, char** argv ) {
	if ( argc == 1 ) funtest();
	if ( argc != 2 && argc != 3 && argc != 6 ) {
		cout << "Usage:" << endl << "\t" << argv[0] << " [<JSON-LD kb file> [<Graph Name> [<Goal's subject> <Goal's predicate> <Goal's object>]]]" << endl;
		cout << "think about socrates, or load a file, or also print a graph from it, or try to prove a triple." << endl;
		return 1;
	}

	cout << "input:" << argv[1] << endl;
	auto kb = jsonld::load_jsonld ( argv[1], true );
	cout<<kb.tostring()<<endl;

	if ( argc == 2 ) return 0;
	auto it = kb.find ( argv[2] ) ;
	if ( it == kb.end() ) {
		cerr << "No such graph." << endl;
		return 1;
	}

	evidence_t evidence, cases;
	/*the way we store rules in jsonld is: graph1 implies graph2*/
	for ( const auto& quad : *it->second ) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		if (p == "http://www.w3.org/2000/10/swap/log#implies") 
		{
			rule_t rule;
			//go thru all quads again, look for the implicated graph (rule head in prolog terms)
			for (const auto& y : *it->second)
				if (y->graph->value == o) {
					rule.head = triple(*y);
					//now look for the subject graph
					for (const auto& x : *it->second) 
						if (x->graph->value == s) 
							rule.body.push_back( triple(*x) );
					cases[p].push_back ( rule );
				}
		}
		else
			cases[p].push_back ({ { p, { mk_res(s), mk_res(o) }}, {}} ); 
	}

	if ( argc == 3 ) return 0;
	const string s = argv[3], p = argv[4], o = argv[5];
	pred_t goal = {p, {{s, {}}, {o, {}}}};
	bool b = prove ( goal, -1, cases, evidence );
	cout << "Prove returned " << b << endl;
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		cout << endl << "---" << endl;
	}
	return 0;
}
