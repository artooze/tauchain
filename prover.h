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
#include <future>
#include <functional>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

typedef boost::interprocess::allocator<void, boost::interprocess::managed_mapped_file::segment_manager> allocator_t;
extern boost::interprocess::managed_mapped_file* segment;
extern allocator_t* alloc;

typedef u64 termid;
typedef boost::interprocess::allocator<std::pair<const resid, termid>, boost::interprocess::managed_mapped_file::segment_manager> salloc;
typedef boost::container::map<resid, termid, std::less<resid>, salloc> subst;
shared_ptr<subst> sub(const subst& s = subst(std::less<resid>(), *alloc));

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
	typedef boost::interprocess::allocator<termid, boost::interprocess::managed_mapped_file::segment_manager> tidalloc;
	typedef boost::container::vector<termid, tidalloc> termset;
	class ruleset {
	public:
		typedef boost::interprocess::allocator<termset, boost::interprocess::managed_mapped_file::segment_manager> tsalloc;
		typedef boost::container::vector<termset, tsalloc> btype;
	private:
		termset _head = termset(*alloc);// = *segment->construct<termset>("head")(*alloc);
		btype  _body = btype(*alloc);
		size_t m = 0;
	public:
		prover* p;
		ruleset(prover* _p) : p(_p) {}
		ruleid add(termid t, const termset& ts);
		ruleid add(termid t);
		const termset& head() const	{ return _head; }
		const btype& body() const	{ return _body; }
		size_t size()			{ return _head.size(); }
		void mark();
		void revert();
		typedef tidalloc ralloc;
		typedef boost::container::list<ruleid, ralloc> rlbase;
		class rulelist : public rlbase {
		public:
			rulelist() : rlbase(*alloc) {}
			using rlbase::rlbase;
		};
		typedef boost::interprocess::allocator<std::pair<const resid, rulelist>, boost::interprocess::managed_mapped_file::segment_manager> r2alloc;
		typedef boost::container::map<resid, rulelist, std::less<resid>, r2alloc> r2id_t;
		string format() const;
		inline const rulelist& operator[](resid id) const {
			static rulelist empty(*alloc);
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
		r2id_t r2id = r2id_t(std::less<resid>(), *alloc);
	} kb;
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	void operator()(termset& goal, shared_ptr<subst> s = 0);
	void operator()(const qdb& goal, shared_ptr<subst> s = 0);
	const term& get(termid) const;
	const term& get(resid) const { throw std::runtime_error("called get(termid) with resid"); }
	~prover();

	typedef boost::container::list<std::pair<ruleid, shared_ptr<subst>>> ground;
	typedef boost::container::map<resid, boost::container::set<std::pair<termid, shared_ptr<ground>>>> evidence;
	evidence e;
	std::vector<subst> substs;
	termid tmpvar();
	void printg(const ground& g);
	void printg(shared_ptr<ground> g) { printg(*g); }
	void printe();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(resid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	string format(const termset& l, bool json = false);
	void prints(const subst& s);
	void prints(shared_ptr<subst> s) { prints(*s); }

	struct proof {
		ruleid rul;
		uint last;
		shared_ptr<proof> prev;
		shared_ptr<subst> s;
		shared_ptr<ground> g;
		proof() : rul(0), prev(0), s(sub()), g(std::make_shared<ground>()) {}
		proof(ruleid r, uint l = 0, shared_ptr<proof> p = 0, shared_ptr<subst> _s = sub(), shared_ptr<ground> _g = std::make_shared<ground>() ) 
			: rul(r), last(l), prev(make_shared<proof>(*p)), s(_s), g(_g) {}
		proof(const proof& p) : rul(p.rul), last(p.last), prev(p.prev ? make_shared<proof>(*p.prev) : 0), s(p.s), g(p.g) {}
	};
	typedef std::deque<std::future<shared_ptr<proof>>> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	string format(termid id, bool json = false);
	void get_dotstyle_list(termid, std::list<resid>&);
	string formatkb(bool json = false);

private:

	class termdb {
		typedef boost::interprocess::allocator<term, boost::interprocess::managed_mapped_file::segment_manager> talloc;
		typedef boost::container::vector<term, talloc> terms_t;
		terms_t terms = terms_t(*alloc);
	public:
		typedef boost::container::list<termid> termlist;
		typedef boost::container::map<resid, termlist> p2id_t;
		size_t size() const { return terms.size(); }
		inline const term& operator[](termid id) const { return terms.at(id); }
		inline const termlist& operator[](resid id) const { return p2id.at(id); }
		inline termid add(resid p, termid s, termid o) { terms.emplace_back(p, s, o); termid r = size(); p2id[p].push_back(r); return r; }
	private:
		p2id_t p2id;
	} _terms;
	friend ruleset;
//	qdb quads;
	int steps = 0;

	void step (shared_ptr<proof>, queue_t&, bool del = true);
	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	inline termid evaluate(termid id, shared_ptr<subst> s) { return evaluate(id, *s); }
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	inline bool unify(termid _s, shared_ptr<subst> ssub, termid _d, shared_ptr<subst> dsub, bool f) {
		return unify(_s, *ssub, _d, *dsub, f);
	}
	bool euler_path(shared_ptr<proof>);
	int builtin(termid id, shared_ptr<proof> p, queue_t& queue);
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
	void printp(proof* p);
	void printp(shared_ptr<proof> p) { printp(&*p); }
	string formats(const subst& s, bool json = false);
	string formats(shared_ptr<subst>& s, bool json = false) { return formats(*s, json); }
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
