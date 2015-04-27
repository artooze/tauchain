// Euler proof mechanism -- Jos De Roo
//version = '$Id: Euler4.js 1398 2007-07-20 16:41:33Z josd $'
#include <vector>
#include <cstdlib>
#include <cmath>
#include <map>
#include <stdexcept>
#include <climits>
#include <deque>
#include <iostream>
#include <sstream>
using namespace std;

const size_t max_preds = 1024 * 1024;
const size_t GND = INT_MAX;

typedef map<int, uint> subst;
typedef vector<pair<uint, subst>> gnd_t;

class bidict {
	map<int, string> m1;
	map<string, int> m2;
	size_t lastk = 1;
public:
	const int set(const string& v) { int k = (int)lastk++; if (v[0] == '?') k = -k;  m1[k] = v; m2[v] = k; return k; }
	void set(const int& k, const string& v) { m1[k] = v; m2[v] = k; }
	const string operator[](const int& k) { return m1[k]; }
	const int operator[](const string& v) { return m2[v]; }
	bool has(const int& k) const { return m1.find(k) != m1.end(); }
	bool has(const string& v) const { return m2.find(v) != m2.end(); }
	string tostr() {
		stringstream s;
		for (auto x : m1) cout << x.first << " := " << x.second << endl;
		return s.str();
	}
} dict;

class predicate {
	static predicate *preds;
	static size_t npreds;
	uint loc;
public:
	int pred = 0;
	vector<uint> args;
	uint& head = * ( unsigned int* )&pred;
	vector<uint>& body = args;
	predicate ( string s) : predicate() {
		if (dict.has(s)) pred = dict[s];
		else pred = dict.set(s);
	}
	predicate ( int p, int q) : predicate(p, vector<uint>(1, q)){}
	predicate ( int p = 0, vector<uint> a = vector<uint>() ) : loc ( npreds++ ), pred ( p ), args ( a ) { }
	predicate ( int p, gnd_t g ) : predicate ( p ) {
		for ( auto x : g ) args.push_back ( x.first );
	}
	//	predicate ( uint p, vector<uint> a = vector<uint>() ) : predicate ( ( int ) p, a ) {}
	static predicate* _ps() {
		return preds;
	}
	operator uint() {
		return loc;
	}
	ostream& operator<<(ostream& o) { 
		o <<  dict[pred] << '(' << pred << ')' << " implied by either "; 
		for (auto x : args) o << dict[preds[x]] << '(' << preds[x] << ')' << " or ";
		return o << '.' << endl;
	}
	static string tostr() {
		stringstream ss;
		for (size_t n = 0; n < npreds; ++n) ss << preds[n] << endl;
		return ss.str();
	}
};
predicate *predicate::preds = new predicate[max_preds];
size_t predicate::npreds = 1;
#define P predicate::_ps()
#define PID (uint)predicate
typedef predicate rule_t;

//uint mkpred(uint p, vector<uint> a) { return mkpred(preds[p].pred, a); }

uint eval ( const uint _t, const subst s ) {
	const predicate& t = P[_t];
	if ( t.pred < 0 ) {
		auto it = s.find ( t.pred );
		return it == s.end() ? ( uint ) predicate ( ) : eval ( it->second, s );
	} else if ( !t.args.size() == 0 ) return _t;
	uint _r = PID ( t.pred );
	predicate& r = P[_r];
	uint a;
	for ( uint x : t.args ) r.args.emplace_back ( ( a = eval ( x, s ) ) ? PID ( P[x].pred ) : a );
	return _r;
}

bool unify ( const uint _s, const subst senv, const uint _d, subst& denv, const bool f, const uint __d = 0 ) {
	if ( !_s ) return true;
	if ( !_d ) {
		if ( f ) {
			if ( !__d ) throw logic_error ( "wrt unify's __d" );
			denv[P[__d].pred] = eval ( _s, senv );
		}
		return true;
	}
	const predicate &s = P[_s], &d = P[_d];
	if ( s.pred < 0 ) return unify ( eval ( _s, senv ), senv, _d, denv, f );
	if ( d.pred < 0 ) return unify ( _s, senv, eval ( _d, denv ), denv, f, _d );

	if ( s.pred != d.pred || s.args.size() != d.args.size() ) return false;

	for ( size_t i = 0; i < s.args.size(); ++i )
		if ( !unify ( s.args[i], senv, d.args[i], denv, f ) )
			return false;
	return true;
}
int builtin() {
	return -1;
}
typedef map<int, vector<uint>> evidence_t;

ostream& operator<<(ostream& o, const subst& s) { for (auto x : s) o << x.first << " := " << x.second << endl; return o; }
ostream& operator<<(ostream& o, const gnd_t& s) { for (auto x : s) o << x.first << " := " << x.second << endl; return o; }

evidence_t prove ( uint goal, int max_steps, const evidence_t& cases ) {
	static bool first = true;
	if (first) { cout<<"prove called, dictionary: "<<endl<<dict.tostr()<<endl; first = false; }
	typedef uint uint;
	cout<<"kb:"<<endl<<predicate::tostr()<<endl;

	struct proof_element {
		uint rule, src, ind;
		proof_element* parent;
		subst env;
		gnd_t ground;
		proof_element ( uint r = 0, uint s = 0, uint i = 0, proof_element* p = 0, subst e = subst(), gnd_t g = gnd_t() ) :
			rule ( r ), src ( s ), ind ( i ), parent ( p ), env ( e ), ground ( g ) {
				cout<<"new proof_element with: ";
				cout<<"rule: "<<rule<<", ";
				cout<<"src: "<<src<<", ";
				cout<<"ind: "<<ind<<", ";
				cout<<"parent: "<<parent<<", ";
				cout<<"env: "<<env<<", ";
				cout<<"gnd: "<<ground<<endl;
				}
	};
	deque<proof_element> proof_trace;

	proof_trace.emplace_back ( goal );
	evidence_t evidence;
	size_t step = 0;
	while ( proof_trace.size() ) {
		proof_element& pe = proof_trace.front();
		proof_trace.pop_front();
		gnd_t gnd = pe.ground;
		step++;
		if ( max_steps != -1 && ( int ) step >= max_steps ) return evidence_t();
		if ( pe.ind >= P[pe.rule].body.size() ) {
			if ( !pe.parent ) {
				for ( size_t i = 0; i < P[pe.rule].body.size(); ++i ) {
					uint t = eval ( P[pe.rule].body[i], pe.env );
					if ( evidence.find ( P[t].pred ) == evidence.end() ) evidence[P[t].pred] = {};
					evidence[P[t].pred].emplace_back ( PID ( t, vector<uint> {PID ( GND, pe.ground ) } ) );
				}
			} else {
				if ( P[pe.rule].body.size() ) gnd.emplace_back ( P[pe.rule], pe.env );
				proof_element r = *pe.parent;
				r.ground = gnd;
				unify ( P[pe.rule].head, pe.env, P[r.rule].body[r.ind], r.env, true );
				r.ind++;
				proof_trace.emplace_back ( r );
			}
		} else {
			uint t = P[pe.rule].body[pe.ind];
			int b = builtin ();// t, pe );
			if ( b == 1 ) {
				gnd.emplace_back ( eval ( t, pe.env ), subst() );
				proof_element r(PID( P[pe.rule].head, P[pe.rule].body ), pe.src, pe.ind, pe.parent, pe.env, gnd);
				r.ind++;
				proof_trace.push_back ( r );
				continue;
			} else if ( b == 0 ) continue;
			if ( cases.find ( P[t].pred ) != cases.end() ) {
				size_t src = 0;
				for ( size_t k = 0; k < cases.at ( P[t].pred ).size(); k++ ) {
					uint rl = cases.at ( P[t].pred ) [k];
					src++;
					gnd_t gnd = pe.ground;
					if ( !P[rl].body.size() ) gnd.emplace_back ( PID ( rl ), subst() );
					proof_element r ( rl, src, 0, &pe, subst(), gnd );
					if ( unify ( t, pe.env, P[rl].head, r.env, true ) ) {
						auto ep = pe;  // euler path
						while ( ep.parent ) {
							ep = *ep.parent;
							if ( ep.src == pe.src && unify ( P[ep.rule].head, ep.env, P[pe.rule].head, pe.env, false ) ) break;
						}
						if ( !ep.parent ) proof_trace.emplace_front ( r );

					}
				}
			}
		}
	}
	return evidence;
}


int main() {
	evidence_t evidence, cases;
	uint Socrates =PID( "Socrates" ), Man=PID ( "Man" ), Mortal=PID ( "Mortal" ), Morrtal=PID ( "Morrtal" ), Male=PID ( "Male" ), _x=PID ( "?x" ), _y=PID ( "?y" );
	rule_t r;
	r.head = PID(dict["a"], {Socrates, Male});
	cases[dict["a"]].push_back (r);
	r.head = PID(dict["a"], {_x, Mortal});
	r.body = { PID(dict["a"], {_x, Man})  };
	cases[dict["a"]].push_back (r);
	r.head = PID(dict["a"], {_x, Man});
	r.body = { PID(dict["a"], {_x, Male})  };

	evidence = prove ( PID (dict["a"], PID(_y, Mortal)), -1, cases );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
//		cout << "  " << e.first << ":" << endl;
//		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
//		cout << endl << "---" << endl;
	}
	cout << "QED!" << endl;
	cout<< evidence.size()<<endl;
	return 0;
}
