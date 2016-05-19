#include "ast.h"
#include <iostream>

typedef ast::rule rule;
typedef ast::term term;

#define mkterm(x) new term(x)

void ast::term::crds(word &kb, crd c, int k) const {
	if (k != -1) c.c.push_back(k);
	if (!p) for (uint n = 0; n < sz; ++n) args[n]->crds(kb, c, n);
	else { c.str = p; kb.emplace(c); }
}

word rule::crds(int rl) {
	word r;
	if (head) head->crds(r), r = push_front(r, -1);
	for (uint n = 0; n < body.size(); ++n) {
		word k;
		body[n]->t->crds(k, crd(0), n+1);
		r.insert(k.begin(), k.end());
	}
	return push_front(r, rl);
}

template<typename T>
struct onret { T f; onret(T f):f(f){} ~onret(){f();} };

wrd::wrd(const word& w) {
	sz = w.size();
	c = new int*[sz];
	str = new pcch[sz];
	uint i = 0, j;
	for (const crd& r : w) {
		c[i]=new int[r.c.size()+1];str[i]=r.str;c[i][r.c.size()]=0;j=0;
		for (int x : r.c) c[i][j++] = x;
		++i;
	}
}

ostream& operator<<(ostream& os, const wrd& w) {
	for (uint n = 0; n < w.sz; ++n) {
		int *i = w.c[n];
		while (*i) os << *i++ << ':';
		if (w.str[n]) os << w.str[n];
		os << endl;
	}
	return os;
}

void refute(wrd &kb, int q) {
}


int ccmp::operator()(ccrd& x, ccrd& y) {
	auto ix = x.c.begin(), ex = x.c.end();
	auto iy = y.c.begin(), ey = y.c.end();
	while (ix != ex && iy != ey)
		if (*ix != *iy) return *ix < *iy ? 1 : -1;
		else ++ix, ++iy;
	return 0;
}

word push_front(cword& t, int i) { word r;
	for (auto x:t){x.c.push_front(i);r.insert(x);} return r; }
ostream& operator<<(ostream& os,cword& w){for(auto c:w)os<<c;return os<<endl;}
ostream& operator<<(ostream& os,ccrd& c) {
	for(int i:c.c)os<<i<<':';if(c.str)os<<'('<<c.str<<')';return os<<endl; }
ostream &ast::term::operator>>(ostream &os) const {
	if(p)return os<<p; os<<'(';auto a = args;
	while(*a)**a++>>os<<' ';return os<<')'; }
ast::term::term(term** _args,uint sz):p(0),args(new term*[sz+1]),sz(sz){
	for (uint n=0;n<sz;++n)args[n]=_args[n];args[sz]=0;} 
ast::term::term(pcch v) : p(ustr(v)), sz(0) { } 
ast::term::~term(){if (!p && args) delete[] args;} 
rule::premise::premise(const term *t):t(t){} 
rule::rule(const term *h, const body_t &b):head(h),body(b){}
template<typename T> inline vector<T> singleton(T x){
	vector<T>r;r.push_back(x);return x;} 
rule::rule(cterm *h,cterm *b):head(h),body(singleton(new premise(b))){}
rule::rule(const rule &r) : head(r.head), body(r.body) {}

void print(const ast& st) {
	dout << "rules: " << st.rules.size() << endl;
	uint n = 0;
	word kb;
	for (auto r : st.rules) {
		word k = r->crds(++n);
		for (auto x : k) kb.insert(x);
	}
	dout << kb << endl;
	for (auto r : st.rules) *r >> dout << endl;
	dout << wrd(kb) << endl;
}

int main() {
//	strings_test();
	ast st;
	st.terms.push_back(new ast::term("GND"));
	readdoc(false, &st);
	//print();
//	din.readdoc(true);
	print(st);
}
