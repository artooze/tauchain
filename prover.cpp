#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <iostream>
#include <ostream>
#include "misc.h"
#include <utility>
#include "object.h"
#include "prover.h"
#include <iterator>
#include <forward_list>
#include <boost/algorithm/string.hpp>
#include <dlfcn.h>
#ifdef with_marpa
#include "marpa_tau.h"
#endif

using namespace boost::algorithm;
int _indent = 0;

const uint max_terms = 1024 * 1024;

bool prover::hasvar(termid id) {
	const term p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

prover::termid prover::evaluate(termid id, const subst& s) {
	if (!id) return 0;
	setproc(L"evaluate");
	termid r;
	const term p = get(id);
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
	TRACE(printterm_substs(id, s); dout << " = "; if (!r) dout << "(null)"; else dout << format(r); dout << std::endl);
	return r;
}

bool prover::unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f) {
	setproc(L"unify");
	termid v;
	if (!_d != !_s) return false;
	const term s = get(_s), d = get(_d);
	bool r, ns = false;
	if (s.p < 0) r = (v = evaluate(_s, ssub)) ? unify(v, ssub, _d, dsub, f) : true;
	else if (d.p < 0) {
		if ((v = evaluate(_d, dsub))) r = unify(_s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(_s, ssub);
				ns = true;
			}
			r = true;
		}
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else r = !s.s || (unify(s.s, ssub, d.s, dsub, f) && unify(s.o, ssub, d.o, dsub, f));
	TRACE(
		dout << "Trying to unify ";
		printterm_substs(_s, ssub);
		dout<<" with ";
		printterm_substs(_d, dsub);
		dout<<" : ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << std::endl);
	return r;
}

bool prover::euler_path(proof* p, termid t, const std::deque<proof*>& queue) {
	{proof* ep = p;
	while ((ep = ep->prev))
		if (unify(kb.head()[ep->rul], ep->s, t, p->s, false))
			{ TRACE(dout<<"Euler path detected\n"); return true; }
	}
	for (auto ep : queue)
		if (unify(kb.head()[ep->rul], ep->s, t, p->s, false))
			{ TRACE(dout<<"Euler path detected\n"); return true; }
	return false;
}

prover::termid prover::tmpvar() {
	static int last = 1;
	return make(mkiri(pstr(string(L"?__v")+_tostr(last++))),0,0);
}

prover::termid prover::list_next(prover::termid cons, proof& p) {
	if (!cons) return 0;
	setproc(L"list_next");
	termset ts;
	ts.push_back(make(rdfrest, cons, tmpvar()));
	(*this)( ts , &p.s);
	if (e.find(rdfrest) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdfrest])
		if (get(x.first).s == cons) {
			r = get(x.first).o;
			break;
		}
	TRACE(dout <<"current cons: " << format(cons)<< " next cons: " << format(r) << std::endl);
	return r;
}

prover::termid prover::list_first(prover::termid cons, proof& p) {
	if (!cons || get(cons).p == rdfnil) return 0;
	setproc(L"list_first");
	termset ts;
	ts.push_back(make(rdffirst, cons, tmpvar()));
	(*this)( ts , &p.s);
	if (e.find(rdffirst) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdffirst])
		if (get(x.first).s == cons) {
			r = get(x.first).o;
			break;
		}
	TRACE(dout<<"current cons: " << format(cons) << " next cons: " << format(r) << std::endl);
	return r;
}

uint64_t dlparam(const node& n) {
	uint64_t p;
	if (!n.datatype || !n.datatype->size() || *n.datatype == *XSD_STRING) {
		p = (uint64_t)n.value->c_str();
	} else {
		const string &dt = *n.datatype, &v = *n.value;
		if (dt == *XSD_BOOLEAN) p = (lower(v) == L"true");
		else if (dt == *XSD_DOUBLE) {
			double d;
			d = std::stod(v);
			memcpy(&p, &d, 8);
		} else if (dt == *XSD_INTEGER /*|| dt == *XSD_PTR*/)
			p = std::stol(v);
	}
	return p;
}

std::vector<prover::termid> prover::get_list(prover::termid head, proof& p) {
	setproc(L"get_list");
	termid t = list_first(head, p);
	std::vector<termid> r;
	TRACE(dout<<"get_list with "<<format(head));
	while (t) {
		r.push_back(t);
		head = list_next(head, p);
		t = list_first(head, p);
	}
//	e = e1;
	TRACE(dout<<" returned " << r.size() << " items: "; for (auto n : r) dout<<format(n)<<' '; dout << std::endl);
	return r;
}

void* testfunc(void* p) {
	derr <<std::endl<< "***** Test func called ****** " << p << std::endl;
	return (void*)(pstr("testfunc_result")->c_str());
//	return 0;
}

string prover::predstr(prover::termid t) { return *dict[get(t).p].value; }
string prover::preddt(prover::termid t) { return *dict[get(t).p].datatype; }

int prover::builtin(termid id, proof* p, std::deque<proof*>& queue) {
	setproc(L"builtin");
	const term t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, p->s) : 0;
	term r1, r2;
	const term *t0 = i0 ? &(r1=get(i0=evaluate(t.s, p->s))) : 0;
	const term* t1 = i1 ? &(r2=get(i1=evaluate(t.o, p->s))) : 0;
	TRACE(	dout<<"called with term " << format(id); 
		if (t0) dout << " subject = " << format(i0);
		if (t1) dout << " object = " << format(i1);
		dout << endl);
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
//	else if (t.p == rdffirst && t0 && startsWith(*dict[t0->p].value,L"_:list") && (!t0->s == !t0->o))
//		r = ((!t0->s && !t0->o) || unify(t0->s, p->s, t.o, p->s, true)) ? 1 : 0;
//	else if (t.p == rdfrest && t0 && startsWith(*dict[t0->p].value,L"_:list") && (!t0->s == !t0->o))
//		r = ((!t0->s && !t0->o) || unify(t0->o, p->s, t.o, p->s, true)) ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == _dlopen) {
		if (get(t.o).p > 0) throw std::runtime_error("dlopen must be called with variable object.");
		std::vector<termid> params = get_list(t.s, *p);
		if (params.size() >= 2) {
			void* handle;
			try {
				string f = predstr(params[0]);
				if (f == L"0") handle = dlopen(0, std::stol(predstr(params[1])));
				else handle = dlopen(ws(f).c_str(), std::stol(predstr(params[1])));
				pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
				p->s[get(t.o).p] = make(dict.set(n), 0, 0);
				r = 1;
			} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
			catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
		}
	}
	else if (t.p == _dlerror) {
		if (get(t.o).p > 0) throw std::runtime_error("dlerror must be called with variable object.");
		auto err = dlerror();
		pnode n = mkliteral(err ? pstr(err) : pstr(L"NULL"), 0, 0);
		p->s[get(t.o).p] = make(dict.set(n), 0, 0);
		r = 1;
	}
	else if (t.p == _dlsym) {
//		if (t1) throw std::runtime_error("dlsym must be called with variable object.");
		if (!t1) {
			std::vector<termid> params = get_list(t.s, *p);
			void* handle;
			if (params.size()) {
				try {
					handle = dlsym((void*)std::stol(predstr(params[0])), ws(predstr(params[1])).c_str());
					pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
					p->s[t1->p] = make(dict.set(n), 0, 0);
					r = 1;
				} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
				catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
			}
		}
	}
	else if (t.p == _dlclose) {
//		if (t1->p > 0) throw std::runtime_error("dlclose must be called with variable object.");
		pnode n = mkliteral(tostr(ws(dlerror())), 0, 0);
		p->s[t1->p] = make(dict.set(mkliteral(tostr(dlclose((void*)std::stol(*dict[t.p].value))), XSD_INTEGER, 0)), 0, 0);
		r = 1;
	}
	else if (t.p == _invoke) {
		typedef void*(*fptr)(void*);
		auto params = get_list(t.s, *p);
		if (params.size() != 2) return -1;
		if (preddt(params[0]) != *XSD_INTEGER) return -1;
		fptr func = (fptr)std::stol(predstr(params[0]));
		void* res;
		if (params.size() == 1) {
			res = (*func)((void*)dlparam(dict[*get_list(params[1],*p).begin()]));
			pnode n = mkliteral(tostr((uint64_t)res), XSD_INTEGER, 0);
			p->s[get(t.o).p] = make(dict.set(n), 0, 0);
		}
		r = 1;
	}
	else if ((t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) && t.s && t.o) {
		termset ts(2);
		termid va = tmpvar();
		ts[0] = make ( rdfssubClassOf, va, t.o );
		ts[1] = make ( A, t.s, va );
		queue.push_front(new proof( kb.add(make ( A, t.s, t.o ), ts), 0, p, subst(), p->g)/*, queue, false*/);
	}
	#ifdef with_marpa
	else if (t.p == marpa_parser_iri)// && !t.s && t.o) //fixme
	/* ?X is a parser created from grammar */
	{
		void* handle = marpa_parser(this, get(t.o).p, p);
		pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
		p->s[get(t.s).p] = make(dict.set(n), 0, 0);
		r = 1;
	}
	else if (t.p == marpa_parse_iri) {
	/* ?X is a parse of (input with parser) */
		if (get(t.s).p > 0) throw std::runtime_error("marpa_parse must be called with variable subject.");
		//auto parser = dict[get(get(i1).s).p].value;
		//if (t1->p != Dot) { TRACE(dout<<std::endl<<"p == " << *dict[t1->p].value<<std::endl);  return -1;}
		TRACE(dout<<std::endl<<format(i1)<<std::endl);
		auto parser = *dict[get(t1->s).p].value;

		//auto text = dict[get(get(get(t.o).o).s).p].value;
		//pnode result = marpa_parse((void*)std::stol(parser), L"SSS");//*text);
		//p->s[get(t.s).p] = make(dict.set(result), 0, 0);
		r = 1;
	}
	#endif
	if (r == 1) {
		proof* r = new proof;
		*r = *p;
		r->g = p->g;
		r->g.emplace_back(kb.add(evaluate(id, p->s), termset()), subst());
		++r->last;
		step(r, queue);
	}

	return r;
}

void prover::step(proof* p, std::deque<proof*>& queue, bool) {
	setproc(L"step");
	++steps;
	TRACE(dout<<"popped frame:\n";printp(p));
	if (p->last != kb.body()[p->rul].size()) {
		if (euler_path(p, kb.head()[p->rul], queue)) return;
		termid t = kb.body()[p->rul][p->last];
		TRACE(dout<<"Tracking back from " << format(t) << std::endl);
		if (builtin(t, p, queue) != -1) return;
		for (auto rl : kb[get(t).p]) {
			subst s;
			if (unify(t, p->s, kb.head()[rl], s, true)) {
				proof* r = new proof(rl, 0, p, s, p->g);
				if (kb.body()[rl].empty()) r->g.emplace_back(rl, subst());
				queue.push_front(r);
			}
		}
	}
	else if (!p->prev) {
		for (auto r = kb.body()[p->rul].begin(); r != kb.body()[p->rul].end(); ++r) {
			substs.push_back(p->s);
			termid t = evaluate(*r, p->s);
			if (!t || hasvar(t)) continue;
			TRACE(dout << "pushing evidence: " << format(t) << std::endl);
			e[get(t).p].emplace(t, p->g);
		}
	} else {
		ground g = p->g;
		if (!kb.body()[p->rul].empty())
			g.emplace_back(p->rul, p->s);
		proof* r = new proof(*p->prev);
		r->g = g;
		unify(kb.head()[p->rul], p->s, kb.body()[r->rul][r->last], r->s, true);
		++r->last;
		step(r, queue);
	}
	TRACE(dout<<"Deleting frame: " << std::endl; printp(p));
//	if (del) delete p;
}

prover::termid prover::list2term(std::list<pnode>& l, const qdb& quads) {
	setproc(L"list2term");
	termid t;
	if (l.empty()) t = make(Dot, 0, 0);
	else {
		pnode x = l.front();
		l.pop_front();
//		TRACE(dout << x->tostring() << endl);
		auto it = quads.second.find(*x->value);
		//its not a list
		if (it == quads.second.end())
			t = make(Dot, make(dict.set(x), 0, 0), list2term(l, quads));
		//its a list
		else {
			auto ll = it->second;
			t = make(Dot, list2term(ll, quads), list2term(l, quads));
		}
	}
	TRACE(dout << format(t) << endl);
	return t;
}

prover::termid prover::quad2term(const quad& p, const qdb& quads) {
	setproc(L"quad2term");
	TRACE(dout<<L"called with: "<<p.tostring()<<endl);
	termid t, s, o;
	//if (dict[p.pred] == rdffirst || dict[p.pred] == rdfrest) return 0;
	auto it = quads.second.find(*p.subj->value);
	if (it != quads.second.end()) {
		auto l = it->second;
		s = list2term(l, quads);
	}
	else
		s = make(p.subj, 0, 0);
	if ((it = quads.second.find(*p.object->value)) != quads.second.end()) {
		auto l = it->second;
		o = list2term(l, quads);
	}
	else
		o = make(p.object, 0, 0);
	t = make(p.pred, s, o);
	TRACE(dout<<"quad: " << p.tostring() << " term: " << format(t) << endl);
	return t;
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q.first ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

void prover::operator()(const qdb& query, const subst* s) {
	termset goal;
	termid t;
	for ( auto q : merge(query) ) 
		if (	dict[q->pred] != rdffirst && 
			dict[q->pred] != rdfrest &&
			(t = quad2term( *q, query )) )
			goal.push_back( t );
	return (*this)(goal, s);
}

prover::~prover() { }
prover::prover(const prover& q) : kb(q.kb), _terms(q._terms)/*, quads(q.quads)*/ { kb.p = this; } 

void prover::addrules(pquad q, qdb& quads) {
	setproc(L"addrules");
	TRACE(dout<<q->tostring()<<endl);
	const string &s = *q->subj->value, &p = *q->pred->value, &o = *q->object->value;
	//if (dict[q->pred] == rdffirst || dict[q->pred] == rdfrest) return;
	termid t;
	TRACE(dout<<"called with " << q->tostring()<<endl);
	if ((t = quad2term(*q, quads))) 
		kb.add(t, termset());
	if (p == implication) {
		if (quads.first.find(o) == quads.first.end()) quads.first[o] = mk_qlist();
		for ( pquad y : *quads.first.at ( o ) ) {
			if ( quads.first.find ( s ) == quads.first.end() ) continue;
			termset ts;
			for ( pquad z : *quads.first.at( s ) )
				if ((dict[z->pred] != rdffirst && 
					dict[z->pred] != rdfrest) &&
					(t = quad2term(*z, quads)))
					ts.push_back( t );
			if ((t = quad2term(*y, quads))) kb.add(t, ts);
		}
	}
}

prover::prover ( qdb qkb, bool check_consistency ) : /*quads(qkb),*/ kb(this) {
	auto it = qkb.first.find(L"@default");
	if (it == qkb.first.end()) throw std::runtime_error("Error: @default graph is empty.");
	if (qkb.first.find(L"false") == qkb.first.end()) qkb.first[L"false"] = make_shared<qlist>();
	for ( pquad quad : *it->second ) addrules(quad, qkb);
	if (check_consistency && !consistency(qkb)) throw std::runtime_error("Error: inconsistent kb");
}

bool prover::consistency(const qdb& quads) {
	setproc(L"consistency");
	bool c = true;
//	prover p(quads, false);
	prover p(*this);
	termid t = p.make(mkiri(pimplication), p.tmpvar(), p.make(False, 0, 0));
	termset g;
	g.push_back(t);
	p(g);
	auto ee = p.e;
	for (auto x : ee) for (auto y : x.second) {
		prover q(*this);
		g.clear();
//		p.e.clear();
		string s = *dict[p.get(p.get(y.first).s).p].value;
		if (s == L"GND") continue;
		TRACE(dout<<L"Trying to prove false context: " << s << endl);
		qdb qq;
		qq.first[L""] = quads.first.at(s);
		q(qq);
		if (q.e.size()) {
			derr << L"Inconsistency found: " << q.format(y.first) << L" is provable as true and false."<<endl;
			c = false;
		}
	}
	return c;
}

#include <chrono>
void prover::operator()(termset& goal, const subst* s) {
//	setproc(L"prover()");
	proof* p = new proof;
	std::deque<proof*> queue;
//	kb.mark();
	p->rul = kb.add(0, goal);
	p->last = 0;
	p->prev = 0;
	if (s) p->s = *s;
	TRACE(dout << KRED << L"Rules:\n" << formatkb()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
	TRACE(dout << KRED << L"Rules:\n" << kb.format()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
	queue.push_front(p);
	using namespace std;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	do {
		proof* q = queue.back();
		queue.pop_back();
		step(q, queue);
	} while (!queue.empty());
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
	TRACE(dout << KWHT << "Evidence:" << endl;printe();/* << ejson()->toString()*/ dout << KNRM);
	TRACE(dout << "elapsed: " << (duration / 1000.) << "ms steps: " << steps << endl);
//	kb.revert();
//	return results();
}

prover::term::term(resid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}

const prover::term& prover::get(termid id) const {
	if (!id || id > (termid)_terms.size()) throw std::runtime_error("invalid term id passed to prover::get");
	return _terms[id - 1]; 
}

prover::termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}

prover::termid prover::make(resid p, termid s, termid o) {
#ifdef DEBUG
	if (!p) throw 0;
#endif
	return _terms.add(p, s, o);
//	return _terms.size();
}

prover::ruleid prover::ruleset::add(termid t, const termset& ts) {
	setproc(L"ruleset::add");
	ruleid r =  _head.size();
	_head.push_back(t);
	_body.push_back(ts);
	r2id[t ? p->get(t).p : 0].push_back(r);
	TRACE(dout<<r<<tab<<p->formatr(r)<<endl);
	return r;
	//TRACE(if (!ts.size() && !p->get(t).s) throw 0);
}

prover::ruleid prover::ruleset::add(termid t) {
	termset ts;
	return add(t, ts);
}


//bool prover::term::isstr() const { node n = dict[p]; return n._type == node::LITERAL && n.datatype == XSD_STRING; }
prover::term::term() : p(0), s(0), o(0) {}
//prover::term::term(const prover::term& t) : p(t.p), s(t.s), o(t.o) {}
//prover::term& prover::term::operator=(const prover::term& t) { p = t.p; s = t.s; o = t.o; return *this; }

pobj prover::json(const termset& ts) const {
	polist_obj l = mk_olist_obj(); 
	for (termid t : ts) l->LIST()->push_back(get(t).json(*this));
	return l;
}

pobj prover::json(const subst& s) const {
	psomap_obj o = mk_somap_obj();
	for (auto x : s) (*o->MAP())[dstr(x.first)] = get(x.second).json(*this);
	return o;
}

pobj prover::json(ruleid t) const {
	pobj m = mk_somap_obj();
	(*m->MAP())[L"head"] = get(kb.head()[t]).json(*this);
	(*m->MAP())[L"body"] = json(kb.body()[t]);
	return m;
};

pobj prover::json(const ground& g) const {
	pobj l = mk_olist_obj();
	for (auto x : g) {
		psomap_obj o = mk_somap_obj();
		(*o->MAP())[L"src"] = json(x.first);
		(*o->MAP())[L"env"] = json(x.second);
		l->LIST()->push_back(o);
	}
	return l;
}

pobj prover::ejson() const {
	pobj o = mk_somap_obj();
	for (auto x : e) {
		polist_obj l = mk_olist_obj();
		for (auto y : x.second) {
			psomap_obj t = mk_somap_obj(), t1;
			(*t->MAP())[L"head"] = get(y.first).json(*this);
			(*t->MAP())[L"body"] = t1 = mk_somap_obj();
			(*t1->MAP())[L"pred"] = mk_str_obj(L"GND");
			(*t1->MAP())[L"args"] = json(y.second);
			l->LIST()->push_back(t);
		}
		(*o->MAP())[dstr(x.first)] = l;
	}
	return o;
}

void prover::ruleset::mark() {
	_r2id = r2id;
	if (!m) m = size(); 
	else m = std::min(size(), m);
}

void prover::ruleset::revert() {
	r2id = _r2id;
	if ( size() <= m ) { 
		m = 0; 
		return; 
	}
	_head.erase(_head.begin() + (m-1), _head.end());
	_body.erase(_body.begin() + (m-1), _body.end());
	m = 0;
}

string prover::ruleset::format() const {
	setproc(L"ruleset::format");
//	dump();
	std::wstringstream ss;
	ss << L'['<<endl;
	for (auto it = r2id.begin(); it != r2id.end();) {
//		TRACE(dout<<it->first<<endl);
		ss <<tab<< L'{' << endl <<tab<<tab<<L'\"'<<(it->first ? *dict[it->first].value : L"")<<L"\":[";
		for (auto iit = it->second.begin(); iit != it->second.end();) {
			ss << p->formatr(*iit, true);
//			TRACE(dout<<s<<endl);
			if (++iit != it->second.end()) ss << L',';
			ss << endl;
		}
		ss << L"]}";
		if (++it != r2id.end()) ss << L',';
	}
	ss << L']';
	return ss.str();
}
