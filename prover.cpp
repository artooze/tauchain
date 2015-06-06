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
#include <dlfcn.h>

using namespace boost::algorithm;

int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf, _dlopen, _dlclose, _dlsym, _dlerror;
int _indent = 0;

etype littype(string s);
string littype(etype s);

#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif

bool prover::hasvar(termid id) {
	const term p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

prover::termid prover::evaluate(termid id, const subst& s) {
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
	const term s = get(_s);
	const term d = get(_d);
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

bool prover::euler_path(proof* p, termid t) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (!kb.head()[ep->rul] == !t &&
			unify(kb.head()[ep->rul], ep->s, t, p->s, false))
			break;
	if (ep) { TRACE(dout<<"Euler path detected\n"); }
	return ep;
}

prover::termid prover::list_next(prover::termid cons, proof& p) {
	//typedef boost::container::map<resid, boost::container::list<std::pair<termid, ground>>> evidence;
//	typedef boost::container::list<std::pair<ruleid, subst>> ground;
// ?R rdf:rest t. ?A rdf:rest ?R
//	return get(*prover(kb, { make(rdfrest, va, get(t).s) })(&p.s).result.begin()).s;
	if (!cons) return 0;
	setproc(L"list_next");
	termset ts;
	ts.push_back(make(rdfrest, get(cons).s, va));
	(*this)( ts , &p.s);
	if (e.find(rdfrest) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdfrest])
		if (get(x.first).s == cons) {
			r = get(x.first).o;
			break;
		}
	TRACE(dout<<"current cons: " << format(cons) << " next cons: " << format(r) << std::endl);
	return r;
//	auto s = find(rdfrest, cons, p.s, false);
//	termid r = s.empty() ? 0 : *s.begin();
	TRACE(dout<<"current cons: " << format(cons) << " next cons: " << format(r) << std::endl);
//	return r;
}

int prover::builtin(termid id, proof* p, std::deque<proof*>& queue) {
	setproc(L"builtin");
	const term t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, p->s) : 0;
	term r1, r2;
	const term *t0 = i0 ? &(r1=get(i0=evaluate(t.s, p->s))) : 0;
	const term* t1 = i1 ? &(r2=get(i1=evaluate(t.o, p->s))) : 0;
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
	else if (t.p == rdffirst) {
		termid n = list_next(id, *p);
		while ((n = list_next(n, *p)));
	}
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == _dlopen && t0 && t1) {
		if (t1->p > 0) throw std::runtime_error("dlopen must be called with variable object.");
//		termid fname = 

/*
	_args first ?fname;
	_args rest ?args1;
	?args1 first ?flag;
	?args1 rest nil;
	_args=t.s dlopen ?handle
*/		
//		u64 handle = (u64)dlopen(ws(dict[t0->p].value).c_str(), RTLD_LAZY | RTLD_GLOBAL);
//		prover pr(&kb, { 
//			quad( dict.get(*t0), dict.get(rdffirst), mkbnode("?fname")  ),
//			quad( dict.get(*t0), dict.get(rdfrest), mkbnode("?argsnext")  ),
//			quad( mkbnode("?argsnext"), dict.get(rdffirst), mkbnode("?flag")  )
//			})(p.s /* init subprover with current's frame substition */);
//		termset q;
//		
//		r = unify(i1, p->s, make(mkliteral(tostr(handle),pstr(XSD_INTEGER),0),0,0), p->s, true) ? 1 : 0;
	}
//void *dlopen(const char *filename, int flag);
//char *dlerror(void);
//void *dlsym(void *handle, const char *symbol);
//int dlclose(void *handle);
	else if (t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) {
		if (!t0) t0 = &get(i0=t.s);
		if (!t1) t1 = &get(i1=t.o);
		termid h = make ( A,           i0, i1 );
		termset ts;
		ts.push_back ( make ( rdfssubClassOf, va, i1 ) );
		ts.push_back ( make ( A,              i0, va ) );
		proof* f = new proof;
		f->rul = kb.add(h, ts, this);
		f->prev = p;
		f->last = 0;
		f->g = p->g;
		if (euler_path(f, h))
			return -1;
		step(f, queue, false);
		TRACE(dout<<"builtin created frame:"<<std::endl;printp(f));
		r = -1;
	}
	if (r == 1) {
		proof* r = new proof;
		*r = *p;
		uint rl = kb.add(evaluate(id, p->s), termset(), this);
		TRACE(dout << "builtin added rule: " << format(rl) << " by evaluating "; printterm_substs(id, p->s); dout << std::endl);
		r->g = p->g;
		r->g.emplace_back(rl, subst());
		++r->last;
		step(r, queue);
	}

	return r;
}

bool prover::maybe_unify(const term s, const term d) {
	return (s.p < 0 || d.p < 0) ? true : (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) ? false :
		!s.s || (maybe_unify(get(s.s), get(d.s)) && maybe_unify(get(s.o), get(d.o)));
}

std::set<uint> prover::match(termid _e) {
	if (!_e) return std::set<uint>();
	setproc(L"match");
	std::set<uint> m;
	termid h;
	const term e = get(_e-1);
	for (uint n = 0; n < kb.size(); ++n)
		if (((h=kb.head()[n])) && maybe_unify(e, get(h)))
			m.insert(n);
	return m;
}

void prover::step(proof* p, std::deque<proof*>& queue, bool del) {
	setproc(L"step");
	TRACE(dout<<"popped frame:\n";printp(p));
	if (p->last != kb.body()[p->rul].size() && !euler_path(p, kb.head()[p->rul])) {
		termid t = kb.body()[p->rul][p->last];
		if (!t) throw 0;
		TRACE(dout<<"Tracking back from " << format(t) << std::endl);
		if (builtin(t, p, queue) != -1) return;
		for (auto rl : match(evaluate(t, p->s))) {
			subst s;
			if (!unify(t, p->s, kb.head()[rl], s, true)) continue;
			proof* r = new proof(rl, 0, p, s, p->g);
			if (kb.body()[rl].empty()) r->g.emplace_back(rl, subst());
			queue.push_front(r);
		}
	}
	else if (!p->prev) {
		for (auto r = kb.body()[p->rul].begin(); r != kb.body()[p->rul].end(); ++r) {
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

prover::termid prover::quad2term(const quad& p) {
	return make(p.pred, make(p.subj, 0, 0), make(p.object, 0, 0));
}

void prover::addrules(pquad q) {
	const string &s = q->subj->value, &p = q->pred->value, &o = q->object->value;
	if ( p != implication || quads.find ( o ) == quads.end() ) 
		kb.add(quad2term(*q), termset(), this);
	else for ( pquad y : *quads.at ( o ) ) {
		termset ts;
		if ( quads.find ( s ) != quads.end() )
			for ( pquad z : *quads.at ( s ) )
				ts.push_back( quad2term( *z ) );
		kb.add(quad2term(*y), ts, this);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

//prover::prover ( ruleset* _kb/*, const qlist query*/ ) : prover(_kb) {
//	for ( auto q : query ) goal.push_back( quad2term( *q ) );
//}
void prover::operator()(qlist query, const subst* s) {
	termset goal;
	for ( auto q : query ) goal.push_back( quad2term( *q ) );
	return (*this)(goal, s);
}

prover::prover(prover::ruleset* _kb/*, prover::termset* query*/) : kb(_kb ? *_kb : *new ruleset) {//, goal(query ? *query : *new termset) {
	kbowner = !_kb;
//	goalowner = !query;
	va = make(mkiri(L"?A"));
	CL(initcl());
}

prover::~prover() { 
	//if (kbowner) delete &kb; if (goalowner) delete &goal; 
}

prover::prover ( qdb qkb/*, qlist query*/ ) : prover() {
	quads = qkb;
	for ( pquad quad : *quads.at(L"@default")) addrules(quad);
//	for ( auto q : query ) goal.push_back( quad2term( *q ) );
}

void prover::operator()(termset& goal, const subst* s) {
	proof* p = new proof;
	std::deque<proof*> queue;
	p->rul = kb.add(0, goal, this);
	p->last = 0;
	p->prev = 0;
	if (s) p->s = *s;
	TRACE(dout << KRED << "Facts:\n" << formatkb() << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	queue.push_front(p);
	do {
		proof* q = queue.back();
		queue.pop_back();
		step(q, queue);
	} while (!queue.empty());
	TRACE(dout << KWHT << "Evidence:" << std::endl; 
		printe(); dout << KNRM);
//	return results();
}

prover::term::term(resid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}

const prover::term& prover::get(termid id) {
	if (!id) throw 0;
	return _terms[id - 1]; 
}

prover::termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}

prover::termid prover::make(resid p, termid s, termid o) {
#ifdef DEBUG
	if (!p) throw 0;
#endif
	_terms.emplace_back(p, s, o);
	return _terms.size();
}

uint prover::ruleset::add(termid t, const termset& ts, prover* p) {
	_head.push_back(t);
	_body.push_back(ts);
#ifdef DEBUG
	if (!ts.size() && !p->get(t).s) throw 0;
#endif	
	return _head.size()-1;
}

etype littype(string s) {
	if (s == XSD_BOOLEAN) return BOOLEAN;
	if (s == XSD_DOUBLE) return DOUBLE;
	if (s == XSD_INTEGER) return INT;
	if (s == XSD_FLOAT) return FLOAT;
	if (s == XSD_DECIMAL) return DECIMAL;
	if (s == XSD_ANYURI) return URISTR;
	if (s == XSD_STRING) return STR;
	throw wruntime_error(L"Unidentified literal type" + s);
}

string littype(etype s) {
	if (s == BOOLEAN) return XSD_BOOLEAN;
	if (s == DOUBLE) return XSD_DOUBLE;
	if (s == INT) return XSD_INTEGER;
	if (s == FLOAT) return XSD_FLOAT;
	if (s == DECIMAL) return XSD_DECIMAL;
	if (s == URISTR) return XSD_ANYURI;
	if (s == STR) return XSD_STRING;
}

bool prover::term::isstr() const { node n = dict[p]; return n._type == node::LITERAL && n.datatype == XSD_STRING; }
prover::term::term() : p(0), s(0), o(0) {}
prover::term::term(const prover::term& t) : p(t.p), s(t.s), o(t.o) {}
prover::term& prover::term::operator=(const prover::term& t) { p = t.p; s = t.s; o = t.o; return *this; }

#ifdef OPENCL
#define __CL_ENABLE_EXCEPTIONS
 
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

std::string clprog = ""
"bool maybe_unify(termid _s, termid _d, const term* t) {"
"	local const term s = t[_s-1], d = t[_d-1];"
"	if (s.p < 0 || d.p < 0) return true;"
"	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;"
"	return !s.s || (maybe_unify(s.s, d.s) && maybe_unify(s.o, d.o));"
"}"
"kernel void match(termid e, const termid* t, uint sz, global termid* res) {"
"	uint n = get_global_id(0);"
"	prefetch(t[n]);"
"	local "
"	if (t[n] && maybe_unify(e, t[n]))"
"		res[pos++] = n;"
"}";
#endif
#ifdef OPENCL
void prover::initcl() {
	cl_int err = CL_SUCCESS;
	try {
		cl::Platform::get(&platforms);
		if (platforms.size() == 0) {
			derr << "Platform size: 0" << std::endl;
			exit(-1);
		}
		cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
		context = cl::Context (CL_DEVICE_TYPE_CPU, properties); 
		devices = context.getInfo<CL_CONTEXT_DEVICES>();
		prog = cl::Program(clprog);
		cl::Kernel kernel(prog, "match", &err);
		cl::Event event;
		cq = cl::CommandQueue (context, devices[0], 0, &err);
		cq.enqueueNDRangeKernel( kernel, cl::NullRange, cl::NDRange(4,4), cl::NullRange, NULL, &event); 
		event.wait();
	} catch (cl::Error err) { derr << err.what() << ':' << err.err() << std::endl; }
}
#endif 
