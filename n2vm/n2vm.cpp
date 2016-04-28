#include <iostream>
#include "n2vm.h"
#include <set>
using namespace std;

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

namespace n2vm {

term::operator string() const {
	stringstream ss;
	if (p) ss << p;
	else {
		ss << '(';
		auto a = args;
		while (a && *a) ss << (string)(**a++) << ' ';
		ss << ')';
	}
	return ss.str();
}

rule::operator string() const {
	stringstream ss;
	ss << "{ ";
	if (h) ss << (string)(*h);
	ss << "} <= {";
	for (unsigned n = 0; n < sz; ++n)
		ss << (string)*b[n] << ' ';
	ss << "}.";
	return ss.str();
}

#define tocstr(x) ( x ? ((string)(*(x))).c_str() : "(null)" )

bool n2vm::add_constraint(hrule r, hprem p, hrule h, const term *x, const term *y) {
	if (x == y) return true;
	if (!x || !y) return false;
	TRACE(printf("add_constraint(%d,%d,%d,%s,%s)\n", r, p, h, tocstr(x), tocstr(y)));
	return add_constraint(*(*kb[r])[p], h, *x, *y);
}

bool n2vm::add_constraint(iprem &p, hrule h, const term &tx, const term &ty) {
	TRACE(printf("add_constraint(%d,%s,%s)\n", h, tocstr(&tx), tocstr(&ty)));
	if (!isvar(tx)) {
		if (isvar(ty)) return add_constraint(p, h, ty, tx);
		if (islist(tx)) {
			if (!islist(ty)) return false;
			return add_lists(p, h, tx, ty);
		}
		return tx.p == ty.p;
	}
	return add_constraint(p, h, tx.p, ty);
}

bool n2vm::add_constraint(iprem &p, hrule h, int x, const term& y) {
	TRACE(printf("add_constraint(%d,%d,%s)\n", h, x, tocstr(&y)));
	sub &s = p[h];
	sub::const_iterator it = s.find(x);
	if (it != s.end()) {
		if (!isvar(*it->second))
			return add_constraint(p, h, *it->second, y);
		const term* z = it->second;
		s[x] = &y;
		return add_constraint(p, h, *z, y);
	}
	return s[x] = &y, true;
}

bool n2vm::add_lists(iprem &p, hrule h, const term &x, const term &y) {
	TRACE(printf("add_lists(%d,%s,%s)\n", h, tocstr(&x), tocstr(&y)));
	auto sz = x.sz;
	if (y.sz != sz) return false;
	auto ix = x.args, iy = y.args;
	for (; *ix;  ++ix, ++iy)
		if (!add_constraint(p, h, **ix, **iy))
			return false;
	return true;
}

bool n2vm::mutate(iprem &dn, const sub &e, auto m, const sub &env) {
	for (auto c : e)
		if (!add_constraint( dn, m.first, c.first, *c.second))
			return false;
	for (auto c : env)
		if (!add_constraint( dn, m.first, c.first, *c.second))
			return false;
	return true;
}

hrule n2vm::mutate(hrule r, const sub &env) {
	irule &d = *new irule, &s = *kb[r];
	auto sz = s.size();
	d.resize(sz);
	for (unsigned n = 0; n < sz; ++n) {
		iprem &dn = *(d[n] = new iprem);
		for (const auto &m : *s[n])
			if (!mutate(dn, m.second, m, env))
				dn.erase(m.first);
		if (dn.empty()) return -1;
	}
	kb.push_back(&d);
	// TODO: remove matched constraints from varmap
	return kb.size();
}

term* n2vm::add_term(const char* p, const std::vector<term*>& args) {
	struct term_cmp {
		int operator()(const term *_x, const term *_y) const {
			static term_cmp tc;
			if (_x == _y) return 0;
			const term &x = *_x, &y = *_y;
			if (x.p > y.p) return 1;
			if (x.p < y.p) return -1;
			int r;
			auto ix = x.args, iy = y.args;
			for (; *ix;  ++ix, ++iy)
				if ((r = tc(*ix, *iy)))
					return r;
			return 0;
		}
	};
	static set<term*, term_cmp> terms;
	term *t = args.empty() ? new term(p) : new term(args);
	terms.emplace(t);
	return t;
}

void n2vm::getvarmap(const term& t, varmap& v) {
	if (isvar(t)) v.push_front(t.p);
	for (auto x = t.args; *x; ++x) getvarmap(**x, v);
}

void n2vm::add_rules(rule *rs, unsigned sz) {
	orig = rs;
	origsz = sz;
	kb.resize(sz);
	for (unsigned r = 0; r < sz; ++r) {
		(kb[r] = new irule)->resize(rs[r].sz);
		if (!rs[r].h) query = r;
		for (unsigned p = 0; p < rs[r].sz; ++p) {
			(*kb[r])[p] = new iprem;
			for (unsigned h = 0; h < sz; ++h)
				add_constraint(r, p, h, rs[r].b[p], rs[h].h);
		}
		if (rs[r].h) getvarmap(*rs[r].h, vars[r]);
	}
}

bool n2vm::tick() {
	TRACE(cout<<"tick"<<endl);
	if (!last) last = curr = new frame(query, 0, 0);
	if (!curr) return false;
	hrule r;
	for (auto m : *(*kb[curr->r])[curr->p]) {
		TRACE(printkb());
		if (-1 != (r = mutate(m.first, m.second))) continue;
		last = (last->next = new frame(r, 0, curr));
	}
	curr = curr->next;
	return true;
}
	
void n2vm::printkb() {
	for (unsigned n = 0; n < kb.size(); ++n) {
		cout << "Rule " << n << ':' << (n < origsz ? (string)orig[n] : string("")) << endl;
		const auto &r = *kb[n];
		for (unsigned k = 0; k < r.size(); ++k) {
			cout << "\tPrem " << k << ':' << endl;
			for (auto &m : *r[k]) {
				cout << "\t\tHead " << m.first << ':' << endl;
				for (auto &x : m.second)
					cout << "\t\t\t" << x.first << ' ' << (string)*x.second << endl;
			}
		}
	}
}
}
