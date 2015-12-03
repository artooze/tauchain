#include <functional>
#include <unordered_map>
#include <queue>
#include <limits>
#include <string.h>

#include "univar.h"

using namespace std;

typedef intptr_t offset_t;//ptrdiff_t?
typedef unsigned char byte;

intptr_t ofst(size_t target, size_t me)
{
	assert(target < numeric_limits<int>::max());
	assert(me < numeric_limits<int>::max());
	return (int)target - (int)me;
}

unsigned long kbdbg_part;
unsigned long kbdbg_part_max;

extern int result_limit ;

#define EEE char entry = 0
#define TRCEEE TRACE(dout << "entry = " << (int)entry << endl)
const char LAST = 33; // last case statement (in release mode), not last entry, the coro might still be invoked with 33 several itmes before returning false

#ifdef DEBUG

#define DBG(x) x
#define TRC(x) x
#define TRCCAP(x) ,x
#define ITEM(x,y) x.at(y)
#define ASSERT assert

#define case_LAST case LAST
#define DONE {entry = 66; return false; }
#define END DONE; default: assert(false);

#else

#define DBG(x)
#define TRC(x)
#define TRCCAP(x)
#define ITEM(x,y) x[y]
#define ASSERT(x)

#define case_LAST default
#define DONE return false
#define END DONE;

#endif



#ifdef NEW
#define KBDBG
#endif

#ifdef KBDBG
#ifdef oneword
nope
#endif
#ifndef DEBUG
nope
#endif
#define IFKBDBG(x) x
#else
#define IFKBDBG(x)
#endif


//for kbdbg
typedef vector<unsigned long> Markup;
typedef std::pair<termid, Markup> toadd;


enum ThingType {BOUND, NODE, OFFSET, LIST, LIST_BNODE, UNBOUND};

#ifndef oneword

/*so, the idea behind this new data structuring is that each little thing doesnt have to be allocated separately,
we can put them in one big array per rule, and we can initialize such locals array from a template simply by memcpy
this is just an attempt at optimization, it isnt a change needed for the correct functioning

	UNBOUND, 	// unbound var
	BOUND,  	// bound var
	NODE, 		// nodeid - atom
	LIST, 		// has size, is followed by its items
	OFFSET		// pointer offset to another item in the vector

so, a rule needs local variables shared among its body joins.
these will be in one contiguous vector of things
a link is an offset, it points to another thing within the vector
its like a rule-local var
the reason for this is that vars may occur multiple times, for example:
{(?z ?y) a b. (?y ?x) a b} => ...
we have two distinct lists in this rule, each contains ?y
the lists themselves will be just consecutive things in the locals vector
*/

class Thing {
public:
	ThingType type;
	union {
		//maybe call this value? how about binding?
		Thing *thing;     // for bound var
		nodeid node;      // for node
		size_t size;      // for list
		offset_t offset;
	};
#ifdef KBDBG
	Markup markup;
#endif
};


//we could add some assertions here maybe
//this abstracts away oneword and struct implementations of Thing
#define get_type(thing) ((thing).type)
#define get_node(thing) (ASSERT(get_type(thing) == NODE), /*then return*/ (thing).term)
#define get_size(thing) ((thing).size)
#define get_thing(ttttt) ((ttttt).thing)
#define get_offset(thing) ((thing).offset)

//these make sense
#define is_offset(thing)	(get_type(thing) == OFFSET)
#define is_unbound(thing)	(get_type(thing) == UNBOUND)
#define is_bound(thing)		(get_type(thing) == BOUND)
#define is_var(thing)		(get_type(thing) == BOUND || get_type(thing) == UNBOUND)
#define is_list(thing)		(get_type(thing) == LIST)
#define is_list_bnode(thing)(get_type(thing) == LIST_BNODE)
#define is_node(thing)		(get_type(thing) == NODE)
#define types_differ(x, y) (x.type != y.type)
#define sizes_differ(x, y) (x.size != y.size)
#define are_equal(x, y) (x.type == y.type && x.size == y.size)

inline void make_this_bound(Thing *me, Thing *val)
{
	me->type = BOUND;
	me->thing = val;
}

inline Thing create_unbound()
{
	Thing x;
	x.type = UNBOUND;
	DBG(x.thing = (Thing *)666;)
	return x;
}

inline Thing create_node(termid v)
{
	Thing x;
	x.type = NODE;
	x.term = v;
	return x;
}

inline void make_this_unbound(Thing * me)
{
	me->type = UNBOUND;
}

void make_this_offset(Thing &t, offset_t o) {
	t.type = OFFSET;
	t.offset = o;
}

void make_this_list(Thing &i0, size_t size)
{
	i0.type = LIST;
	i0.size = size;
}

#else

/* oneword:
kinda like http://software-lab.de/doc/ref.html#cell but with bits in the pointees

 00 = var        // address left intact or zero
 01 = node       // we mostly just compare these anyway
010 = positive offset
110 = negative offset
 11 = list(size)
*/

typedef uintptr_t *Thing; // unsigned int that is capable of storing a pointer
#define databits(x) (((uintptr_t)x) & ~0b11)
#define typebits(t) ((uintptr_t)t & (uintptr_t)0b11)
static_assert(sizeof(uintptr_t) == sizeof(size_t), "damn");


static inline void make_this_bound(Thing * me, Thing * v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	*me = (Thing)v;
}

static inline Thing create_unbound()
{
	return 0;
}

static inline Thing create_node(termid v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	return (Thing)((uintptr_t)v | 0b01);
}

static inline void make_this_unbound(Thing * me)
{
	*me = 0;
}

void make_this_offset(Thing &t, offset_t o) {
	byte sign = 0;
	if (o < 0)
	{
		sign = 1;
		o = -o;
	}
	uintptr_t r = o;
	r = r << 1;
	r |= sign;
	r = r << 2;
	r |= 0b10;
	t = (Thing)r;
}

void make_this_list(Thing &i0, size_t size)
{
	i0 = (Thing)((size << 2) | 0b11);
}

static inline offset_t get_offset(Thing x)
{
	uintptr_t xx = (uintptr_t)x;
	byte sign = (xx >> 1) & 0b10;
	offset_t val = xx >> 3;
	ASSERT(val);
	val -= (sign * val);
	return val;
}

static inline Thing* get_thing(Thing x)
{
	return (Thing*)x;
}

static inline termid get_term(Thing x)
{
	return (termid)databits(x);
}

static inline size_t get_size(Thing x)
{
	return (size_t)((uintptr_t)x >> 2);
}

static inline bool types_differ(Thing a, Thing b)
{
	return (((long)a ^ (long)b) & 0b11);
}

#define sizes_differ(x, y) (x != y)

static inline bool is_bound(Thing x)
{
	return ((((uintptr_t)x & (uintptr_t)0b11) == 0) && x);
}

static inline bool is_unbound(Thing x)
{
	return x == 0;
}

#define is_var(thing) 		(typebits(thing) 	== 0b00)
#define is_node(thing) 		(typebits(thing) 	== 0b01)
#define is_offset(thing) 	(typebits(thing) 	== 0b10)
#define is_list(thing) 		(typebits(thing) 	== 0b11)

#define are_equal(x, y) (x == y)

static inline ThingType get_type(Thing x)
{
	if(is_bound(x))
		return BOUND;
	if(is_unbound(x))
		return UNBOUND;
	if(is_node(x))
		return NODE;
	if(is_list_bnode(x))
		return LIST_BNODE;
	if(is_list(x))
		return LIST;
	ASSERT(is_offset(x));
	return OFFSET;
}

//</oneword>
#endif //ndef oneword


inline void add_kbdbg_info(Thing &t, Markup markup)
{
	(void) t;
	(void) markup;
#ifdef KBDBG
	t.markup = markup;
#endif
}



//region types, globals, funcs


typedef vector<Thing> Locals;

typedef std::pair<Thing*,Thing*> thingthingpair;
typedef std::vector<thingthingpair> ep_t;

typedef function<bool()> coro;
typedef function<bool(Thing*,Thing*)> pred_t;
typedef function<pred_t()> pred_gen;
typedef function<bool(Thing*,Thing*)> rule_t;
typedef function<rule_t()> rule_gen;
typedef function<bool(Thing*,Thing*, Thing*)> join_t;
typedef function<join_t()> join_gen;
//btw im using gen in the sense that its a lambda generating another lambda
typedef function<join_gen(nodeid, join_gen, size_t, size_t, Locals&)>  join_gen_gen;

enum PredParam {HEAD_S, HEAD_O, LOCAL, CONST};
typedef map<PredParam, map<PredParam, join_gen_gen>> Perms;

map<nodeid, vector<pred_t>> builtins;
map<string, string> log_outputString;

typedef function<pred_t(nodeid)> wildcard_pred_t;
wildcard_pred_t wildcard_pred;

typedef function<bool(Thing*,nodeid, Thing*)> wildcard_t;
wildcard_t wildcard;



Perms perms;
map<PredParam, string> permname;
map<nodeid, vector<size_t>> pred_index;
#ifdef DEBUG
std::map<nodeid, pred_t> preds;
typedef map<termid, size_t> locals_map;
#else
std::unordered_map<nodeid, pred_t> preds;
typedef unordered_map<termid, size_t> locals_map;
#endif



typedef map<nodeid, vector<pair<Thing, Thing>>> ths_t;

std::vector<ep_t*> eps_garbage;
vector<Locals*> consts_garbage;
vector<Locals*> locals_templates_garbage;
ths_t * ths_garbage;

//counters
long steps = 0;
long unifys = 0;


//some forward declarations
coro unbound_succeed(Thing *x, Thing *y, Thing * origa, Thing * origb);
coro unify(Thing *, Thing *);
void check_pred(nodeid pr);
rule_t seq(rule_t a, rule_t b);
rule_t compile_rule(prover::ruleid r);
void build_in_rules();
void build_in_facts();

//endregion



//region succeed and fail



//yield once
static coro gen_succeed()
{
	EEE;
	return [entry]() mutable{
		switch(entry)
		{
		case 0:
			entry = LAST;
			return true;
		case_LAST:
			END
		}
	};
}

static join_t succeed_with_args()
{
	EEE;
	return [entry](Thing *Ds, Thing *Do, Thing* _) mutable{
		(void)Ds;
		(void)Do;
		(void)_;
		switch(entry)
		{
		case 0:
			entry = LAST;
			return true;
		case_LAST:
			END
		}
		/*
		Macros resolve to:
		case 0:
			entry = 33;
			return true;
		case 33:
			{entry = 66; return false}; default: assert(false); 

		*/
	};
}

static join_gen succeed_with_args_gen()
{
	return []() {
		return succeed_with_args();
	};
}

#ifndef DEBUG

static bool fail()
{
	setproc("fail");
	TRACE(dout << "..." << endl;)
	return false;
}

static bool fail_with_args(Thing *_s, Thing *_o)
{
	(void)_s;
	(void)_o;
	setproc("fail_with_args");
	TRACE(dout << "..." << endl;)
	return false;
}

#define GEN_FAIL (fail)
#define GEN_FAIL_WITH_ARGS (fail_with_args)
#define UNIFY_FAIL(a,b) GEN_FAIL
#else

coro dbg_fail()
{
	byte entry = 0;
	return [entry]() mutable{
		setproc("dbg_fail lambda");
		TRCEEE;

		switch(entry)
		{
		case 0:
			entry = 66;
			return false;
		default:
			ASSERT(false);
		}
	};
}

pred_t dbg_fail_with_args()
{
	int entry = 0;
	return [entry](Thing *_s, Thing *_o) mutable{
		setproc("dbg_fail_with_args lambda");
		TRCEEE;

		(void)_s;
		(void)_o;

		switch(entry)
		{
		case 0:
			entry = 1;
			return false;
		default:
			ASSERT(false);
		}
	};
}

#define GEN_FAIL (dbg_fail())
#define GEN_FAIL_WITH_ARGS (dbg_fail_with_args())



#ifdef KBDBG

string kbdbg_str(const Thing * x)
{
	stringstream o;
	o << "[" << "\"" << x << "\""  << ", ";
	size_t c=0;
	for (auto i: x->markup) {
		o << i;
		if (++c != x->markup.size())
			o << ", ";
	}
	o << "]";
	//assert(c);
	return o.str();
}

coro kbdbg_unify_fail(const Thing *a, const Thing *b)
{
	int entry = 0;
	return [entry, a, b]() mutable{
		switch(entry)
		{
		case 0:
			entry = 1;
			dout << "{\"type\":\"fail\", \"a\":" << kbdbg_str(a) << ", \"b\":" << kbdbg_str(b) << "}" << endl;
			return false;
		default:
			ASSERT(false);
		}
	};
}

#define UNIFY_FAIL(a,b) kbdbg_unify_fail(a,b)
#else
#define UNIFY_FAIL(a,b) GEN_FAIL


#endif // kbdbg
#endif // debug


inline void kbdbg_bind(const Thing *a, bool bind, const Thing *b)
{
	(void)a;
	(void)b;
	(void)bind;
#ifdef KBDBG
	dout << "{\"type\":\"";
	if(!bind) dout << "un";
	dout << "bind\", \"a\":" << kbdbg_str(a) << ", \"b\":" << kbdbg_str(b) << "}" << endl;
#endif
}


#ifndef KBDBG

#define UNIFY_SUCCEED(a,b) gen_succeed()

#else

static coro UNIFY_SUCCEED(const Thing *a, const Thing *b)
{
	EEE;
	return [entry, a ,b]() mutable{
		switch(entry)
		{
		case 0:
			entry = LAST;
			kbdbg_bind(a, true, b);
			return true;
		case_LAST:
			kbdbg_bind(a, false, b);
			entry = 66;
			return false;
		default:
			assert(false);
		}
	};
}


#endif


//endregion




string str(const Thing *_x)
{
	Thing x = *_x;
	switch (get_type(x)) {
		case BOUND: {
			const Thing *thing = get_thing(x);
			ASSERT(thing);
			return "var(" + str(thing) + ")";
		}
		case UNBOUND:
			return "var()";
		case NODE: {
			const nodeid node = get_node(x);
			ASSERT(node);
			return *dict[node->p].value;
		}
		case LIST_BNODE: {
			const nodeid node = get_node(x);
			ASSERT(node);
			return *dict[node->p].value;
		}
		case LIST: {
			const size_t size = get_size(x);
			stringstream r;
			r << "{" << size << " items}(";
			for (size_t i = 0; i < size; i++) {
				if (i != 0) r << " ";
				r << str(_x + 1 + (i*2));
			}
			if (!size)
				r << " ";
			return r.str() + ")";
		}
		case OFFSET: {
			const offset_t offset = get_offset(x);
			stringstream r;
			r << "<offset ";
			if (offset >= 0)
				r << "+";
			r << offset << ">->";
			r << str(_x + offset);
			return r.str();
		}
	}
	ASSERT(false);
}

static Thing *getValue (Thing *_x) __attribute__ ((pure));
static Thing *getValue (Thing *_x)
	/*
		# If this Variable is unbound, then just return this Variable.^M
		# Otherwise, if this has been bound to a value with unify, return the value.^M
		# If the bound value is another Variable, this follows the "variable chain"^M
		# to the end and returns the final value, or the final Variable if it is unbound.^M
		# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html^M
		def getValue(self):^M
			if not self._isBound:^M
				return self^M

			result = self._value^M
			while isinstance(result, Variable):^M
				if not result._isBound:^M
					return result^M

				# Keep following the Variable chain.^M
				result = result._value^M

			return result^M
	*/
{
	ASSERT(_x);
	Thing x = *_x;

	if (is_bound(x)) {
		Thing * thing = get_thing(x);
		ASSERT(thing);
		return getValue(thing);
	}
	//Need to understand this whole offset thing better
	else if (is_offset(x))
	{
		const offset_t offset = get_offset(x);
		Thing * z = _x + offset;
		make_this_bound(_x, z);
		return getValue(z);
	}
	else
		return _x;
}






/*  # If this Variable is bound, then just call YP.unify to unify this with arg.
	# (Note that if arg is an unbound Variable, then YP.unify will bind it to
	# this Variable's value.)
	# Otherwise, bind this Variable to YP.getValue(arg) and yield once.  After the
	# yield, return this Variable to the unbound state.
	# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html */
function<bool()> unboundunifycoro(Thing * me, Thing *arg
#ifdef DEBUG
, Thing * origa, Thing * origb
#endif
)
{
		FUN;
		TRACE(dout << "!Bound" << endl;)
		Thing *argv = getValue(arg);
		TRACE(dout << "unify with [" << argv << "]" << str(argv) << endl;)

		if (argv == me) {
			TRACE(dout << "argv == me" << endl;)
			//# We are unifying this unbound variable with itself, so leave it unbound.^M
#ifdef DEBUG
			return unbound_succeed(me, argv, origa, origb);
#else
			return gen_succeed();
#endif
		}
		else {
			TRACE(dout << "!= me" << endl;)

			EEE;
			return [me, entry, argv
			#ifdef DEBUG
			, origa, origb
			#endif
			]() mutable {
				setproc("var unify lambda");
				TRCEEE;
				switch (entry) {
					case 0: {
						TRACE(dout << "binding [" << me << "]" << str(me) << " to [" << argv << "]" << str(argv) << endl;)
						ASSERT(is_unbound(*me));
						make_this_bound(me, argv);
						#ifdef KBDBG
						kbdbg_bind(origa, true, origb);
						#endif
						entry = LAST;
						return true;
					}
					case_LAST:
						TRACE(dout << "unbinding [" << me << "]" << str(me) << " from [" << argv << "]" << str(argv) << endl;)
						ASSERT(is_bound(*me));
						make_this_unbound(me);
						#ifdef KBDBG
						kbdbg_bind(origa, false, origb);
						#endif
						END
				}
			};
		}
	}


bool would_unify(Thing *this_, Thing *x_)
/*ep check is supposed to determine equality by seeing if the values would unify
* (but not actually doing the unifying assignment)*/
{
		FUN;
		const Thing me = *this_;
		const Thing x = *x_;
		ASSERT(!is_offset(me));
		ASSERT(!is_offset(x));
		ASSERT(!is_bound(x));
	//return type_bits(me) &&
		if (is_var(me))
			return true;// we must have been an unbound var
		else if (types_differ(me, x)) // in oneword mode doesnt differentiate between bound and unbound!
			return false;
		else if(is_node(me))
		{
			/*const auto this_term = get_term(this);
			const auto x_term = get_term(x);
			TRACE(dout << op->format(this_term) << " =?= " << op->format(x_term) << endl;)*/
			bool r = are_equal(me, x);
			ASSERT(op->_terms.equals(get_term(me), get_term(x)) == r);
			return r;
		}
		else
		{
			ASSERT(is_list(me));
			const auto size = get_size(me);
			if(size != get_size(x)) return false;
			for (size_t i = 0; i < size; i++)
				if (!would_unify(getValue(this_+1+(i*2)) , getValue(x_+1+(i*2))))
					return false;
			return true;
		}
}



#ifdef DEBUG

coro unbound_succeed(Thing *x, Thing *y, Thing * origa, Thing * origb)
{
	EEE;
	return [entry, x, y, origa, origb]() mutable {
		ASSERT(is_unbound(*x));
		setproc("unbound_succeed lambda");
		TRACE(dout << str(x) << " " << str(y) << endl);
		TRCEEE;
		switch (entry) {
			case 0:
				entry = LAST;
				kbdbg_bind(origa,true,origb);
				return true;
			case_LAST:
				kbdbg_bind(origa,false,origb);
				entry = 66;
				return false;
			default:
				assert(false);
		}
	};
}
#endif



//region sprint

string sprintVar(string label, Thing *v){
	stringstream wss;
	wss << label << ": (" << v << ")" << str(v);
	return wss.str();
}

string sprintPred(string label, nodeid pred){
	stringstream wss;
	wss << label << ": (" << pred << ")" << dict[pred];
	return wss.str();
}

string sprintThing(string label, Thing *t){
	stringstream wss;
	wss << label << ": [" << t << "]" << str(t);
	return wss.str();
}

string sprintSrcDst(Thing *Ds, Thing *s, Thing *Do, Thing *o){
	stringstream wss;
	wss << sprintThing("Ds", Ds) << ", " << sprintThing("s",s) << endl;
	wss << sprintThing("Do", Do) << ", " << sprintThing("o",o);
	return wss.str();
}


//endregion



//region kb

void free_garbage()
{
	for (auto x: eps_garbage)
	{
		ASSERT(!x->size());
		delete x;
	}
	eps_garbage.clear();

	for (auto x: consts_garbage)
		delete x;
	consts_garbage.clear();
	for (auto x: locals_templates_garbage)
		delete x;
	locals_templates_garbage.clear();

}


void free_garbage_nonassert()
{
	for (auto x: eps_garbage)
		delete x;

}




void add_rule(nodeid pr, const rule_t &x)
{
	//If the nodeid is not already in preds then make this
	//rule_t (pred_t?) its pred function. 
	if (preds.find(pr) == preds.end())
		preds[pr] = x;
	//Otherwise, sequence this pred function with what's
	//already in preds.
	else {
		TRACE(dout << "seq, nodeid: " << pr << "(" << dict[pr] << ")" << endl;)
		preds[pr] = seq(x, preds[pr]);
	}
}

void compile_pred(nodeid pr)
//Check preds to see if the pred has already been compiled.
//Don't compile preds that have already been compiled. Return instead.
//Check builtins to see if the pred is a built-in. If we do find that
//node-id, then do add_rule for each pred_t in the associated
//std::vector<pred_t> in the builtins table. Return unless the
//builtin pred is 'rdftype'.

// What's up with that return?

//If it was rdftype or is not in the builtins table, then check to
//see if it's in pred_index. If it's not, then return, otherwise,
//get the list of rules in which this node is the predicate for the
//head. For each rule in the list, compile the rule and add it to the
//rule for that predicate using add_rule. 

//add_rule is simple, it just checks to see if the pred has already been 
//compiled, if not, then the rule_t we pass it becomes the rule_t for
//that pred. If the pred is already partially compiled, then the
//rule_t we pass it just gets sequenced with what's already there (using seq).
{
	FUN;

	if (preds.find(pr) != preds.end())
		return;

	if (builtins.find(pr) != builtins.end()) {
		for (auto b: builtins[pr]) {
			TRACE(dout << "builtin: " << dict[pr] << endl;)
			add_rule(pr, b);
		}
		//lets not shadow rdftype by builtins for now
		if (pr != rdftype)
			return;
	}

	if (pred_index.find(pr) == pred_index.end())
		return;

	//All the rules with that pred in the head
	vector<size_t> rs = pred_index.at(pr);
	TRACE(dout << "# of rules: " << rs.size() << endl;)
	//For each of those rules, compile it, and add the result to preds.
	for (auto r: rs)
		add_rule(pr, compile_rule(r));
}



void check_pred(nodeid pr)
{
	FUN;
	if (pred_index.find(pr) == pred_index.end() && builtins.find(pr) == builtins.end()) {
		dout << KRED << "Predicate '" << KNRM << dict[pr] << "' not found." << endl;
		preds[pr] = GEN_FAIL_WITH_ARGS;
	}
 }


void compile_kb()
{
	FUN;
	//TRACE(dout << "# of rules: " << op->heads.size() << endl;)

	//pred_index :	std::map<nodeid, vector<size_t>>
	pred_index.clear();

	//pred_t :	function<bool(Thing*,Thing*)>	
	//preds : 	std::map<old::nodeid, pred_t>
	preds.clear();

	//eps, locals_templates_garbage, consts_garbage
	free_garbage();

	//Loop over the heads starting from the end and going backwards.
	//Pred_index maps the nodeid for each unique pred to a list of rules
	//that use that pred, so at pred_index[pr] we'll add the index of the
	//rule (i.e. the index into heads/bodies).
	//old::prover->heads/bodies --> pred_index, we group rules by their pred
	for (int i = op->heads.size(); i > 0; i--)
	{
		nodeid pr = op->heads[i - 1]->p;
		TRACE(dout << "adding rule for pred [" << pr << "] " << dict[pr] << "'" << endl;)
		pred_index[pr].push_back(i - 1);
	}

	//pred_index --> preds (compilation step)

	for(auto x: builtins){
		compile_pred(x.first);
	}
	for(auto x: pred_index){
		compile_pred(x.first);
	}
}



//endregion











//region coros





coro unifjoin(Thing *a, Thing *b, coro c)
{
	FUN;
	TRACE(dout << "..." << endl;)
	EEE;
	coro uc;
	TRC(int call = 0;)
	return [a,b,c, uc, entry TRCCAP(call)]() mutable{
		setproc("unifjoin1");
		TRCEEE;
		TRC(call++;)

		switch(entry)
		{
		case 0:
			uc = unify(a,b);
			entry = LAST;
			while(uc()){
				ASSERT(call == 1);
				while(c()){
					ASSERT(call == 1);
					return true;
		case_LAST:;
					ASSERT(call == 2);
				};
			}
			END;
		}
	};
}



coro listunifycoro(Thing *a_, Thing *b_)
{
	FUN;

	const Thing a = *a_;
	const Thing b = *b_;

	//TRACE(dout << str(a_) << " X " << str(b_) << endl;)

	ASSERT(is_list(a));
	ASSERT(is_list(b));

	//gotta join up unifcoros of vars in the lists
	if(sizes_differ(a,b))
		return GEN_FAIL ;

	coro r = gen_succeed();

	for(int i = get_size(b)-1;i >= 0; i--)
	{
		r = unifjoin(a_+1+(i*2), b_+1+(i*2), r);
	}

	return r;
}
/*
	# If arg1 or arg2 is an object with a unify method (such as Variable or^M
	# Functor) then just call its unify with the other argument.  The object's^M
	# unify method will bind the values or check for equals as needed.^M
	# Otherwise, both arguments are "normal" (atomic) values so if they^M
	# are equal then succeed (yield once), else fail (don't yield).^M
	# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html^M
	(returns an iterator)
*/
coro unify(Thing *a_, Thing *b_){
	FUN;
	unifys++;
	DBG(auto origa = a_;)
	DBG(auto origb = b_;)
	TRACE(dout << str(a_) << " X " << str(b_) << endl;)

	if (a_ == b_) {//?
		TRACE(dout << "a == b" << endl;)
		return UNIFY_SUCCEED(origa, origb);
	}

	a_ = getValue(a_);
	Thing a = *a_;
	ASSERT(!is_bound(a));
	ASSERT(!is_offset(a));
	if (is_unbound(a))
		return unboundunifycoro(a_, b_
		#ifdef DEBUG
		,origa, origb
		#endif
		);

	b_ = getValue(b_);
	Thing b = *b_;
	ASSERT(!is_bound(b));
	ASSERT(!is_offset(b));
	if (is_unbound(b))
		return unboundunifycoro(b_, a_
		#ifdef DEBUG
		,origb, origa
		#endif
		);

	if(are_equal(a,b)) {
		if (is_node(a)) {
			ASSERT(op->_terms.equals(get_term(a), get_term(b)));
			return UNIFY_SUCCEED(origa, origb);
		}
		if (is_list(a)) {
			//TRACE(dout << "Both args are lists." << endl;)
			return listunifycoro(a_, b_);
		}
	}

	TRACE(dout << "Fail. origa:[" << origa << "] origb:[" << origb << "] a:["<< a_ << "]" << str(a_) << " b:[" << b_ << "]" << str(b_) << endl;)
	return UNIFY_FAIL(origa, origb);
}

rule_t seq(rule_t a, rule_t b){
	FUN;
	TRACE(dout << ".." << endl;)
	EEE;
	TRC(int call = 0;)
	return [a, b, entry TRCCAP(call)](Thing *Ds, Thing *Do) mutable{
		setproc("seq1");
		TRC(call++;)
		TRACE(dout << "call: " << call << endl;)
		switch(entry){
		case 0:
			entry = 1;
			while(a(Ds, Do)){
				TRACE(dout << "MATCH A." << endl;)
				return true;
		case 1: ;
			}
			entry = LAST;
			while(b(Ds, Do)){
				TRACE(dout << "MATCH B." << endl;)
				return true;
		case_LAST: ;
			}
			TRACE(dout << "SWITCH DONE." << endl;)
			END
		}
		TRACE(dout << "Why are we here?" << endl;)
		ASSERT(false);
	};
}


/*
one join function, joins "just pass" query s and o down between them,
each join calls one pred,
so we are only concerned with permuting the two params to the pred,
and these can be either: s, o, or a local var, or a const
a join captures two indexes into the locals/consts table, which it may or may not use
*/


#include "perms.cpp"




bool islist(termid t)
{
	ASSERT(t);/*
	dout << t << endl;
	dout << t->p << endl;
	dout << &dict << endl;
	dout << " " << dict[t->p].value  << endl;*/
	return *dict[t->p].value == ".";
}

PredParam maybe_head(PredParam pp, termid head, termid x)
{
#ifndef KBDBG
	if (head) {
		assert(head->s && head->o);
		if (x == head->s)
			return HEAD_S;
		if (x == head->o)
			return HEAD_O;
	}
#endif
	return pp;
}

//return term's PredParam and possibly also its index into the corresponding vector
PredParam find_thing (termid x, size_t &index, locals_map &lm, locals_map &cm)
{
	auto it = lm.find(x);
	if (it != lm.end()) {
		index = it->second;
		return LOCAL;
	}
	else {
		index = cm.at(x);
		return CONST;
	}
}

#ifdef KBDBG
PredParam kbdbg_find_thing (unsigned long part, size_t &index, Locals &locals)
{
	size_t r = 0;
	for(auto i: locals)
	{
		if (i.markup.size() == 1 && i.markup.at(0) == part) {
			index = r;
			return LOCAL;
		}
		r++;
	}
	assert(false);
}
#endif

//find thing in locals or consts by termid
Thing &fetch_thing(termid x, Locals &locals, Locals &consts, locals_map &lm, locals_map &cm)
{
	size_t i;
	auto pp = find_thing(x, i, lm, cm);
	if (pp == LOCAL)
		return locals[i];
	else if (pp == CONST)
		return consts[i];
	else
		assert(false);
}


void print_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head)
{
	(void)head;
	dout << endl << "locals map: (termid, pos, thing, kbdbg)" << endl;
	for (auto x: lm)
		dout << " " << op->format(x.first) << "     " << x.second << "     " << str(&locals.at(x.second)) <<
#ifdef KBDBG
		"     " << kbdbg_str(&locals.at(x.second)) <<
#endif
		endl;
#ifdef KBDBG
	dout << "locals: (pos, thing, kbdbg)" << endl;
	for (size_t i = 0; i<locals.size(); i++)
		dout << " " << i << "     " << str(&locals.at(i)) << "     " << kbdbg_str(&locals.at(i)) << endl;
#endif
	if (cm.size()) {
		dout << "consts:" << endl;
		for (auto x: cm)
			dout << op->format(x.first) << " : : " << x.second << "  --> " << str(&consts.at(x.second)) << endl;
	}
}




//think {?a x ?a. (?a) x b} => b x b
//locals: var (thats the ?a) | list header (size 1) | offset - 2 (pointing to the first var)  | nil 
//consts:  b (node - a constant)
//a const in a list wont go into consts but stay in the list:
//{?a x ?a. (?a b) x b} => b x b 
//locals: var (thats the ?a) | list header (size 2) | offset - 2 (pointing to the first var) | list bnode ("size" 1) | node b | nil 
//consts: the node b

void make_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head, qlist &body)
{
	FUN;

	std::queue<toadd> lq;
	TRACE(dout << "head:" << op->format(head) << endl);

	/* Function definition region */
	//We make 3 function which we then apply later, so in reading this make_locals function,
	//treat this as if they're just some functions and skip past them until needed.



	//in add_node we ignored lists, we just added them as nodes and queued them up
	//more like we don't pass lists to add_node in the first place

	//As long as there's still terms in lq, pop the 1st one off the list and 
	auto expand_lists = [&lq, &locals, &lm]() {
		setproc("expand_lists");
		while (!lq.empty()) {
			toadd ll = lq.front();
			lq.pop();

			termid l = ll.first;

			auto lst = op->get_dotstyle_list(l);

			Thing i0; // list size item, a sort of header / pascal string (array?) style thing
#ifdef KBDBG
			add_kbdbg_info(i0, ll.second);
			unsigned long list_part = 0;
#endif
			make_this_list(i0, lst.size());
			locals.push_back(i0);
			lm[l] = locals.size()-1; // register us in the locals map

			size_t bnode_counter = lst.size();

			//For each item in the list, 
			for (auto li: lst) {
				TRACE(dout << "item..." << endl;)
#ifdef KBDBG
				Markup m = ll.second;
				m.push_back(list_part++);
#endif
				Thing t;
				if (li->p < 0) { //its a var
					auto it = lm.find(li); //is it already in locals?
					if (it == lm.end()) { //no? create a fresh var
						t = create_unbound();
						lm[li] = locals.size();
					}
					else { //yes? just point to it
						make_this_offset(t, ofst(it->second, locals.size()));
					}
				}
				else { //its a node
					t = create_node(li);
					if (islist(li))
						#ifdef KBDBG
						lq.push(toadd(li, m));
						#else
						lq.push(toadd(li, {}));
						#endif
				}
#ifdef KBDBG
				add_kbdbg_info(t, m);
#endif
				locals.push_back(t);

				//we add bnodes that simulate rdf list structure
				Thing bnode;
				make_this_list(bnode, --bnode_counter);
				locals.push_back(bnode);
				
			}

			//final nil
			Thing nil = create_node(op->make(rdfnil,0,0));
			locals.push_back(nil);

		}
	};

	//replace NODEs whose terms are lists with OFFSETs. expand_lists left them there.
	auto link_lists = [&locals, &lm]() {
		for (size_t i = 0; i < locals.size(); i++) {
			Thing &x = locals[i];
			if (is_node(x) && islist(get_term(x))) {
				make_this_offset(x, ofst(lm.at(get_term(x)), i));
			}
		}
	};


	//vec is a vector of Things, and m is a map from termid's to indexes into
	//that vector.
	auto add_node = [](bool var, toadd xx, Locals &vec, locals_map &m) {
		setproc("add_node");
		Thing t;
		add_kbdbg_info(t, xx.second);
		termid x = xx.first;
		TRACE(dout << "termid:" << x << " p:" << dict[x->p] << "(" << x->p << ")" << endl;)

		//Check to see if the termid is already in m (locals/consts).
		//If it's not then add it.
		auto it = m.find(x);
		if (it == m.end()) {
			//This will give the position in vec that the Thing
			//will go, and m will map termid's to their position in
			//vec.
			m[x] = vec.size();

			//Create a Thing for this termid and push it to the back
			//of vec.
			if(var)
				t = create_unbound();
			else
				t = create_node(x);
			add_kbdbg_info(t, xx.second);
			vec.push_back(t);
		}
#ifdef KBDBG
		else
		{
			make_this_offset(t, ofst(it->second, vec.size()));
			add_kbdbg_info(t, xx.second);
			vec.push_back(t);
		}
#endif
	};


	/* Execution region */ 

	vector<toadd> terms;

//Need to understand this kbdbg stuff
	unsigned long old_kbdbg_part = kbdbg_part;

	//Make toadds out of the head and all terms in the body and push these
	//into terms.
	if (head) {
		terms.push_back(toadd(head->s, {kbdbg_part++}));
		terms.push_back(toadd(head->o, {kbdbg_part++}));
	}
	for (termid bi: body) {
		terms.push_back(toadd(bi->s, {kbdbg_part++}));
		terms.push_back(toadd(bi->o, {kbdbg_part++}));
	}
	//TRACE(dout << "terms.size:" << terms.size() << endl);

	//For all our terms (toadds) in terms, if the term is not
	//a variable or a list, then "add_node(false, xx, locals, lm)".
	//If the term is a variable, then "add_node(true, xx, locals, lm)".
	//If it's a list, then push it to lq to be processed later.
	for (toadd xx: terms) {
		termid x = xx.first;
		if (x->p > 0 && !islist(x)) {
#ifndef KBDBG
			//force rule s and o into locals for now
			if (head && (x == head->s || x == head->o))
				add_node(false, xx, locals, lm);
			else
				add_node(false, xx, consts, cm);
#else
			add_node(false, xx, locals, lm);
#endif
		}
		else if (x->p < 0)
			add_node(true, xx, locals, lm);
		else if (x->p > 0 && islist(x))
			lq.push(xx);
		else
			assert(false);
	}

	expand_lists();
	link_lists();

	kbdbg_part_max = kbdbg_part;
	kbdbg_part = old_kbdbg_part;

	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	TRACE(print_locals(locals, consts, lm, cm, head);)
}




join_gen compile_body(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head, prover::termset body)
{
	FUN;
	join_gen jg = succeed_with_args_gen();
	//for (int i = body.size() - 1; i >= 0; i--) {
		//termid &bi = body[i];
	auto b2 = body;
	reverse(b2.begin(), b2.end());
#ifdef KBDBG
	auto max = kbdbg_part_max;
#endif

	//For every term in the body, check the pred to see if there's a head
	//with this pred. If not we'll set the pred-function for that pred to
	//fail_with_args. 
	for (termid bi: b2)
	{
		check_pred(bi->p);
		termid s = bi->s;
		termid o = bi->o;
		size_t i1, i2;
		PredParam sk, ok;
#ifdef KBDBG
		(void)lm;
		(void)cm;
		ok = maybe_head(kbdbg_find_thing(--max, i2, locals), head, o);
		TRACE(dout<<"max:"<<max<<endl);
		sk = maybe_head(kbdbg_find_thing(--max, i1, locals), head, s);
		TRACE(dout<<"max:"<<max<<endl);

#else
		(void)locals;
		sk = maybe_head(find_thing(s, i1, lm, cm), head, s);
		ok = maybe_head(find_thing(o, i2, lm, cm), head, o);
#endif
		TRACE(dout <<"perm " << permname.at(sk) << " " << permname.at(ok) << endl );
		jg = perms.at(sk).at(ok)(bi->p, jg, i1, i2, consts);
		//typedef function<join_gen(pred_gen, join_gen, size_t, size_t, Locals&)>
	}
	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	return jg;
}






bool find_ep(ep_t *ep, /*const*/ Thing *s, /*const*/ Thing *o)
{
	FUN;
	s = getValue(s);
	o = getValue(o);
	TRACE(dout << ep->size() << " ep items:" << endl);
	for (auto i: *ep) 
	{
		auto os = i.first;
		auto oo = i.second;
		TRACE(dout << endl << " " << str(os) << "    VS     " << str(s) << endl << str(oo) << "    VS    " << str(o) << endl;)
		if (would_unify(os,s))
		{
			//TRACE(dout << ".." << endl);
			if(would_unify(oo,o)) {
				TRACE(dout << endl << "EP." << endl;)
				return true;
			}
		}
			TRACE(dout << endl << "---------------------" << endl);

	}
	return false;
}




rule_t compile_rule(prover::ruleid r)
{
	FUN;

	//Get the head and body associated with the ruleid.
	termid head = op->heads[r];
	prover::termset body = op->bodies[r];

	//What do each of these do?

	//should draw a picture of the data structure(s)
	//maps from termids to indexes into locals/consts
	locals_map lm, cm;
	//they will be needed after this func is over so we allocate them on the heap
	Locals &locals_template = *new Locals();
	locals_templates_garbage.push_back(&locals_template);//and register them for garbage collection
	Locals *consts_ = new Locals();
	consts_garbage.push_back(consts_);
	Locals &consts = *consts_;

	make_locals(locals_template, consts, lm, cm, head, body);


	join_gen jg = compile_body(locals_template, consts, lm, cm, head, body);

	size_t hs, ho; // indexes of head subject and object in locals
#ifdef KBDBG
	kbdbg_find_thing(kbdbg_part++, hs, locals_template);
	kbdbg_find_thing(kbdbg_part++, ho, locals_template);
	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	kbdbg_part = kbdbg_part_max;
#else
	//ignoring key, because head s and o go into locals always
	find_thing(head->s, hs, lm, cm);//sets hs
	find_thing(head->o, ho, lm, cm);
#endif

	EEE;
	join_t j;
	coro suc, ouc;
	TRC(int call = 0;)
	ep_t *ep = new ep_t();//ep is one per rule just as locals_template and consts
	eps_garbage.push_back(ep);

	//where to memcpy locals from and what length
	auto locals_data = locals_template.data();
	auto locals_bytes = locals_template.size() * sizeof(Thing);
	Thing * locals=0; //to be malloced inside the lambda
	const bool has_body = body.size(); // does this rule have a body or is it a fact?

	return [has_body, locals_bytes, locals_data, ep, hs, ho, locals ,&consts, jg, suc, ouc, j, entry TRCCAP(call) TRCCAP(r)](Thing *s, Thing *o) mutable {
		setproc("rule");
		TRC(++call;)
		TRACE(dout << op->formatr(r) << endl;)
		TRACE(dout << "call=" << call << endl;)
		switch (entry) {
			case 0: 
				locals = (Thing*)malloc(locals_bytes);
				memcpy(locals, locals_data, locals_bytes);
			
				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
				ASSERT(hs < locals_bytes / sizeof(Thing));
				ASSERT(ho < locals_bytes / sizeof(Thing));

				suc = unify(s, &locals[hs]); // try to match head subject
				while (suc()) {
					TRACE(dout << "After suc() -- " << endl;)
					//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
					ASSERT(call == 1);

					ouc = unify(o, &locals[ho]);
					while (ouc()) {
						TRACE(dout << "After ouc() -- " << endl;)
						//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
						ASSERT(call == 1);

						if ((steps != 0) && (steps % 1000000 == 0)) (dout << "step: " << steps << endl);
							++steps;

						o = getValue(o);
						s = getValue(s);

						if (has_body && find_ep(ep, s, o)) {
							goto end;
						}
						if (has_body ) ep->push_back(thingthingpair(s, o));

						j = jg();
						while (j(s, o, locals)) {
							TRACE(dout << "After c0() -- " << endl;)
							//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

							if(has_body) {
								ASSERT(ep->size());
								ep->pop_back();
							}

							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;

				case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
							if (has_body ) ep->push_back(thingthingpair(s, o));
						}

						if(has_body) {
							ASSERT(ep->size());
							ep->pop_back();
						}
				end:;
					}
				}

				TRACE(dout << "DONE." << endl;)
				free(locals);
				END
		}
	};
}



//endregion











//region interface



pnode thing2node(Thing *t_, qdb &r) {
	t_ = getValue(t_);

	auto t = *t_;

	if (is_list(t))
	{
		const string head = listid();
		for (size_t i = 0; i < get_size(t); i++) {
			auto x = (t_ + 1 + (i*2));
			r.second[head].emplace_back(thing2node(x, r));
		}
		return mkbnode(pstr(head));
	}

	if (is_node(t))
		return std::make_shared<node>(dict[get_term(t)->p]);

	//dout << "thing2node: Wtf did you send me?, " << str(t_) << endl;
	assert(is_unbound(t));
	return mkiri(pstr("?V?A?R?"));
}


void add_result(qdb &r, Thing *s, Thing *o, nodeid p)
{
	r.first["@default"]->push_back(
		make_shared<quad>(
			quad(
				thing2node(s, r),
				std::make_shared<node>(dict[p]),
				thing2node(o, r)
			)
		)
	);
}






yprover::yprover ( qdb qkb)  {
	make_perms_table();
	build_in_facts();
	build_in_rules();
	compile_kb();
}


yprover::~yprover()
{
	free_garbage();
}


void yprover::thatsAllFolks(int nresults){
	dout << "That's all, folks, " << nresults << " results." << endl;
	dout << unifys << " unifys, " << steps << " steps." << endl;
	steps_ = steps;
	unifys_ = unifys;
}

#ifdef KBDBG

/*kbdbg fell to the wayside, it needs to be adapted to bnodes in lists and to builtins*/

void print_kbdbg_part(stringstream &o, termid t, unsigned long part)
{
	o << "[";
	if (islist(t)) {
		o << "\"( \",";
		unsigned long p = 0;
		auto lst = op->get_dotstyle_list(t);
		
		for (auto i: lst)
		{
			print_kbdbg_part(o, i, p++);
			o << ",\" \",";
		}
		o << "\")\"";
	}
	else
		o << "\"" << dstr(t->p, true) << "\"";
	o << "]";
}

void print_kbdbg_term(stringstream &o, termid t, unsigned long &part)
{
	print_kbdbg_part(o, t->s, part++);
	o << ",\" " << dstr(t->p, true) << " \",";
	print_kbdbg_part(o, t->o, part++);
}

void print_kbdbg_termset(stringstream &o, prover::termset b, unsigned long &part)
{
	size_t i = 0;
	for (auto bi: b) {
		print_kbdbg_term(o, bi, part);
		if (i++ != b.size() - 1)
			o << ", \". \", ";
	}
}


void print_kbdbg(prover::termset query)
{
	unsigned long part = 0;
	for (auto rules: pred_index) {
		for (auto rule: rules.second) {
			stringstream o;
			auto h = op->heads[rule];
			o << "\"{\",";
			print_kbdbg_term(o, h, part);
			o << ",\"}\"";
			auto b = op->bodies[rule];
			if (b.size()) {
				o << ",\" <= {\",";
				print_kbdbg_termset(o, b, part);
				o << ",\"}\"";
			}
			o << ",\"\\n\"";
			dout << "[" << o.str() << "]" << endl;

		}
	}
	stringstream o;
	print_kbdbg_termset(o, query, part);
	dout << "[" << o.str() << "]" << endl;
}
#endif


void yprover::query(const qdb& goal){
	FUN;
	results.clear();
	auto &q = *goal.first.at("@default");
#ifdef KBDBG
	print_kbdbg(q);
#endif
	steps = 0;
	unifys = 0;
	int nresults = 0; 
	locals_map lm, cm;
	Locals locals, consts;

	dout << KGRN << "COMPILE QUERY" << KNRM << endl;
	make_locals(locals, consts, lm, cm, 0, q);
	join_gen jg = compile_body(locals, consts, lm, cm, 0, q);

	join_t coro = jg();
	dout << KGRN << "RUN" << KNRM << endl;
	while (coro( (Thing*)666,(Thing*)666, locals.data() )) {
		nresults++;
		dout << KCYN << "RESULT " << KNRM << nresults << ":";
		qdb r;
		r.first["@default"] = mk_qlist();

		//go over the triples of the query to print them out
		for(auto i: q)
		{
			Thing *s = &fetch_thing(i->s, locals, consts, lm, cm);
			Thing *o = &fetch_thing(i->o, locals, consts, lm, cm);

			TRACE(dout << sprintThing("Subject", s) << " Pred: " << dict[i->p] << " "  << sprintThing("Object", o) << endl;)

			//lets try to get the original names of unbound vars
			Thing n1, n2;
			if (is_unbound(*s)) {
				s = &n1;
				n1 = create_node(i->s);
			}
			if (is_unbound(*o)) {
				o = &n2;
				n2 = create_node(i->o);
			}

			dout << str(getValue(s)) << " " << dict[i->p] << " "  << str(getValue(o)) << endl;

			add_result(r, s, o, i->p);
		}

		results.emplace_back(r);

		if (result_limit && nresults == result_limit) {
			dout << "STOPPING at " << KRED << nresults << KNRM << " results."<< endl;
			free_garbage_nonassert();
			goto out;
		}

	}
	thatsAllFolks(nresults);
	if (log_outputString.size()) {
		dout << "log#outputString:" << endl;
		for (const auto x:log_outputString)
			dout << x.first << ": " << x.second << endl;
		log_outputString.clear();
	}
	out:;
}

//endregion


void add_facts(vector<vector<nodeid>> facts)
{
	///std::sort(myList.begin(), myList.end(), [](int x, int y){ return std::abs(x) < std::abs(y); });
	///sort(facts.begin(), facts.end(), [](auto a, auto b) { return a[1] < b[1]; });

	ths_t &ths = *new ths_t;
	ths_garbage = &ths;///.push_back(ths);

	for (auto f: facts)
		ths[f[1]].push_back({
			create_node(op->make(f[0])),
			create_node(op->make(f[2]))});
	
	coro suc, ouc;
	for (auto ff:ths)
	{
		auto &pairs = ff.second;
		EEE;
		size_t pi = 0;
		builtins[ff.first].push_back([suc,ouc,pi,pairs,entry](Thing *s_, Thing *o_)	mutable{
			switch(entry)
			{
			case 0:
				if (pi < pairs.size())
				{
					suc = unify(s_,&pairs[pi].first);
					while(suc()){
						ouc = unify(o_,&pairs[pi].second);
						while(ouc())
						{
							entry = LAST;
							return true;
			case_LAST:;
						}
					}
				}
			END;
			}
		});
	} 
}
	




void build_in_facts()
{
	add_facts({

		{rdfAlt,                          rdfssubClassOf,    rdfsContainer},
		{rdfBag,                          rdfssubClassOf,    rdfsContainer},
		{rdfsContainerMembershipProperty, rdfssubClassOf,    rdfProperty},
		{rdfsDatatype,                    rdfssubClassOf,    rdfsClass},
		{rdfSeq,                          rdfssubClassOf,    rdfsContainer},
		{rdfXMLLiteral,                   rdfssubClassOf,    rdfsLiteral},
		{rdfXMLLiteral,                   rdftype,           rdfsDatatype},
		{rdfscomment,                     rdfsdomain,        rdfsResource},
		{rdfscomment,                     rdfsrange,         rdfsLiteral},
		{rdfsdomain,                      rdfsdomain,        rdfProperty},
		{rdfsdomain,                      rdfsrange,         rdfsClass},
		{rdffirst,                        rdfsdomain,        rdfList},
		{rdffirst,                        rdfsrange,         rdfsResource},
		{rdffirst,                        rdftype,           owlFunctionalProperty},
		{rdfsisDefinedBy,                 rdfsdomain,        rdfsResource},
		{rdfsisDefinedBy,                 rdfsrange,         rdfsResource},
		{rdfsisDefinedBy,                 rdfssubPropertyOf, rdfsseeAlso},
//		{mkiri(pstr(":HMC")), rdftype, mkiri(pstr(":banana"))},
		{rdfslabel,                       rdfsdomain,        rdfsResource},
		{rdfslabel,                       rdfsrange,         rdfsLiteral},
		{rdfsmember,                      rdfsdomain,        rdfsContainer},
		{rdfsmember,                      rdfsrange,         rdfsResource},
		{rdfobject,                       rdfsdomain,        rdfStatement},
		{rdfobject,                       rdfsrange,         rdfsResource},
		{rdfpredicate,                    rdfsdomain,        rdfStatement},
		{rdfpredicate,                    rdfsrange,         rdfProperty},
		{rdfsrange,                       rdfsdomain,        rdfProperty},
		{rdfsrange,                       rdfsrange,         rdfsClass},
		{rdfrest,                         rdfsdomain,        rdfList},
		{rdfrest,                         rdfsrange,         rdfList},
		{rdfrest,                         rdftype,           owlFunctionalProperty},
		{rdfsseeAlso,                     rdfsdomain,        rdfsResource},
		{rdfsseeAlso,                     rdfsrange,         rdfsResource},
		{rdfssubClassOf,                  rdfsdomain,        rdfsClass},
		{rdfssubClassOf,                  rdfsrange,         rdfsClass},
		{rdfssubPropertyOf,               rdfsdomain,        rdfProperty},
		{rdfssubPropertyOf,               rdfsrange,         rdfProperty},
		{rdfsubject,                      rdfsdomain,        rdfStatement},
		{rdfsubject,                      rdfsrange,         rdfsResource},
		{rdftype,                         rdfsdomain,        rdfsResource},
		{rdftype,                         rdfsrange,         rdfsClass},
		{rdfvalue,                        rdfsdomain,        rdfsResource},
		{rdfvalue,                        rdfsrange,         rdfsResource},
		{rdfnil,                          rdftype,           rdfList}
	});
}


void build_in_rules()
{//under construction
	EEE;
	coro suc, suc2, ouc;
	Thing *s = nullptr, *o = nullptr;
	Thing ss;
	//Thing oo;
	Thing *r = nullptr;
	Thing a,b;
	a = create_unbound();
	b = create_unbound();
	/*ep_t *ep = new ep_t();
	eps.push_back(ep)*/
	pred_t p1, p2;

	//Thing c_rdfsType = create_node(op->make(rdftype));
	//Thing c_rdfsResource = create_node(op->make(rdfsResource));
	//Thing c_rdfssubClassOf = create_node(op->make(rdfssubClassOf));



	//http://www.w3.org/TR/lbase/#using


	//rdfs:Resource(?x)
	/*<HMC_a> koo7: you mean if one queries "?x a rdf:Resource" should they get every known subject as a result?
	<HMC_a> the answer would be yes. :-)
	<koo7> HMC_a, every known subject and object, right?
	<koo7> or....every nodeid in the kb thats not in the pred position...where do we draw the line?
	<HMC_a> well, when i say "known subject" i don't really mean everything in the subject position, i mean every node useable as a subject (non-literal)
	<koo7> ok
	<koo7> what do you mean non-literal?
	<HMC_a> you wouldn't bind each int as a result, for example
	<HMC_a> if you returned "0 a Resource" "1 a Resource" "2 a Resource"..... this would be a bit of a problem ;-)
	<koo7> yeah, so everything that explicitly appears in the kb
	<koo7> traverse lists too
	<HMC_a> yes remember that lists are logically just triples as well...
	<koo7> err well wouldnt that mean the bnodes that rdf lists are made up of?
	<HMC_a> so any node name that appears within a list is in the object position of some rdf:first triple
	<HMC_a> yes, the bnode names as well*/
	/*builtins[rdftype].push_back([a, b, c_rdfsResource, entry, suc, suc2, ouc, s, o, p1](Thing *s_, Thing *o_) mutable {
		switch (entry) {
			case 0:
				o = getValue(o_);
				ASSERT(!is_offset(*o));
				ouc = unify(o, &c_rdfsResource);
				while (ouc())
				{
					s = getValue(s_);
					if(!is_unbound(*s))
					{
						entry = 1;
						return true;
					}
			case 1:
					if(is_unbound(*s))
					{
						for (auto &x: preds)
						{
							p1 = x.second;//a coro-permanent copy
							while(p1(&a, &b))
							suc = unify(s, &a);
							while(suc())
							{
								entry = 2;
								return true;
								case 2:;
							}
							suc2 = unify(s, b);
							while(suc2())
							{
								entry = 3;
								return true;
								case 3:;
							}
						}

					}
				}
				entry = 66;
				return false;
				END
		}
	});*/





	//todo rdfs:Class(?y) implies (?y(?x) iff rdf:type(?x ?y))





	//rdfs:domain(?x ?y) implies ( ?x(?u ?v)) implies ?y(?u) )
	//rdfs:range(?x ?y) implies ( ?x(?u ?v)) implies ?y(?v) )
	/*builtins[rdfsType].push_back([entry,p1,p2](Thing *s, Thing *c) mutable {
		switch(entry){
		case 0:
			p1 = ITEM(preds,rdfsdomain);
			Thing p = create_unbound();
			while (p1(&p, c))
			{
				ASSERT(is_node(p));
				pp = get_term(p)->p;
				if (preds.find(pp) != preds.end())
				{
					p2 = ITEM(preds, pp);
					Thing o = create_unbound();
					while(p2(s, o)
					{
						entry = LAST;
						return true;
		case_LAST:;
					}
				}
			}
			return false;
			END;
		}
	});
	builtins[rdfsRange].push_back([entry](Thing *s_, Thing *o_) mutable {
		switch(entry){
		case 0:
			for(auto x : rangeItems){
				ouc = unify(o,x.first);
				while(ouc()){
					for(auto y : x.second){
						suc = unify(s,y);
						while(suc()){
							entry = 1;
							return true;
		case 1:;
						}
					}
				}
			}
			entry = 2;
			return false;
		case 2:;
			assert(false);
		}
	});*/












/*
	wildcard_pred = [p1](nodeid r){
		EEE;
		return [entry, r, p1](Thing *s, Thing *o) mutable {
			//just iterate over wildcard
			switch (entry) {
				case 0:

					p1 = wildcard;
					while (p1(s, r, o)) {
						entry = LAST;
						return true;
						case_LAST:;
					}

					END;
			}
		};

	//{?P @has rdfs:subPropertyOf ?R. ?S ?P ?O} => {?S ?R ?O}.
	wildcard = [p1,p2](Thing *s, nodeid r, Thing *o) mutable {

		switch(entry){
		case 0:
			p1 = ITEM(preds,rdfssubPropertyOf);
			Thing r = create_unbound();
			while (p1(p, r))
			{
				ASSERT(is_node(p));
				pp = get_term(p)->p;
				if (preds.find(pp) != preds.end())
				{
					p2 = ITEM(preds, pp);
					Thing o = create_unbound();
					while(p2(s, o)
					{
						entry = LAST;
						return true;
		case_LAST:;
					}
				}
			}
			return false;
			END;
		}

	});
*/


/*
<HMC_a> koo7: for the moment I'm less concerned about getting rdfs going and more interested in facilities like log:outputString and math:sum and etc
<HMC_a> really even just those two would be enough to get some useful results out of the fuzzer, lol :-)
<koo7> HMC_a, i cant get too far without you being specific/providing some specs
<HMC_a> well
<koo7> so far all your input was "like in master"
<HMC_a> http://www.w3.org/2000/10/swap/doc/CwmBuiltins <-- "not quite specs" XD
<koo7> Dump :s to stdout ordered by :k whereever { :k log:outputString :s }
<HMC_a> so math:* and string:* should be pretty straightforward, list:* nearly so...
<koo7> so this means it waits until the query is done?
<HMC_a> yes, it caches up the output strings until the end
<koo7> ok
<koo7> as on list, list:append seems a misnomer
<HMC_a> then sorts them by subject
<HMC_a> then prints
<HMC_a> koo7: you mean it is more aptly called "concat"? ;-)
<koo7> yeah something like that
<HMC_a> I don't think you're the first to mention it, hehe
* HMC_a shrugs
<koo7> alright
<koo7> its fully namespaced, after all
<nilli> I used the link koo7 gave me and changed the name but its going on #zennet. anyway never mind
<nilli> ill just sit in the dark
<HMC_a> sure, and I'm not against doubling up on some builtins later... maybe in the end we have list:append, tau:append, and tau:concat, with tau:append taking just a subject of a pair list of a list and a literal and doing an actual "append" and the other two both being "concat"...
<HMC_a> but we want list:append to be there and to match cwm, in any case, so that any existing logic "out there" that calls upon cwm's list:append will do the right thing
<HMC_a> ;-)
<koo7> cool*/

/*
	old::string link = "http://www.w3.org/TR/rdf-schema/#ch_range";
	auto link_node = dict.set(mkiri(pstr(link)));
	builtins[link_node].push_back(
		[entry, ... ](Thing *s_, Thing *o_) mutable {
			switch(entry){
			case 0:  
				
			}
		}
	);
*/

	//@prefix math: <http://www.w3.org/2000/10/swap/math#>.

	//sum: The subject is a list of numbers. The object is calculated as the arithmentic sum of those numbers.

	string bu = "http://www.w3.org/2000/10/swap/math#sum";
	auto bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[r, bu, entry, ouc, s, ss](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {

						s = getValue(s_);
						ss = *s;
						if (!is_list(ss)) {
							dout << bu << ": " << str(s) << " not a list" << endl;
							DONE;
						}
						long total = 0;
						const size_t size = get_size(ss);
						for (size_t i = 0; i < size; i++) {
							Thing item = *(s + 1 + i);
							if (!is_node(item)) {
								dout << bu << ": " << str(&item) << " not a node" << endl;
								DONE;
							}
							node p = dict[get_term(item)->p];
							if (p.datatype != XSD_INTEGER) {
								dout << bu << ": " << p.tostring() << " not an int" << endl;
								DONE;
							}
							total += atol(ws(*p.value).c_str());
						}

						std::stringstream ss;
						ss << total;

						r = new(Thing);
						*r = create_node(op->make(dict[mkliteral(pstr(ss.str()), pstr("XSD_INTEGER"), 0)], 0, 0));

						ouc = unify(o_, r);
					}
						while (ouc()) {
							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;
							case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
						delete (r);
						END;
				}
			}
	);


	//@prefix log: <http://www.w3.org/2000/10/swap/log#>.

	//outputString	The subject is a key and the object is a string, where the strings are to be output in the order of the keys. See cwm --strings in cwm --help.

	bu = "http://www.w3.org/2000/10/swap/log#outputString";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_node(o2)) {
							dout << bu << ": " << str(o) << " not a node" << endl;
							DONE;
						}
						auto sss = dict[get_term(s2)->p].tostring();
						auto ooo = dict[get_term(o2)->p].tostring();

						log_outputString[sss] = ooo;
						entry = LAST;
						return true;
					}
					case_LAST:;
						END;
				}
			}
	);


	//@prefix list: <http://www.w3.org/2000/10/swap/list#>.
	//list last item
	bu = "http://www.w3.org/2000/10/swap/list#last";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_list(s2)) {
							dout << bu << ": " << str(s) << " not a list" << endl;
							DONE;
						}

						auto size = get_size(s2);
						if (size == 0) DONE;
						ouc = unify(s + size, o_);
					}
						while (ouc()) {
							entry = LAST;
							return true;
							case_LAST:;
						}
						END;
				}
			}
	);


	//nope
	//item in list
	bu = "http://www.w3.org/2000/10/swap/list#in";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto t = get_term(s2);

						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_list(o2)) {
							dout << bu << ": " << str(o) << " not a list" << endl;
							DONE;
						}

						const auto size = get_size(o2);
						//?bool found = false;

						for (size_t i = 0; i < size; i++) {
							Thing item = *(o + 1 + i);
							if (is_node(item)) {
								if (t == get_term(item)) {
									entry = LAST;
									return true;
								}
							}
						}
					}
					case_LAST:;
						END;
				}
			}
	);


	//if the subject is bound, and bound to a list just take it's first item.
	// If it is bound to something that is not a list, fail.
	// If it is unbound, do a trivial yield (no new binding first).
	builtins[rdffirst].push_back(
			[entry, ouc, s, o](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						if (is_unbound(*s))
							ouc = gen_succeed();
						else if (is_list(*s))
							ouc = unify(o, s + 1);
						else
							ouc = GEN_FAIL;
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);

	builtins[rdfrest].push_back(
			[entry, ouc, s, o](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						if (is_list(*s) && get_size(*s))
							ouc = unify(o, s + 2);
						else
							ouc = GEN_FAIL;
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);

}


#ifdef notes65465687














//http://wifo5-03.informatik.uni-mannheim.de/bizer/SWTSGuide/carroll-ISWC2004.pdf

//Unicode

//XML Schema
//http://www.w3.org/TR/2009/CR-xmlschema11-2-20090430/

//LBase
//http://www.w3.org/TR/lbase/

//RDF
//http://www.w3.org/2011/rdf-wg/wiki/Main_Page
//http://www.w3.org/TR/2014/NOTE-rdf11-primer-20140225/
//http://www.w3.org/TR/2013/WD-rdf11-mt-20130409/
//http://www.w3.org/TR/rdf11-new/
//http://www.w3.org/TR/rdf11-concepts/
//http://www.w3.org/TR/rdf-syntax-grammar/
//http://www.w3.org/TR/2014/NOTE-rdf11-datasets-20140225/

//N-Quads
//http://www.w3.org/TR/2014/REC-n-quads-20140225/

//N-Triples
//http://www.w3.org/TR/n-triples/

//JSON

//JSON-LD
//http://www.w3.org/TR/json-ld/

//Notation 3
//

//RIF
//http://www.w3.org/standards/techs/rif#w3c_all
//http://www.w3.org/TR/rif-dtb/
//http://www.w3.org/TR/2013/REC-rif-dtb-20130205/

//Cwm Builtins
//http://www.w3.org/2000/10/swap/doc/CwmBuiltins   	--< HMC_a_> not all but most
//which?

//DTLC
//http://rbjones.com/rbjpub/logic/cl/tlc001.htm
//http://ceur-ws.org/Vol-354/p63.pdf

//OWL
//http://www.w3.org/TR/owl2-overview/

//make our semantics conform to them! ^














/*log:equalTo a rdf:Property;
True if the subject and object are the same RDF node (symbol or literal).
Do not confuse with owl:sameAs.
A cwm built-in logical operator, RDF graph level.
*/




int prover::rdfs_builtin(const term& t, const term *t0, const term *t1) {
	//TODO: correctly, so that subqery proof trace not opaque?
	int r = -1;
	//rdfs:Resource(?x)
	if ((t.p == A || t.p == rdftype) && t0 && t1 && t1->p == rdfsResource)
		r = 1;
	else if (t.p == rdfssubClassOf && t1->p == rdfsResource) {
		r = 0;
		// #{?C a rdfs:Class} => {?C rdfs:subClassOf rdfs:Resource}.
		{
			prover copy(*this);
			auto ps = copy.askt(t0, rdftype, make(rdfsClass, 0, 0));
			if (ps.size())
				return 1;
		}
	}
	else if (t.p == rdfssubClassOf) {
		r = 0;
		//#{?B rdfs:subClassOf ?C. ?A rdfs:subClassOf ?B} => {?A rdfs:subClassOf ?C}.
		{
			prover copy(*this);
			auto bs = copy.askt(copy.tmpvar(), rdfssubClassOf, t1);
			for (auto b: bs) {
				prover copy(*this);
				auto xs = copy.askt(t0, rdfssubClassOf, b);
				if (xs.size())
					return 1;
			}
		}
		//#{?X a rdfs:Datatype} => {?X rdfs:subClassOf rdfs:Literal}.
		if (t1->p == rdfsLiteral) {
			{
				prover copy(*this);
				auto ps = copy.askt(t0, rdftype, make(rdfsDatatype, 0, 0));
				if (ps.size())
					return 1;
			}
		}

	else if (t.p == A || t.p == rdftype) {
		r = 0;
		// {?P @has rdfs:domain ?C. ?S ?P ?O} => {?S a ?C}.
		{
			prover copy(*this);
			//TRACE(dout << "{?P @has rdfs:domain ?C. ?S ?P ?O} => {?S a ?C}." << std::endl;)
			auto ps = copy.askt(copy.tmpvar(), rdfsdomain, t1);
			for (termid p: ps) {
				dout << "\n\nYAYdomain!!\n\n" << std::endl;
				auto xx = copy.askt(t0, p->p, copy.tmpvar());
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}
		//{?P @has rdfs:range ?C. ?S ?P ?O} => {?O a ?C}.
		{
			prover copy(*this);
			auto ps = copy.askt(copy.tmpvar(), rdfsrange, make(t1->p, 0, 0));
			dout << "yays:" << ps.size() << std::endl;
			for (termid p: ps) {
				dout << "p:" << p << std::endl;
				dout << "\n\nYAYrange!!\n\n" << std::endl;
				auto xx = copy.askt(copy.tmpvar(), p->p, make(t0->p, 0, 0));
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}
		//#{?A rdfs:subClassOf ?B. ?S a ?A} => {?S a ?B}.
		{
			prover copy(*this);
			auto as = copy.askt(copy.tmpvar(), rdfssubClassOf, t1);
			for (termid a: as) {
				auto xx = copy.askt(t0, rdftype, a);
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}


	}
	else if (t.p == rdfssubPropertyOf) {
		r = 0;
		//#{?X a rdfs:ContainerMembershipProperty} => {?X rdfs:subPropertyOf rdfs:member}.
		if (t1 && t1->p == rdfsmember) {
			prover copy(*this);
			auto ps = copy.askt(copy.tmpvar(), rdftype, make(rdfsContainerMembershipProperty, 0, 0));
			if (ps.size())
				return 1;
		}
		//#{?Q rdfs:subPropertyOf ?R. ?P rdfs:subPropertyOf ?Q} => {?P rdfs:subPropertyOf ?R}.
		{
			prover copy(*this);
			auto qs = copy.askt(copy.tmpvar(), rdfssubPropertyOf, t1);
			for (termid q: qs) {
				auto ps = copy.askt(t0, rdfssubPropertyOf, q);
				if (ps.size())
					return 1;
			}
		}
	}
	//#{?P @has rdfs:subPropertyOf ?R. ?S ?P ?O} => {?S ?R ?O}.
	{
		prover copy(*this);
		auto ps = copy.askt(copy.tmpvar(), rdfssubPropertyOf, make(t.p, 0, 0));
		for (termid p: ps) {
			auto xs = copy.askt(t0, p->p, t1);
			if (ps.size())
				return 1;
		}
	}
	return r;
}



#endif
