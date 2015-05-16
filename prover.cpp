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
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;
bidict& gdict = dict;

int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf;

namespace prover {

term* terms = 0;
termset* termsets = 0;
rule* rules = 0;
proof* proofs = 0;
uint nterms, ntermsets, nrules, nproofs;

//void printps(term* p, subst* s); // print a term with a subst
void printterm_substs(const term& p, const subst& s);

int _indent = 0;
std::list<string> proc;
string indent() {
	if (!_indent) return string();
	std::wstringstream ss;
//	ss << _indent;
	for (auto it = proc.rbegin(); it != proc.rend(); ++it) {
		string str = L"(";
		str += *it;
		str += L") ";
		ss << std::setw(8) << str;
	}
	ss  << '\t' << std::setw(_indent * 2);
//	for (int n = 0; n < _indent; ++n) ss << "     ";
	return ss.str();
}

struct _setproc {
	string prev;
	_setproc(const string& p) {
		prover::proc.push_front(p);
	}
	~_setproc() {
		prover::proc.pop_front();
	}
};

#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif

void initmem() {
	static bool once = false;
	if (!once) {
		terms = new term[max_terms];
		termsets = new termset[max_termsets];
		rules = new rule[max_rules];
		proofs = new proof[max_proofs];
		once = true;
	}
//	memset(terms, 0, sizeof(term) * max_terms);
//	memset(termsets, 0, sizeof(termset) * max_termsets);
//	memset(rules, 0, sizeof(rule) * max_rules);
//	memset(proofs, 0, sizeof(proof) * max_proofs);
	nterms = ntermsets = nrules = nproofs = 0;
}

term* evaluate(term* p, subst& s) {
	setproc(L"evaluate");
	term* r;
	if (!p) return 0;
	if (p->p < 0) {
		auto it = s.find(p->p);
		r = it == s.end() ? 0 : evaluate(s[p->p], s);
	} else if (!p->s && !p->o)
		r = p;
	else {
		term *a = evaluate(p->s, s);
		r = &terms[nterms++];
		r->p = p->p;
		if (a)
			r->s = a;
		else {
			r->s = &terms[nterms++];
			r->s->p = p->s->p;
			r->s->s = r->s->o = 0;
		}
		a = evaluate(p->o, s);
		if (a)
			r->o = a;
		else {
			r->o = &terms[nterms++];
			r->o->p = p->o->p;
			r->o->s = r->o->o = 0;
		}
	}
	TRACE(printterm_substs(*p, s); dout << " = "; if (!r) dout << "(null)"; else printterm(*r); dout << std::endl);
	return r;
}
/*
void printps(term& p, subst& s) {
	printterm(p);
	dout<<" [ ";
	prints(s);
	dout<<" ] ";
}
*/
bool unify(term* s, subst& ssub, term* d, subst& dsub, bool f) {
	setproc(L"unify");
	term* v;
	bool r, ns = false;
	if (!s && !d) {
		TRACE(dout << "unification of two nulls returned true" << std::endl);
		return true;
	}
	else if (s->p < 0) 
		r = (v = evaluate(s, ssub)) ? unify(v, ssub, d, dsub, f) : true;
	else if (d->p < 0) {
		if ((v = evaluate(d, dsub)))
			r = unify(s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d->p] = evaluate(s, ssub);
				ns = true;
			}
			r = true;
		}
	} else if (!(s->p == d->p && !s->s == !d->s && !s->o == !d->o))
		r = false;
	else r = unify(s->s, ssub, d->s, dsub, f) && unify(s->o, ssub, d->o, dsub, f);
	TRACE(dout << "Trying to unify ";
		printterm_substs(*s, ssub);
		dout<<" with ";
		printterm_substs(*d, dsub);
		dout<<"... ";
		if (r) {
			dout << "passed";
			if (ns) {
				dout << " with new substitution: " << dict[d->p] << " / ";
				printterm(*dsub[d->p]);
			}
		} else dout << "failed";
		dout << std::endl);
	return r;
}

bool euler_path(proof* p) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (ep->rul == p->rul &&
			unify(ep->rul->p, *ep->s, p->rul->p, *p->s, false))
			break;
	if (ep) {
		TRACE(dout<<"Euler path detected\n");
		return true;
	}
	return false;
}

int builtin(term& t, proof& p, session* ss) {
	setproc(L"builtin");
	if (t.p == GND) return 1;
	term* t0 = evaluate(t.s, *p.s);
	term* t1 = evaluate(t.o, *p.s);
	if (t.p == logequalTo)
		return t0 && t1 && t0->p == t1->p ? 1 : 0;
	if (t.p == lognotEqualTo)
		return t0 && t1 && t0->p != t1->p ? 1 : 0;
	if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		return unify(t0->s, *p.s, t.o, *p.s, true) ? 1 : 0;
	if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		return unify(t0->o, *p.s, t.o, *p.s, true) ? 1 : 0;
	if (t.p == A || t.p == rdfsType) {
//		if (t1 && ((t1->p == rdfList && t0 && t0->p == Dot) || t1->p == rdfsResource))
//			return 1;
//		if (!t.o) 
//			return -1;
		if (!t0) t0 = t.s;
		if (!t1) t1 = t.o;
		session s2;
		s2.rkb = ss->rkb;
		term& vA = terms[nterms++];
		vA.p = dict.set(L"?A");
		vA.s = vA.o = 0;
		term& q1 = terms[nterms++];
		q1.p = rdfssubClassOf;
		q1.s = &vA;
		q1.o = t1;
		s2.goal.insert(&q1);
		prove(&s2);
		auto it = s2.e.find(rdfssubClassOf);
		if (it == s2.e.end()) return -1;
		bool res = true;
		for (auto tg : it->second) {
			int subclasser = tg.first->s->p;
			TRACE(dout << "subclasser: " << dict[subclasser] << std::endl);
			session s3;
			s3.rkb = ss->rkb;
			term& sc = terms[nterms++];
			sc.p = subclasser;
			sc.o = sc.s = 0;
			term& q2 = terms[nterms++];
			q2.p = A;
			q2.s = t0;
			q2.o = &sc;
			s3.goal.insert(&q2);
			prove(&s3);
//			std::pair<term*, ground> e = s3.e[A].front();
			for (auto e : s2.e[rdfssubClassOf])
				res &= unify(e.first, /**p.s*/*e.second.front().second, &t, *p.s, true);
			for (auto e : s3.e[A]) 
				res &= unify(e.first, /**p.s*/*e.second.front().second, &t, *p.s, true);
		}
		return res ? 1 : -1;
	}
	#ifdef marpa
	if (t.p == marpa_parsed) {
	#endif

	return -1;
}

void prove(session* ss) {
	setproc(L"prove");
	termset& goal = ss->goal;
	ruleset& cases = ss->rkb;
	queue qu;
	rule* rg = &rules[nrules++];
	proof* p = &proofs[nproofs++];
	rg->p = 0;
	rg->body = goal;
	p->rul = rg;
	p->last = rg->body.begin();
	p->prev = 0;
	qu.push_back(p);
	TRACE(dout << KRED << "Facts:\n"; printrs(cases));
	TRACE(dout << KGRN << "Query:\n"; printl(goal); dout << KNRM);
	_indent++;
	do {
		p = qu.back();
		qu.pop_back();
		TRACE(dout<<"popped frame:\n";printp(p));
		if (p->last != p->rul->body.end()) {
			term* t = *p->last;
			TRACE(dout<<"Tracking back from ";
				printterm(*t);
				dout << std::endl);
			int b = builtin(*t, *p, ss);
			if (b == 1) {
				proof* r = &proofs[nproofs++];
				rule* rl = &rules[nrules++];
				*r = *p;
				rl->p = evaluate(t, *p->s);
				TRACE(dout << "builtin added rule: ";
					printr(*rl);
					dout << " by evaluating ";
					printterm_substs(*t, *p->s);
					dout << std::endl);
				r->g = p->g;
				r->g.emplace_back(rl, make_shared<subst>());
				++r->last;
				qu.push_back(r);
//				TRACE(dout<<"pushing frame:\n";printp(r));
		            	continue;
		        }
		    	else if (!b) 
				continue;

			auto it = cases.find(t->p);
			if (it == cases.end())
				continue;
			for (auto x : cases) {
//			if (x.first >= 0 && x.first != t->p)
//				continue;
			std::list<rule*>& rs = x.second;
//			list<rule*>& rs = it->second;
			for (rule* rl : rs) {
				subst s;
				if (unify(t, *p->s, rl->p, s, true)) {
					if (euler_path(p))
						continue;
					proof* r = &proofs[nproofs++];
					r->rul = rl;
					r->last = r->rul->body.begin();
					r->prev = p;
					r->s = make_shared<subst>(s);
					r->g = p->g;
					if (rl->body.empty())
						r->g.emplace_back(rl, make_shared<subst>());
					qu.push_front(r);
//					TRACE(dout<<"pushing frame:\n";printp(r));
				} else {
				//	TRACE(dout<<"\tunification failed\n");
				}
			}
		}}
		else if (!p->prev) {
			for (auto r = p->rul->body.begin(); r != p->rul->body.end(); ++r) {
				term* t = evaluate(*r, *p->s);
				ss->e[t->p].emplace_back(t, p->g);
			}
			//TRACE(dout<<"no prev frame. queue:\n");
			//TRACE(printq(qu));
		} else {
			ground g = p->g;
			proof* r = &proofs[nproofs++];
			if (!p->rul->body.empty())
				g.emplace_back(p->rul, p->s);
			*r = *p->prev;
			r->g = g;
			r->s = make_shared<subst>(*p->prev->s);
			unify(p->rul->p, *p->s, *r->last, *r->s, true);
			++r->last;
			qu.push_back(r);
//			TRACE(dout<<"pushing frame:\n";printp(r));
//			TRACE(dout<<L"finished a frame. queue size:" << qu.size() << std::endl);
//			TRACE(printq(qu));
			continue;
		}
	} while (!qu.empty());
	_indent--;
//	dout << indent()/* << "=========================="*/ << std::endl;
	TRACE(dout << KWHT << "Evidence:\n"; printe(ss->e); dout << KNRM);
//	dout << indent()/* << "=========================="*/ << std::endl;
}

bool equals(term* x, term* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

string format(const term& p) {
	std::wstringstream ss;
	if (p.s)
		ss << format (*p.s);
	ss << L' ' << dstr(p.p) << L' ';
	if (p.o)
		ss << format (*p.o);
	return ss.str();
}

void printterm(const term& p) {
	if (p.s) {
		printterm(*p.s);
		dout << L' ';
	}
	dout << dstr(p.p);
	if (p.o) {
		dout << L' ';
		printterm(*p.o);
	}
}

void printp(proof* p) {
	if (!p)
		return;
	dout << KCYN;
	dout << indent() << L"rule:\t";
	printr(*p->rul);
	dout<<std::endl<<indent();
//	dout << L"\n\tindex:\t" << std::distance(p->rul->body.begin(), p->last) << std::end;
	if (p->prev)
		dout << L"prev:\t" << p->prev <<std::endl<<indent()<< L"subst:\t";
	else
		dout << L"prev:\t(null)"<<std::endl<<indent()<<"subst:\t";
	if (p->s) prints(*p->s);
	dout <<std::endl<<indent()<< L"ground:" << std::endl;
	++_indent;
	printg(p->g);
	--_indent;
	dout << L"\n";
	dout << KNRM;
}

void printrs(const ruleset& rs) {
	for (auto x : rs)
		for (auto y : x.second) {
			dout << indent();
			printr(*y);
			dout << std::endl;
		}
}

void printq(const queue& q) {
	for (auto x : q) {
		printp(x);
		dout << std::endl;
	}
}

void prints(const subst& s) {
//	dout << '[';
	for (auto x : s) {
		dout << dstr(x.first) << L" / ";
		printterm(*x.second);
		dout << ' ';// << std::endl;
	}
//	dout << "] ";
}

string format(const termset& l) {
	std::wstringstream ss;
	auto x = l.begin();
	while (x != l.end()) {
		ss << format (**x);
		if (++x != l.end())
			ss << L',';
	}
	return ss.str();
}

void printl(const termset& l) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm(**x);
		if (++x != l.end())
			dout << L',';
	}
}

void printr(const rule& r) {
	if(!r.body.empty())
	{
		printl(r.body);
		dout << L" => ";
	}
	if (r.p)
		printterm(*r.p);
	if(r.body.empty())
		dout << L".";
}

void printterm_substs(const term& p, const subst& s) {
	if (p.s) {
		printterm_substs(*p.s, s);
		dout << L' ';
	}
	dout << dstr(p.p);
	if(s.find(p.p) != s.end()) {
		dout << L" (";
		printterm(*s.at(p.p));
		dout << L" )";
	}
	if (p.o) {
		dout << L' ';
		printterm_substs(*p.o, s);
	}
}

void printl_substs(const termset& l, const subst& s) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm_substs(**x, s);
		if (++x != l.end())
			dout << L',';
	}
}

void printr_substs(const rule& r, const subst& s) {
	printl_substs(r.body, s);
	dout << L" => ";
	printterm_substs(*r.p, s);
}

string format(const rule& r) {
	std::wstringstream ss;
	if (!r.body.empty())
		ss << format(r.body) << L" => ";
	ss << format(*r.p);
	return ss.str();
}

void printg(const ground& g) {
	for (auto x : g) {
		dout << indent();
		printr_substs(*x.first, *x.second);
		if (!x.second || x.second->empty()) 
			dout << std::endl;
	}
//	dout<<'.';
}

void printe(const evidence& e) {
	for (auto y : e)
		for (auto x : y.second) {
			dout << indent();
			printterm(*x.first);
			dout << L":" << std::endl;
			++_indent;
			printg(x.second);
			--_indent;
			dout << std::endl;
#ifdef IRC
			sleep(1);
#endif
		}
}
}

prover::term* mkterm(const wchar_t* p, const wchar_t* s = 0, const wchar_t* o = 0) {
	prover::term* t = &prover::terms[prover::nterms++];
	t->p = dict.set(string(p));
	if (s) t->s = mkterm(s, 0, 0);
	if (o) t->o = mkterm(o, 0, 0);
	return t;
}

prover::term* mkterm(string s, string p, string o) {
	return mkterm(p.c_str(), s.c_str(), o.c_str());
}

prover::term* quad2term(const quad& p) {
	return mkterm(p.pred->value.c_str(), p.subj->value.c_str(), p.object->value.c_str());
}

int szqdb(const qdb& x) {
	int r = 0;
	for (auto y : x)
		r += y.second->size();
	return r;
}

void reasoner::addrules(string s, string p, string o, prover::session& ss, const qdb& kb) {
	if (p[0] == L'?')
		throw 0;
	prover::rule* r = &prover::rules[prover::nrules++];
	r->p = mkterm(s, p, o );
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		ss.rkb[r->p->p].push_back(r);
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		r = &prover::rules[prover::nrules++];
		prover::term* tt = quad2term( *y );
		r->p = tt;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				r->body.insert( quad2term( *z ) );
		ss.rkb[r->p->p].push_back(r);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

bool reasoner::prove ( qdb kb, qlist query ) {
	prover::session ss;
	//std::set<string> predicates;
	for ( jsonld::pquad quad : *kb.at(L"@default")) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
			addrules(s, p, o, ss, kb);
	}
	for ( auto q : query )
		ss.goal.insert( quad2term( *q ) );
	prover::prove(&ss);
	return ss.e.size();
}

bool reasoner::test_reasoner() {
	dout << L"to perform the socrates test, put the following three lines in a file say f, and run \"tau < f\":" << std::endl
		<<L"socrates a man.  ?x a man _:b0.  ?x a mortal _:b1.  _:b0 => _:b1." << std::endl<<L"fin." << std::endl<<L"?p a mortal." << std::endl;
	return 1;
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}
