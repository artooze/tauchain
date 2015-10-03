#include <map>
#include <vector>
#include <utility>
using namespace std;

enum etype { IRI, VAR, BNODE, LITERAL, LIST };
struct resource {
	etype type;
	union {
		resource* sub;	// substitution, in case of var.
				// not used in compile time
		resource** args;// list items. last item must be null
	};
};
vector<resource> res;

struct triple {
	resource *r[3]; // spo
};

struct rule {
	triple head;
	vector<triple> body;
};
vector<rule> rules;

typedef map<resource*, resource*> vars;

bool compile(resource *s, resource *d, vars& c) {
	if (s->type != VAR && d->type != VAR) {
		if (s != d) return c.clear(), false;
		if (s->type != LIST) return true;
		resource **rs, **rd;
		for (rs = s->args, rd = d->args; *rs && *rd;)
			if (!*rs != !*rd || !compile(*rs++, *rd++, c))
				return false;
		return true;
	}
	return c[s] = d, true;
}

bool compile(triple *s, triple *d, vars& c) {
	for (int n = 0; n < 3; ++n)
		if (!compile(s->r[n], d->r[n], c))
			return false;
	return true;
}

// compiler's intermediate representation language. the information
// in the following variable contains everything the emitter has to know
map<tuple<int/*head*/,int/*body*/,int/*head*/>, vars> intermediate;

void compile() {
	vars c;
	for (int n = 0; n < rules.size(); ++n)
		for (int k = 0; k < rules[n].body.size(); ++k)
			for (int m = 0; m < rules.size(); ++m)
				if (compile(&rules[n].body[k], &rules[m].head, c))
					intermediate[make_tuple(n, k, m)] = c, c.clear();
};