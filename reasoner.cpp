/*
    euler.cpp

    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include "reasoner.h"

bidict dict;
predicate *predicates = new predicate[max_predicates];
rule *rules = new rule[max_rules];
frame *frames = new frame[max_frames];
uint npredicates = 0, nrules = 0, nframes = 0;
const string implication = "http://www.w3.org/2000/10/swap/log#implies";

rule& rule::init ( predicate* h, predlist b ) {
	head = h;
	body = b;
	return *this;
}

int builtin ( predicate* p ) {
	if ( p && p->pred == dict["GND"] ) return 1;
	return -1;
}

frame& frame::init ( const frame& f ) {
	if ( nframes >= max_frames ) throw "Buffer overflow";
	if ( !f.parent ) return init ( f.rul, f.src, f.ind, 0, f.substitution, f.ground );
	return init ( f.rul, f.src, f.ind, &frames[nframes++].init ( *f.parent ), f.substitution, f.ground );
}

frame& frame::init ( rule* _r, uint _src, uint _ind, frame* p, subst _s, ground_t _g ) {
	if ( nframes >= max_frames ) throw "Buffer overflow";
	rul = _r;
	src = _src;
	ind = _ind;
	parent = p;
	substitution = _s;
	ground = _g;
	return *this;
}

void printkb() {
	static bool pause = false;
	cout << endl << "dumping kb with " << npredicates << " predicates, " << nrules << " rules and " << nframes << " frames. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "rules: " << endl;
	for ( uint n = 0; n < nrules; ++n ) cout << rules[n] << endl;
	cout << "frames: " << endl;
	for ( uint n = 0; n < nframes; ++n ) cout << frames[n] << endl;
	if ( pause ) cout << "type <enter> to continue or <c><enter> to stop pausing...";
	cout << endl;
	if ( pause && getchar() == 'c' ) pause = false;
}

predicate* evaluate ( predicate& t, const subst& sub ) {
	if ( t.pred < 0 ) {
		auto it = sub.find ( t.pred );
		return it == sub.end() ? 0 : evaluate ( *it->second, sub );
	}
	if ( t.args.empty() ) return &t;
	predicate *p, *r;
	r = &predicates[npredicates++].init ( t.pred );
	for ( auto x : t.args ) r->args.emplace_back ( ( p = evaluate ( *x, sub ) ) ? p : &predicates[npredicates++].init ( x->pred ) );
	//	cout << "eval ( " << t << '/' << sub << " ) returned ";
	//	if (!r) cout << "(null)" << endl;
	//	else cout << *r << endl;
	return r;
}

bool unify ( predicate& s, const subst& ssub, predicate& d, subst& dsub, bool f ) {
	predicate* p;
	if ( s.pred < 0 ) return ( p = evaluate ( s, ssub ) ) ? unify ( *p, ssub, d, dsub, f ) : true;
	if ( d.pred >= 0 )
		return ( s.pred == d.pred ) && equal ( s.args.begin(), s.args.end(), d.args.begin(), d.args.end(), [&ssub, &dsub, &f] ( predicate * p1, predicate * p2 ) {
		return unify ( *p1, ssub, *p2, dsub, f );
	} );
	p = evaluate ( d, dsub );
	if ( ( p = evaluate ( d, dsub ) ) ) return unify ( s, ssub, *p, dsub, f );
	if ( f ) dsub[d.pred] = evaluate ( s, ssub );
	return true;
}

predlist to_predlist ( const ground_t& g ) {
	//typedef list<pair<rule*, subst>> gnd;
	predlist r;
	for ( auto x : g ) r.push_back ( x.first->head );
	return r;
}

evidence_t proof_done ( const frame& current_frame ) {
	evidence_t evidence;
	for ( auto x : current_frame.rul->body ) {
		auto t = evaluate ( *x, current_frame.substitution );
		if ( evidence.find ( t->pred ) == evidence.end() ) evidence[t->pred] = {};
		evidence[t->pred].emplace_front ( &rules[nrules++].init ( t, {&predicates[npredicates++].init ( dict["GND"], to_predlist ( current_frame.ground ) ) } ) , subst() );
	}
	return evidence;
}

frame* next_frame ( const frame& current_frame, ground_t& g ) {
	if ( !current_frame.rul->body.empty() ) g.emplace_front ( current_frame.rul, current_frame.substitution );
	frame& new_frame = frames[nframes++].init ( *current_frame.parent );
	new_frame.ground = ground_t();
	unify ( *current_frame.rul->head, current_frame.substitution, *new_frame.rul->body[new_frame.ind], new_frame.substitution, true );
	new_frame.ind++;
	return &new_frame;
}

frame* match_cases ( frame& current_frame, predicate& t, cases_t& cases ) {
	trace ( "looking for " << t << " in cases: " << endl << cases << endl << "end cases" << endl );
	if ( cases.find ( t.pred ) == cases.end() /* || cases[t->pred].empty()*/ ) return 0;

	uint src = 0;
	for ( rule* _rl : cases[t.pred] ) {
		rule& rl = *_rl;
		trace ( "trying to unify rule " << rl << " from cases against " << t << "... " );
		src++;
		ground_t ground = current_frame.ground;
		if ( rl.body.empty() ) ground.emplace_back ( &rl, subst() );
		frame& candidate_frame = frames[nframes++].init ( &rl, src, 0, &current_frame, subst(), ground );
		if ( rl.head && unify ( t, current_frame.substitution, *rl.head, candidate_frame.substitution, true ) ) {
			trace ( "unified!" << endl );
			frame& ep = current_frame;
			while ( ep.parent ) {
				ep = *ep.parent;
				if ( ( ep.src == current_frame.src ) &&
				        unify ( *ep.rul->head, ep.substitution, *current_frame.rul->head, current_frame.substitution, false ) )
					break;
			}
			if ( !ep.parent ) {
				//	cout << "pushing frame: " << candidate_frame << endl;
				return &candidate_frame;
			}
		} else
			trace ( "unification failed" << endl );
	}
	return 0;
}

evidence_t prove ( rule* goal, int max_steps, cases_t& cases ) {
	deque<frame*> queue;
	queue.emplace_back ( &frames[nframes++].init ( goal ) );
	uint step = 0;

	cout << "goal: " << *goal << endl;
	while ( !queue.empty() && ++step ) {
		frame& current_frame = *queue.front();
		queue.pop_front();
		ground_t g = current_frame.ground;
		if ( max_steps != -1 && ( int ) step >= max_steps ) return evidence_t();
		if ( current_frame.ind >= current_frame.rul->body.size() ) {
			if ( !current_frame.parent ) return proof_done ( current_frame );
			else queue.push_back ( next_frame ( current_frame, g ) );
		} else {
			predicate* t = current_frame.rul->body[current_frame.ind];
			int b = builtin ( t ); // ( t, c );
			if ( b == 1 ) {
				g.emplace_back ( &rules[nrules++].init ( evaluate ( *t, current_frame.substitution ) ), subst() );
				frame& r = frames[nframes++].init ( current_frame );
				r.ground = ground_t();
				r.ind++;
				queue.push_back ( &r );
			} else if ( b ) if ( frame* f = match_cases ( current_frame, *t, cases ) ) queue.push_front ( f );
		}
	}
	return evidence_t();
}

predicate* mkpred ( string s, const vector<predicate*>& v = vector<predicate*>() ) {
	uint p;
	p = dict.has ( s ) ? dict[s] : dict.set ( s );
	return &predicates[npredicates++].init ( p, v );
}

rule* mkrule ( predicate* p = 0, const vector<predicate*>& v = vector<predicate*>() ) {
	return &rules[nrules++].init ( p, v );
}

predicate* triple ( const string& s, const string& p, const string& o ) {
	return mkpred ( p, { mkpred ( s ), mkpred ( o ) } );
};

predicate* triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
};

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

/*
    10:05 < HMC_A_> if X=>Y
    10:05 < HMC_A_> then for each statement in Y make a rule
    10:05 < HMC_A_> of X=>Yi for Yi in Y
    10:05 < HMC_A_> where X is all the statements in X
    10:06 < HMC_A_> the body (X) is a vector of statements (triples)
    10:06 < HMC_A_> each head (Yi) is only one statement (triple)
    10:06 < HMC_A_> conjunction, disjunction!
    10:06 < HMC_A_> body is conjuncted
    10:06 < HMC_A_> head is disjuncted
*/
evidence_t prove ( const qlist& kb, const qlist& query ) {
	evidence_t evidence;
	cases_t cases;
	for ( auto quad : kb ) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		dict.set ( s );
		dict.set ( o );
		dict.set ( p );
		if ( p != implication ) cases[dict[p]].push_back ( mkrule ( 0, {triple ( p, s, o ) } ) );
		else for ( const auto& y : kb )
				if ( y->graph->value == o ) {
					rule& rul = *mkrule();
					rul.head = triple ( *y );
					for ( const auto& z : kb )
						if ( z->graph->value == s )
							rul.body.push_back ( triple ( *z ) );
					cases[dict[p]].push_back ( &rul );
				}
	}

	rule& goal = *mkrule();
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
	return prove ( &goal, -1, cases );
}

bool test_reasoner() {
	dict.set ( "a" );
	dict.set ( "GND" );
	evidence_t evidence;
	cases_t cases;
	typedef predicate* ppredicate;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Mortal} ), { mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Man   } ), { mkpred ( "a", {_x, Male} )  } ) );

	cout << "cases:" << endl << cases << endl;
	printkb();
	predicate* goal = mkpred ( "a", { _y, Mortal } );
	evidence = prove ( mkrule ( goal, { goal } ), -1, cases );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	cout << evidence << endl;
	cout << "QED!" << endl;
	cout << evidence.size() << endl;
	return evidence.size();
}
