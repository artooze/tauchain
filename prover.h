#ifndef __PROVER_H__
#define __PROVER_H__

/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include <deque>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include <functional>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>

typedef u64 subid;
typedef u64 termid;
typedef boost::container::map<resid, termid> subst;
subid sub(const subst& s = subst());

class prover {
public:

	class term {
	public:
		term();
		term(resid _p, termid _s, termid _o);
		resid p;
		termid s, o;
		pobj json(const prover&) const;
	};
	typedef u64 ruleid;
	typedef boost::container::vector<termid> termset;
	static boost::container::map<subid, subst> subs;
	class ruleset {
		termset _head;
		boost::container::vector<termset> _body;
		size_t m = 0;
	public:
		prover* p;
		ruleset(prover* _p) : p(_p) {}
		ruleid add(termid t, const termset& ts);
		ruleid add(termid t);
		const termset& head() const				{ return _head; }
		const boost::container::vector<termset>& body() const	{ return _body; }
		size_t size()						{ return _head.size(); }
		void mark();
		void revert();
		typedef boost::container::list<ruleid> rulelist;
		typedef boost::container::map<resid, rulelist> r2id_t;
		string format() const;
		inline const rulelist& operator[](resid id) const {
			static rulelist empty;
			auto x = r2id.find(id);
			return x == r2id.end() ? empty : x->second;
		}
/*		void dump() const {
			for (auto x : r2id) {
				dout << x.first << endl;
				for (auto y : x.second)
					dout << tab << y << tab << p->formatr(y, false) << tab << p->formatr(y, true)<<endl;
			}
		}*/
	private:
		r2id_t r2id, _r2id;
	} kb;
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	void operator()(termset& goal/*, const subst* s = 0*/);
	void operator()(const qdb& goal/*, const subst* s = 0*/);
	const term& get(termid) const;
	const term& get(resid) const { throw std::runtime_error("called get(termid) with resid"); }
	~prover();

	typedef boost::container::list<std::pair<ruleid, subid>> ground;
	typedef boost::container::map<resid, boost::container::set<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<subst> substs;
	termid tmpvar();
	void printg(const ground& g);
	void printe();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(resid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	string format(const termset& l, bool json = false);
	void prints(const subst& s);
	void prints(subid s) { prints(subs[s]); }

	struct proof {
		ruleid rul;
		uint last;
		std::shared_ptr<proof> prev;
		subid s;
		ground g;
		proof() : rul(0), prev(0) {}
		proof(ruleid r, uint l = 0, proof* p = 0, const subid& _s = sub(), const ground& _g = ground() ) 
			: rul(r), last(l), prev(make_shared<proof>(*p)), s(_s), g(_g) {}
		proof(const proof& p) : rul(p.rul), last(p.last), prev(p.prev ? make_shared<proof>(*p.prev) : 0), s(p.s), g(p.g) {}
	};

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	string format(termid id, bool json = false);
private:

	class termdb {
	public:
		typedef boost::container::list<termid> termlist;
		typedef boost::container::map<resid, termlist> p2id_t;
		size_t size() const { return terms.size(); }
		inline const term& operator[](termid id) const { return terms.at(id); }
		inline const termlist& operator[](resid id) const { return p2id.at(id); }
		inline termid add(resid p, termid s, termid o) { terms.emplace_back(p, s, o); termid r = size(); p2id[p].push_back(r); return r; }
	private:
		boost::container::vector<term> terms;
		p2id_t p2id;
	} _terms;
	friend ruleset;
//	qdb quads;
	int steps = 0;

	void step (proof*, std::deque<proof*>&, bool del = true);
	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	inline termid evaluate(termid id, subid s) { return evaluate(id, subs[s]); }
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	inline bool unify(termid _s, subid ssub, termid _d, subid dsub, bool f) {
		return unify(_s, subs[ssub], _d, subs[dsub], f);
	}
	bool euler_path(proof* p, termid t, const std::deque<proof*>&);
	int builtin(termid id, proof* p, std::deque<proof*>& queue);
	bool match(termid e, termid h);
	termid quad2term(const quad& p, const qdb& quads);
	termid list_next(termid t, proof&);
	termid list_first(termid t, proof&);
	bool kbowner, goalowner;
	string predstr(termid t);
	string preddt(termid t);
	string formatg(const ground& g, bool json = false);
	bool islist(termid);
	bool consistency(const qdb& quads);

	// formatters

	string format(resid) { throw std::runtime_error("called format(termid) with resid"); }
	string format(resid, bool) { throw std::runtime_error("called format(termid) with resid"); }
	string format(term t, bool json = false);
	string formatr(ruleid r, bool json = false);
	string formatkb(bool json = false);
	void printp(proof* p);
	string formats(const subst& s, bool json = false);
	string formats(subid& s, bool json = false) { return formats(subs[s], json); }
	void printterm_substs(termid id, const subst& s);
	void printl_substs(const termset& l, const subst& s);
	void printr_substs(ruleid r, const subst& s);
	void jprinte();
	pobj json(const termset& ts) const;
	pobj json(const subst& ts) const;
	pobj json(const ground& g) const;
	pobj json(ruleid rl) const;
	pobj ejson() const;
};
#endif
