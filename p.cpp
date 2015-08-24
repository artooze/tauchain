#include <cstdlib>
#include <vector>
#include <map>
#include <iostream>
#include <set>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include "strings.h"
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;
using std::vector;
using std::map;
using std::wstringstream;
using std::endl;
auto &din = std::wcin;
auto &dout = std::wcout;
typedef std::wstring string;
struct term;
typedef vector<term*> termset;
typedef int resid;
typedef std::map<resid, term*> subs;

string lower ( const string& s_ ) { string s = s_; std::transform ( s.begin(), s.end(), s.begin(), ::towlower ); return s; }
pstring pstr ( const string& s ) { return std::make_shared<string>(s); }
pstring wstrim(string s) { trim(s); return pstr(s); }
pstring wstrim(const wchar_t* s) { return wstrim(string(s)); }
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }
resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, False, rdfnil, rdfsResource, rdfsdomain, implies;

struct term {
	term() : term(0,0,0) {}
	term(termset& kb, termset& query) : term() { addbody(query); trymatch(kb); }
	resid p;
	term *s, *o;
	term(resid _p, term* _s = 0, term* _o = 0) : p(_p), s(_s), o(_o) {}
	struct body_t {
		friend term;
		term* t;
		body_t(term* _t) : t(_t) {}
		struct match 	{ match(term* _t, const subs& _s):t(_t),s(_s){} term* t; subs s; };//* matches = 0;
		typedef vector<match*> mvec;
		mvec matches;
		mvec::iterator begin() 	{ return matches.begin(); }
		mvec::iterator end() 	{ return matches.end(); }
		mvec::iterator it;
		subs ds;
		bool operator()(const subs& s) {
			if (!state) { it = begin(); state = true; }
			else { ++it; ds.clear(); }
			while (it != end()) {
				dout << "matching " << format(t, true) << " with " << format((*it)->t, true) << "... ";
				if (it != end() && t->unify(s, (*it)->t, ds)) { dout << " passed" << endl; return state = true; }
				else { ds.clear(); ++it; dout << " failed" << endl; continue; }
			}
			return state = false;
		}
	private:
		void addmatch(term* x, const subs& s) {
			dout << "added match " << format(x) << " to " << format(t) << endl;
			matches.push_back(new match(x, s));
		}
		bool state = false;
	};
	typedef vector<body_t*> bvec;
	bvec body;
	typedef bvec::iterator bvecit;
	typedef bvec::const_iterator bveccit;
	bvecit begin()	 	{ return body.begin(); }
	bvecit end() 		{ return body.end(); }
	bveccit begin() const 	{ return body.begin(); }
	bveccit end() const 	{ return body.end(); }
//	size_t szbody() const 		{ return body.size(); }
//	const body_t& getbody(int n) const { return *body[n]; }

	term* addbody(const termset& t) { for (term* x : t) addbody(x); }
	term* addbody(term* t) {
		body.push_back(new body_t(t));
		return this;
	}

	void trymatch(termset& t) { for (auto& b : *this) for (term* x : t) trymatch(*b, x); }

	term* evaluate(const subs& ss) {
		static subs::const_iterator it;
		if (p < 0) return ((it = ss.find(p)) == ss.end()) ? 0 : it->second->p < 0 ? 0 : it->second->evaluate(ss);
		if (!s && !o) return this;
		if (!s || !o) throw 0;
		term *a = s->evaluate(ss), *b = o->evaluate(ss);
		return new term(p, a ? a : s, b ? b : o);
	}
	bool unify_ep(const subs& ssub, term* _d, const subs& dsub) {
		return (!(!p || !_d || !_d->p)) && ((p < 0) ? unifvar_ep(ssub, _d, dsub) : unif_ep(ssub, *_d, dsub, !s));
	}
	bool unify(const subs& ssub, term* _d, subs& dsub) {
		return (!(!p || !_d || !_d->p)) && ((p < 0) ? unifvar(ssub, _d, dsub) : unif(ssub, *_d, dsub, !s));
	}
private:
	static term* v; // temp var for unifiers

	void trymatch(body_t& b, term* t) {
		if (t == this) return;
		dout << "trying to match " << format(b.t) << " with " << format(t) << endl;
		static subs d;
		if (b.t->unify(subs(), t, d)) b.addmatch(t, d);
		d.clear();
	}

	bool unifvar(const subs& ssub, term* _d, subs& dsub) {
		if (!_d) return false;
		static subs::const_iterator it;
		if ((v = evaluate(ssub))) return v->unify(ssub, _d, dsub);
		if ((it = dsub.find(p)) != dsub.end() && it->second != _d && it->second->p > 0) return false;
		dsub[p] = _d;
		return true;
	}
	bool unifvar_ep(const subs& ssub, term* _d, const subs& dsub) {
		return _d && ((v = evaluate(ssub)) ? v->unify_ep(ssub, _d, dsub) : true);
	}

	bool unif(const subs& ssub, term& d, subs& dsub, bool pred) {
		if (!d.p || d.p == implies) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
			dsub.emplace(d.p, evaluate(ssub));
			return true;
		}
		return p == d.p && (pred || s->unify(ssub, d.s, dsub) && o->unify(ssub, d.o, dsub) );
	}

	bool unif_ep(const subs& ssub, term& d, const subs& dsub, bool pred) {
		if (!d.p || d.p == implies) return false;
		if (d.p < 0) return ((v = d.evaluate(dsub))) ? unify_ep(ssub, v, dsub) : true;
		return p == d.p && (pred || s->unify_ep(ssub, d.s, dsub) && o->unify_ep(ssub, d.o, dsub));
	}
};

term* term::v;

class bidict {
	std::map<resid, string> ip;
	std::map<string, resid> pi;
public:
	resid operator[] ( pstring s ) { return (*this)[*s]; }
	void init() {
		GND = set( L"GND" );
		implies = set(L"=>");
		Dot = set(L".");
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		auto it = pi.find ( v );
		if ( it != pi.end() ) return it->second;
		resid k = pi.size() + 1;
		if ( v[0] == L'?' ) k = -k;
		pi[v] = k;
		ip[k] = v;
		return k;
	}

	string operator[] ( resid k ) { return ip[k]; }
	resid operator[] ( string v ) { return set(v); }
	bool has ( resid k ) const { return ip.find ( k ) != ip.end(); }
	bool has ( string v ) const { return pi.find ( v ) != pi.end(); }
} dict;

#define EPARSE(x) throw wruntime_error(string(x) + string(s,0,48));
#define SKIPWS while (iswspace(*s)) ++s
#define RETIF(x) if (*s == x) return 0
#define RETIFN(x) if (*s != x) return 0
#define PROCEED1 while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')')
#define PROCEED PROCEED1 t[pos++] = *s++; t[pos] = 0; pos = 0
#define TILL(x) do { t[pos++] = *s++; } while (x)
class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
public:
	nqparser() : t(new wchar_t[4096*4096]) {}
	~nqparser() { delete[] t; }

	bool readcurly(termset& ts) {
		while (iswspace(*s)) ++s;
		if (*s != L'{') return false;
		ts.clear();
		do { ++s; } while (iswspace(*s));
		if (*s == L'}') ++s;
		ts = (*this)(s);
		return true;
	}

	term* readlist() {
		if (*s != L'(') return (term*)0;
		++s; 
		term *head = new term(Dot), *pn = head;
		while (*s != L')') {
			SKIPWS;
			if (*s == L')') break;
			if (!(pn->s = readany(true)))
				EPARSE(L"couldn't read next list item: ");
			SKIPWS;
			pn = pn->o = new term(Dot);
			if (*s == L'.') while (iswspace(*s++));
			if (*s == L'}') EPARSE(L"expected { inside list: ");
		};
		do { ++s; } while (iswspace(*s));
		return head;
	}

	term* readiri() {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0; ++s;
			return new term(dict[t]);
		}
		if (*s == L'=' && *(s+1) == L'>') {
			++++s;
			return new term(implies);
		}
		PROCEED;
		pstring iri = wstrim(t);
		if (lower(*iri) == L"true") return new term(dict[L"true"]);
		if (lower(*iri) == L"false") return new term(dict[L"false"]);
		return new term(dict[iri]);
	}

	term* readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return new term(dict[wstrim(t)]); } 
	term* readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return new term(dict[wstrim(t)]); }

	term* readlit() {
		SKIPWS; RETIFN(L'\"');
		++s;
		TILL(!(*(s-1) != L'\\' && *s == L'\"'));
		string dt, lang;
		++s;
		PROCEED1 {
			if (*s == L'^' && *++s == L'^') {
				if (*++s == L'<') {
					++s; while (*s != L'>') dt += *s++;
					++s; break;
				}
			} else if (*s == L'@') { while (!iswspace(*s)) lang += *s++; break; }
			else EPARSE(L"expected langtag or iri:");
		}
		t[pos] = 0; pos = 0;
		string t1 = t;
		boost::replace_all(t1, L"\\\\", L"\\");
		return new term(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
	}

	term* readany(bool lit = true) {
		term* pn;
		return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
	}

	termset operator()(const wchar_t* _s) {
		std::list<std::pair<term*, termset>> preds;
		s = _s;
//		string graph;
		term *subject, *pn;
		termset subjs, ts, heads, objs;
		pos = 0;
		auto pos1 = preds.rbegin();

		while(*s) {
			if (readcurly(subjs)) subject = 0;
			else if (!(subject = readany(false))) EPARSE(L"expected iri or bnode subject:"); // read subject
			do { // read predicates
				while (iswspace(*s) || *s == L';') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readiri())) { // read predicate
					preds.emplace_back(pn, termset());
					pos1 = preds.rbegin();
				} else EPARSE(L"expected iri predicate:");
				do { // read objects
					while (iswspace(*s) || *s == L',') ++s;
					if (*s == L'.' || *s == L'}') break;
					if (readcurly(objs)) pos1->second = objs;
					else if ((pn = readany(true))) pos1->second.push_back(pn); // read object
					else EPARSE(L"expected iri or bnode or literal object:");
					SKIPWS;
				} while (*s == L','); // end predicates
				SKIPWS;
			} while (*s == L';'); // end objects
//			if (*s != L'.' && *s != L'}' && *s) { // read graph
//				if (!(pn = readbnode()) && !(pn = readiri()))
//					EPARSE(L"expected iri or bnode graph:");
//				graph = dict[pn->p];
//			} else graph = ctx;
			SKIPWS; while (*s == '.') ++s; SKIPWS;
			if (*s == L')') EPARSE(L"expected ) outside list: ");
			if (subject)
				for (auto x : preds)
					for (term* o : x.second)
						heads.emplace_back(new term(x.first->p, subject, o));
			else for (auto o : objs) heads.emplace_back(o->addbody(subjs));
			if (*s == L'}') { ++s; return heads; }
			preds.clear();
		}
		return heads;
	}
};

termset readqdb ( std::wistream& is) {
	string s, c;
	nqparser p;
	std::wstringstream ss;
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && *wstrim(s.c_str() + 3) == L".") break;
		ss << ' ' << s << ' ';
	}
	return p((wchar_t*)ss.str().c_str());
}

string formatlist(const term* t, bool in = false) {
	if (!t || !t->s || !t->o) return L"";
	if (t->p != Dot)
		throw 0;
	std::wstringstream ss;
	if (!in) ss << L'(';
	ss << formatlist(t->o, true) << L' ' << format(t->s) << L' ';
	if (!in) ss << L')';
	return ss.str();
}

string format(const term* t, bool body) {
	if (!t) return L"";
	std::wstringstream ss;
	if (body && t->p == implies) {
		ss << L'{';
		for (auto x : *t) ss << format(x->t) << L';';
		ss << L'}';
		ss << format(t, false);
	}
	else if (!t->p) return L"";
	else if (t->p != Dot) {
		if (t->s) ss << format(t->s) << L' ' << dict[t->p] << L' ' << format(t->o) << L'.';
		else ss << dict[t->p];
	}
	else return formatlist(t);
	return ss.str();
}

#define IDENT for (int n = 0; n < dep; ++n) ss << L'\t'

string format(const termset& t, int dep) {
	std::wstringstream ss;
	for (term* x : t) {
		if (!x) continue;
		IDENT;
		ss << format(x, true) << (x->body.size() ? L" implied by: " : L" a fact.") << endl;
		for (term::body_t* y : *x) {
			IDENT;
			ss << L"\t" << format(y->t, true) << L" matches to heads:" << endl;
			for (term::body_t::match* z : *y) {
				IDENT;
				ss << L"\t\t" << format(z->t, true) << endl;
			}
		}
	}
	return ss.str();
}

typedef std::list<std::pair<term*, subs>> ground;
typedef std::map<resid, std::list<std::pair<term*, ground>>> evidence;

struct proof {
	term* rule = 0;
	term::bvecit b;
	shared_ptr<proof> prev = 0, creator = 0, next = 0;
	subs s;
	ground g() const {
		if (!creator) return ground();
		ground r = creator->g();
		termset empty;
		if (btterm) r.emplace_back(btterm, subs());
		else if (creator->b != creator->rule->end()) {
			if (!rule->body.size()) r.emplace_back(rule, subs());
		} else if (creator->rule->body.size()) r.emplace_back(creator->rule, creator->s);
		return r;	
	}
	term* btterm = 0;
	proof(shared_ptr<proof> c, term* r, term::bvecit* l = 0, shared_ptr<proof> p = 0, const subs&  _s = subs())
		: rule(r), b(l ? *l : rule->begin()), prev(p), creator(c), s(_s) { }
	proof(shared_ptr<proof> c, proof& p) 
		: proof(c, p.rule, &p.b, p.prev) { }
};

void step(shared_ptr<proof> _p) {
	size_t steps = 0;
	shared_ptr<proof> lastp = _p;
	evidence e;
	while (_p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << endl);
		auto ep = _p;
		proof& p = *_p;
		term* t = p.rule;
		if (t) {
			while (t && (ep = ep->prev))
				if (ep->rule == p.rule && ep->rule->unify_ep(ep->s, t, p.s)) {
					_p = _p->next;
					t = 0;
				}
			if (!t) continue;
		}
		if (p.b != p.rule->end()) {
			term::body_t& pb = **p.b;
			while (pb(p.s)) {
				auto r = make_shared<proof>(_p, (*pb.it)->t, nullptr, _p, pb.ds);
				if (lastp) lastp->next = r;
				lastp = r;
			}
		}
		else if (!p.prev) {
			term* t;
			for (auto r : *p.rule) {
			if (!(t = (r->t->evaluate(p.s)))) continue;
				e[t->p].emplace_back(t, p.g());
				dout << "proved: " << format(t) << endl;
			}
		}
		else {
			proof& ppr = *p.prev;
			shared_ptr<proof> r = make_shared<proof>(_p, ppr);
			auto rl = p.rule;
			r->s = ppr.s;
			rl->unify(p.s, (*r->b)->t, r->s);
			++r->b;
			step(r);
		}
		_p = p.next;
	}
};
int main() {
	dict.init();
	termset kb = readqdb(din);
	dout << "kb:" << endl << format(kb) << endl;
	termset query = readqdb(din);

	term* q = new term(kb, query);

	kb.push_back(q);
	dout << "kb:" << endl << format(kb) << endl;

	step(make_shared<proof>(nullptr, q));

	return 0;
}
