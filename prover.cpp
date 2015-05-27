//#define READLINE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <iostream>
#include <ostream>
#include "misc.h"
#include <utility>
#include "object.h"
#include "prover.h"
#include <iterator>
#include <forward_list>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf;
int _indent = 0;

std::list<string> proc;
string indent() {
	if (!_indent) return string();
	std::wstringstream ss;
	for (auto it = proc.rbegin(); it != proc.rend(); ++it) {
		string str = L"(";
		str += *it;
		str += L") ";
		ss << std::setw(8) << str;
	}
	ss << "    " << std::setw(_indent * 2);
	return ss.str();
}

struct _setproc {
	string prev;
	_setproc(const string& p) {
		proc.push_front(p);
	}
	~_setproc() {
		proc.pop_front();
	}
};

#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif

bool prover::hasvar(termid id) {
	const term& p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

prover::termid prover::evaluate(termid id, const subst& s) {
	setproc(L"evaluate");
	termid r;
	const term& p = get(id);
	if (p.p < 0) {
		auto it = s.find(p.p);
		r = it == s.end() ? 0 : evaluate(it->second, s);
	} else if (!p.s && !p.o)
		r = id;
	else {
		termid a = evaluate(p.s, s);
		termid b = evaluate(p.o, s);
		r = make(p.p, a ? a : make(get(p.s).p), b ? b : make(get(p.o).p));
	}
	TRACE(printterm_substs(id, s); dout << " = "; if (!r) dout << "(null)"; dout << format(r); dout << std::endl);
	return r;
}

bool prover::unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f) {
	setproc(L"unify");
	termid v;
	const term& s = get(_s);
	const term& d = get(_d);
	bool r, ns = false;
	if (s.p < 0) 
		r = (v = evaluate(_s, ssub)) ? unify(v, ssub, _d, dsub, f) : true;
	else if (d.p < 0) {
		if ((v = evaluate(_d, dsub)))
			r = unify(_s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(_s, ssub);
				ns = true;
			}
			r = true;
		}
	} else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o))
		r = false;
	else
		r = !s.s || (unify(s.s, ssub, d.s, dsub, f) && unify(s.o, ssub, d.o, dsub, f));
	TRACE(dout << "Trying to unify ";
		printterm_substs(_s, ssub);
		dout<<" with ";
		printterm_substs(_d, dsub);
		dout<<"... ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << std::endl);
	return r;
}

bool prover::euler_path(proof* p, termid t, const ruleset& kb, bool update) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (!kb.head()[ep->rul] == !t &&
			unify(kb.head()[ep->rul], ep->s, t, p->s, update))
			break;
	if (ep) { TRACE(dout<<"Euler path detected\n"); }
	return ep;
}

int prover::builtin(termid id) {
	setproc(L"builtin");
	const term& t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, p->s) : 0;
	const term* t0 = i0 ? &get(evaluate(t.s, p->s)) : 0;
	const term* t1 = i1 ? &get(evaluate(t.o, p->s)) : 0;
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) {
		if (!t0) t0 = &get(t.s);
		if (!t1) t1 = &get(t.o);
		static termid vs = make(L"?S");
		static termid va = make(L"?A");
		static termid vb = make(L"?B");
		static termid h = make ( A,              vs, vb );
		termset ts;
		ts.push_back ( make ( rdfssubClassOf, va, vb ) );
		ts.push_back ( make ( A,              vs, va ) );
		proof* f = new proof;
		f->rul = kb.add(h, ts);
		f->prev = p;
		f->last = 0;
		f->g = p->g;
		if (euler_path(f, h, kb))
			return -1;
		q.push_back(f);
		TRACE(dout<<"builtin created frame:"<<std::endl;printp(f, kb));
		r = -1;
	}
	if (r == 1) {
		proof* r = new proof;
		*r = *p;
		uint rl = kb.add(evaluate(id, p->s), termset());
		TRACE(dout << "builtin added rule: "; printr(rl, kb);
			dout << " by evaluating "; printterm_substs(id, p->s); dout << std::endl);
		r->g = p->g;
		r->g.emplace_back(rl, subst());
		++r->last;
		q.push_back(r);
	}

	return r;
}

bool prover::maybe_unify(termid _s, termid _d) {
	if (!_s != !_d) return false;
	if (!_s) return true;
	const term& s = get(_s);
	const term& d = get(_d);
	if (s.p < 0 || d.p < 0) return true;
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	return !s.s || (maybe_unify(s.s, d.s) && maybe_unify(s.o, d.o));
}

std::set<uint> prover::match(termid e, const termid* t, uint sz) {
	std::set<uint> m;
	for (uint n = 0; n < sz; ++n)
		if (t[n] && maybe_unify(e, t[n]))
			m.insert(n);
	return m;
}

void prover::search() {
	++_indent;
	setproc(L"prove");
	p = new proof;
	p->rul = kb.add(0, goal);
	p->last = 0;
	p->prev = 0;
	q.push_back(p);
	TRACE(dout << KRED << "Facts:\n" << format(kb) << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	++_indent;
	do {
		p = q.back();
		q.pop_back();
		TRACE(dout<<"popped frame:\n";printp(p, kb));
		if (p->last != kb.body()[p->rul].size() && !euler_path(p, kb.head()[p->rul], kb)) {
			termid t = kb.body()[p->rul][p->last];
			if (!t) throw 0;
			TRACE(dout<<"Tracking back from " << format(t) << std::endl);
			if (builtin(t) != -1) continue;

			for (auto rl : match(evaluate(t, p->s), &kb.head()[0], kb.size())) {
				subst s;
				termid h = kb.head()[rl];
				if (!unify(t, p->s, h, s, true))
					continue;
				proof* r = new proof;
				r->rul = rl;
				r->last = 0;
				r->prev = p;
				r->g = p->g;
				r->s = s;
				if (kb.body()[rl].empty())
					r->g.emplace_back(rl, subst());
				q.push_front(r);
			}
		}
		else if (!p->prev) {
			for (auto r = kb.body()[p->rul].begin(); r != kb.body()[p->rul].end(); ++r) {
				termid t = evaluate(*r, p->s);
				if (!t || hasvar(t)) continue;
				TRACE(dout << "pushing evidence: " << format(t) << std::endl);
				e[get(t).p].emplace_back(t, p->g);
			}
		} else {
			ground g = p->g;
			proof* r = new proof;
			if (!kb.body()[p->rul].empty())
				g.emplace_back(p->rul, p->s);
			*r = *p->prev;
			r->g = g;
			r->s = p->prev->s;
			unify(kb.head()[p->rul], p->s, kb.body()[r->rul][r->last], r->s, true);
			++r->last;
			q.push_back(r);
		}
//		delete &p;
	} while (!q.empty());
	--_indent;
	--_indent;
	TRACE(dout << KWHT << "Evidence:" << std::endl; 
		printe(e, kb); dout << KNRM);
}


string prover::format(termid id) {
	const term& p = get(id);
	std::wstringstream ss;
	if (p.s)
		ss << format (p.s);
	ss << L' ' << dstr(p.p) << L' ';
	if (p.o)
		ss << format (p.o);
	return ss.str();
}

void prover::printp(proof* p, const ruleset& kb) {
	if (!p) return;
	dout << KCYN;
	dout << indent() << L"rule:   ";
	printr(p->rul, kb);
	dout<<std::endl<<indent();
	if (p->prev)
		dout << L"prev:   " << p->prev <<std::endl<<indent()<< L"subst:  ";
	else
		dout << L"prev:   (null)"<<std::endl<<indent()<<"subst:  ";
	prints(p->s);
	dout <<std::endl<<indent()<< L"ground: " << std::endl;
	++_indent;
	printg(p->g, kb);
	--_indent;
	dout << L"\n";
	dout << KNRM;
}

void prover::printq(const queue& q, const ruleset& kb) {
	for (auto x : q) {
		printp(x, kb);
		dout << std::endl;
	}
}

void prover::prints(const subst& s) {
	for (auto x : s)
		dout << dstr(x.first) << L" / " << format(x.second) << ' ';
}

string prover::format(const termset& l) {
	std::wstringstream ss;
	auto x = l.begin();
	while (x != l.end()) {
		ss << format (*x);
		if (++x != l.end())
			ss << L',';
	}
	return ss.str();
}

void prover::printterm_substs(termid id, const subst& s) {
	const term& p = get(id);
	if (p.s) {
		printterm_substs(p.s, s);
		dout << L' ';
	}
	dout << dstr(p.p);
	if(s.find(p.p) != s.end()) {
		dout << L" (" << format(s.at(p.p)) << L" )";
	}
	if (p.o) {
		dout << L' ';
		printterm_substs(p.o, s);
	}
}

void prover::printl_substs(const termset& l, const subst& s) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm_substs(*x, s);
		if (++x != l.end())
			dout << L',';
	}
}

void prover::printr_substs(int r, const subst& s, const ruleset& kb) {
	printl_substs(kb.body()[r], s);
	dout << L" => ";
	printterm_substs(kb.head()[r], s);
}

string prover::format(int r, const ruleset& kb) {
	std::wstringstream ss;
	if (!kb.body()[r].empty()) ss << format(kb.body()[r]) << L" => ";
	if (kb.head()[r]) 	ss << format(kb.head()[r]);
	if (kb.body()[r].empty()) 	ss << L".";
	return ss.str();
}

string prover::format(const ruleset& rs) {
	std::wstringstream ss;
	uint n = 0;
	auto ith = rs.head().begin();
	auto itb = rs.body().begin();
	while (ith != rs.head().end()) {
		if (!itb->empty()) 	ss << format(*itb) << L" => ";
		if (*ith) 		ss << format(*ith);
		if (itb->empty()) 	ss << L".";
		++n;
		++ith;
		++itb;
	}
	return ss.str();
}

void prover::printr(int r, const ruleset& kb) {
	dout << format(r, kb);
}

void prover::printg(const ground& g, const ruleset& kb) {
	for (auto x : g) {
		dout << indent();
		printr_substs(x.first, x.second, kb);
		dout << std::endl;
	}
}

void prover::printe(const evidence& e, const ruleset& kb) {
	for (auto y : e)
		for (auto x : y.second) {
			dout << indent() << format(x.first) << L":" << std::endl;
			++_indent;
			printg(x.second, kb);
			--_indent;
			dout << std::endl;
		}
}

prover::termid prover::mkterm(const wchar_t* p, const wchar_t* s, const wchar_t* o, const quad& q) {
	termid ps = s ? prover::make(dict.set(s), 0, 0/*, q.subj*/) : 0;
	termid po = o ? prover::make(dict.set(o), 0, 0/*, q.object*/) : 0;
	return prover::make(dict.set(string(p)), ps, po/*, q.pred*/);
}

prover::termid prover::mkterm(string s, string p, string o, const quad& q) {
	return mkterm(p.c_str(), s.c_str(), o.c_str(), q);
}

prover::termid prover::quad2term(const quad& p) {
	return mkterm(p.subj->value, p.pred->value, p.object->value, p);
}

void prover::addrules(pquad q, const qdb& kb) {
	const string &s = q->subj->value, &p = q->pred->value, &o = q->object->value;
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		this->kb.add(quad2term(*q), termset());
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		termset ts;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				ts.push_back( quad2term( *z ) );
		this->kb.add(quad2term(*y), ts);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

prover::prover ( qdb kb, qlist query ) {
	for ( jsonld::pquad quad : *kb.at(L"@default")) 
		addrules(quad, kb);
	for ( auto q : query )
		goal.push_back( quad2term( *q ) );
	search();
}

uint prover::ruleset::add(termid t, const termset& ts) {
	_head.push_back(t);
	_body.push_back(ts);
	return _head.size()-1;
}

prover::term::term(int _p, termid _s, termid _o, int props) : p(_p), s(_s), o(_o), properties(props) {}

const prover::term& prover::get(termid id) { 
	return terms[id - 1]; 
}

prover::termid prover::make(string p, termid s, termid o, int props) { 
	return make(dict.set(p), s, o, props); 
}

prover::termid prover::make(int p, termid s, termid o, int props) {
	if (!p) throw 0;
	terms.emplace_back(p,s,o,props);
	return terms.size();
}
