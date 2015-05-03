/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#ifndef __REASONER_H__
#define __REASONER_H__

#include <forward_list>
#include <deque>
#include <climits>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <vector>
#include <cstdio>
//#include "parsers.h"
#include "misc.h"
#include "rdf.h"

using namespace std;
using namespace jsonld;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = M1, max_rules = M1, max_frames = M1;

typedef vector<const struct predicate*> predlist;
typedef vector<const struct rule*> rulelist;
ostream& operator<< ( ostream& o, const rulelist& l );
ostream& operator<< ( ostream& o, const predlist& l );

struct predicate {
	int pred = 0;
	predlist args;
	predicate& init ( int _p = 0, predlist _args = predlist() );
	const predicate* evaluate ( const map<int, const predicate*>& sub ) const;
};

struct rule {
	const predicate* head = 0;
	predlist body;
	rule& init ( const predicate* h, predlist b = predlist() );
};

typedef map<int, const predicate*> subst;
typedef list<pair<const rule*, subst>> ground_t;
typedef map<int, list<pair<const predicate*, ground_t>>> evidence_t;
typedef map<int, list<const rule*>> cases_t;

struct frame {
	const rule* rul = 0;
	uint ind = 0;
	const frame* parent = 0;
	subst substitution;
	ground_t ground;
	static frame& init ( class reasoner* r, const frame& f );
	static frame& init ( class reasoner*, const rule* _r = 0, uint _ind = 0, const frame* p = 0, subst _s = subst(), ground_t _g = ground_t() );
};

class reasoner {
	friend struct frame;
	predicate *predicates = new predicate[max_predicates];
	rule *rules = new rule[max_rules];
	frame *frames = new frame[max_frames];
	uint npredicates = 0, nrules = 0, nframes = 0;
	predicate* GND;
	predlist to_predlist ( const ground_t& g );

	int 			builtin 	( const predicate* t, const frame& f );
	const predicate* 	unify 		( const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f );
	void 			evidence_found 	( const frame& current_frame, evidence_t& evidence );
	frame* 			next_frame 	( const frame& current_frame );
	const frame* 		match_rule 	( const frame& current_frame, const predicate& t, const rule& rl );
	deque<const frame*> 	process_frame 	( const frame& current_frame, const cases_t& cases, evidence_t& evidence );
	deque<const frame*> 	init		( const rule* goal );

	predicate* mkpred ( string s, const vector<const predicate*>& v = vector<const predicate*>() );
	rule* mkrule ( const predicate* p = 0, const vector<const predicate*>& v = vector<const predicate*>() );
	const predicate* triple ( const string& s, const string& p, const string& o );
	const predicate* triple ( const jsonld::quad& q );
public:
	//reasoner(size_t _npreds = max_predicates, size_t _nrules = max_rules, size_t _nframes = max_frames);
	reasoner();
	~reasoner();
	evidence_t prove ( const rule* goal, int maxNumberOfSteps, const cases_t& cases );
	evidence_t prove ( const qdb& kb, const qlist& query );
	bool test_reasoner();
	void printkb();
};

ostream& operator<< ( ostream& o, const subst& s );
ostream& operator<< ( ostream& o, const ground_t& s );
ostream& operator<< ( ostream& o, const evidence_t& e );
ostream& operator<< ( ostream& o, const cases_t& e );
ostream& operator<< ( ostream& o, const rule& r );
ostream& operator<< ( ostream& o, const predicate& p );
ostream& operator<< ( ostream& o, const frame& f );

#endif
