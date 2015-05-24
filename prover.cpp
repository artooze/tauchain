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

int _indent = 0;
namespace prover {

rule* rules = 0;
proof* proofs = 0;
uint nrules, nproofs;

void printterm_substs(const term& p, const subst& s);

std::set<const term*> term::terms;
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
		rules = new rule[max_rules];
		proofs = new proof[max_proofs];
		once = true;
	}
	nrules = nproofs = 0;
}

const term* evaluate(const term& p, const subst& s) {
	setproc(L"evaluate");
	if (p.p < 0) {
		auto it = s.find(p.p);
		return it == s.end() ? 0 : evaluate(*it->second, s);
	} else if (!p.s && !p.o)
		return &p;
	else {
		const term* a = evaluate(*p.s, s);
		const term* b = evaluate(*p.o, s);
		const term* r = term::make(p.p, a ? a : term::make(p.s->p), b ? b : term::make(p.o->p));
		TRACE(printterm_substs(p, s); dout << " = "; if (!r) dout << "(null)"; else printterm(*r); dout << std::endl);
		return r;
	}
}

bool unify(const term& s, const subst& ssub, const term& d, subst& dsub, bool f) {
	setproc(L"unify");
	const term* v;
	bool r, ns = false;
	if (s.p < 0) 
		r = (v = evaluate(s, ssub)) ? unify(*v, ssub, d, dsub, f) : true;
	else if (d.p < 0) {
		if ((v = evaluate(d, dsub)))
			r = unify(s, ssub, *v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(s, ssub);
				ns = true;
			}
			r = true;
		}
	} else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o))
		r = false;
	else {
		r = !s.s || (unify(*s.s, ssub, *d.s, dsub, f) && unify(*s.o, ssub, *d.o, dsub, f));
	}
	TRACE(dout << "Trying to unify ";
		printterm_substs(s, ssub);
		dout<<" with ";
		printterm_substs(d, dsub);
		dout<<"... ";
		if (r) {
			dout << "passed";
			if (ns) {
				dout << " with new substitution: " << dict[d.p] << " / ";
				printterm(*dsub[d.p]);
			}
		} else dout << "failed";
		dout << std::endl);
	return r;
}

bool euler_path(proof* p) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (!ep->rul->p == !p->rul->p &&
			unify(*ep->rul->p, *ep->s, *p->rul->p, *p->s, false))
			break;
	if (ep) {
		TRACE(dout<<"Euler path detected\n");
		return true;
	}
	return false;
}

int builtin(const term& t, session& ss) {
	setproc(L"builtin");
	int r = -1;
	proof& p = *ss.p;
	const term* t0 = t.s ? evaluate(*t.s, *p.s) : 0;
	const term* t1 = t.o ? evaluate(*t.o, *p.s) : 0;
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(*t0->s, *p.s, *t.o, *p.s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(*t0->o, *p.s, *t.o, *p.s, true) ? 1 : 0;
	else if (t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) {
		if (!t0) t0 = t.s;
		if (!t1) t1 = t.o;
//		static std::set<proof*> ps;
//		if (ps.find(&p) != ps.end()) {
//			ps.erase(&p);
//			return -1;
//		}
		const term* vs = term::make(L"?S");
//		if (p.rul->p && p.rul->p->s == vs) 
//			return -1;
		proof* f = &proofs[nproofs++];
		rule* rl = &rules[nrules++];
		rl->body ( term::make ( rdfssubClassOf, t0, t1 ) );
		rl->body ( term::make ( A,              vs, t0 ) );
		rl->p    = term::make ( A,              vs, t1 ) ;
		p.rul->body(rl->p);
		f->rul = rl;
		f->prev = &p;//.prev;
		f->last = rl->body().begin();
		f->g.clear();
		if (euler_path(f)) 
			return -1;
		TRACE(dout<<"builtin created frame:"<<std::endl;printp(f));
//		ps.insert(f);
		ss.q.push_front(f);
		r = 1;
/*
		session s2;
		s2.kb = ss.kb;
		s2.goal.insert(q1);
		prove(s2);
//		TRACE(dout << "s2 returned evidence: " << std::endl; printe(s2.e));
		auto it = s2.e.find(rdfssubClassOf);
		if (it != s2.e.end()) {
			bool res = true;
			for (auto tg : it->second) {
				int subclasser = tg.first->s->p;
				session s3;
				s3.kb = ss.kb;
				const term* sc = term::make(subclasser);
				const term* q2 = term::make(A, t0, sc);
				s3.goal.insert(q2);
				prove(s3);
				for (auto e : s2.e[rdfssubClassOf])
					for (auto g : e.second)
						(*p.s)[t.s->p] = e.first->o;
				for (auto e : s3.e[A])
					for (auto g : e.second)
						(*p.s)[t.o->p] = e.first->s;
			}
			r = res ? 1 : -1;
		}*/
	}
	if (r == 1) {
		proof* r = &proofs[nproofs++];
		rule* rl = &rules[nrules++];
		rl->p = evaluate(t, *p.s);
		*r = p;
//		r->rul = rl;
		TRACE(dout << "builtin added rule: ";
			printr(*rl);
			dout << " by evaluating ";
			printterm_substs(t, *p.s);
			dout << std::endl);
		r->g = p.g;
		r->g.emplace_back(rl, subst());
		++r->last;
		ss.q.push_back(r);
		TRACE(dout<<"pushing frame:\n";printp(r));
	}

	return r;
}

void prove(session& ss) {
	++_indent;
	setproc(L"prove");
	termset& goal = ss.goal;
	ruleset& cases = ss.kb;
	rule* rg = &rules[nrules++];
	ss.p = &proofs[nproofs++];
	rg->p = 0;
	rg->body(goal);
	ss.p->rul = rg;
	ss.p->last = rg->body().begin();
	ss.p->prev = 0;
	ss.q.push_back(ss.p);
	TRACE(dout << KRED << "Facts:\n";);
	TRACE(printrs(cases));
	TRACE(dout << KGRN << "Query: "; printl(goal); dout << KNRM << std::endl);
	++_indent;
	do {
		ss.p = ss.q.back();
		ss.q.pop_back();
		proof& p = *ss.p;
		TRACE(dout<<"popped frame:\n";printp(&p));
		if (p.callback.target<int(struct session&)>() && !p.callback(ss)) continue;
		if (p.last != p.rul->body().end()) {
			const term* t = *p.last;
			if (!t) throw 0;
			TRACE(dout<<"Tracking back from ";
				printterm(*t);
				dout << std::endl);
			if (builtin(*t, ss) != -1) continue;

			for (auto x : cases)
				for (auto rl : x.second) {
					subst s;
					if (unify(*t, *p.s, *rl->p, s, true)) {
						if (euler_path(&p))
							continue;
						proof* r = &proofs[nproofs++];
						r->rul = rl;
						r->last = r->rul->body().begin();
						r->prev = &p;
						r->s = std::make_shared<subst>(s);
						r->g = p.g;
						if (rl->body().empty())
							r->g.emplace_back(rl, subst());
						ss.q.push_front(r);
					} 
				}
		}
		else if (!p.prev) {
			for (auto r = p.rul->body().begin(); r != p.rul->body().end(); ++r) {
				const term* t = evaluate(**r, *p.s);
				ss.e[t->p].emplace_back(t, p.g);
			}
		//	ss.q.clear();
		} else {
			ground g = p.g;
			proof* r = &proofs[nproofs++];
			if (!p.rul->body().empty())
				g.emplace_back(p.rul, *p.s);
			*r = *p.prev;
			r->g = g;
			r->s = std::make_shared<subst>(*p.prev->s);
			unify(*p.rul->p, *p.s, **r->last, *r->s, true);
			++r->last;
			ss.q.push_back(r);
			continue;
		}
		if (p.next) {
			*p.next->s = *p.s;
			ss.q.push_back(p.next);
		}
	} while (!ss.q.empty());
	--_indent;
	--_indent;
	TRACE(dout << KWHT << "Evidence:" << std::endl; 
		printe(ss.e); dout << KNRM);
}

bool equals(const term* x, const term* y) {
	return (!x == !y) && (!x || (x && equals(x->s, y->s) && equals(x->o, y->o)));
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
	if (!p) return;
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
	dout << indent() << L"rule:   ";
	printr(*p->rul);
	dout<<std::endl<<indent();
	if (p->prev)
		dout << L"prev:   " << p->prev <<std::endl<<indent()<< L"subst:  ";
	else
		dout << L"prev:   (null)"<<std::endl<<indent()<<"subst:  ";
	prints(*p->s);
	dout <<std::endl<<indent()<< L"ground: " << std::endl;
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
	if(!r.body().empty())
	{
		printl(r.body());
		dout << L" => ";
	}
	if (r.p)
		printterm(*r.p);
	if(r.body().empty())
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
	printl_substs(r.body(), s);
	dout << L" => ";
	printterm_substs(*r.p, s);
}

string format(const rule& r) {
	std::wstringstream ss;
	if (!r.body().empty())
		ss << format(r.body()) << L" => ";
	ss << format(r.p);
	return ss.str();
}

void printg(const ground& g) {
	for (auto x : g) {
		dout << indent();
		printr_substs(*x.first, x.second);
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
		}
}
}

const prover::term* mkterm(const wchar_t* p, const wchar_t* s = 0, const wchar_t* o = 0) {
//	prover::term* t = &prover::terms[prover::nterms++];
	const prover::term* ps = s ? mkterm(s, 0, 0) : 0;
	const prover::term* po = o ? mkterm(o, 0, 0) : 0;
	return prover::term::make(dict.set(string(p)), ps, po);
}

const prover::term* mkterm(string s, string p, string o) {
	return mkterm(p.c_str(), s.c_str(), o.c_str());
}

const prover::term* quad2term(const quad& p) {
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
		ss.kb[r->p->p].push_back(r);
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		r = &prover::rules[prover::nrules++];
//		r = prover::rule();
		const prover::term* tt = quad2term( *y );
		r->p = tt;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				r->body( quad2term( *z ) );
		ss.kb[r->p->p].push_back(r);
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
	prover::prove(ss);
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
