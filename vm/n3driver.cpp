#include "n3driver.h"
#include <sstream>
#include <cstdio>
ostream &dout = std::cout;

//namespace n3driver {

vector<term*> terms;
vector<rule*> rules;
map<const term *, int> dict;
//din_t din;
char ch;
bool f = false, done, in_query = false, eof = false;

#define THROW(x, y) { \
    stringstream s;  \
    s << x << ' ' << y; \
    throw runtime_error(s.str()); }

char peek() {
	char r = f ? ch : ((f = true), ch = getchar());
	eof = r == EOF;
	return r;
}
char get() {
	char r = f ? ((f = false), ch) : (ch = getchar());
	eof = r == EOF;
	return r;
}
char get(char c) {
	char r = get();
	if (c != r) THROW("expected: ", c);
	return r;
}
bool good() { return !feof(stdin) && ch != EOF && !eof; }
void skip() { while (good() && isspace(peek())) get(); }
void getline() {
	size_t sz = 0;
	char *s = 0;
	f = false;
	::getline(&s, &sz, stdin);
	free(s);
	ch = ' ';
}
//din_t() : eof(false) {/* wcin >> std::noskips;*/ } 
string &trim(string &s) {
	static string::iterator i = s.begin();
	while (isspace(*i)) s.erase(i), i = s.begin();
	size_t n = s.size();
	if (n) {
		while (isspace(s[--n])) ;
		s = s.substr(0, ++n);
	}
	return s;
}

string edelims = ")}.";
term* readany();

string till() {
	skip();
	static char buf[4096];
	static size_t pos;
	pos = 0;
	while (good() && edelims.find(peek()) == string::npos && !isspace(peek()) && pos < 4096)
		buf[pos++] = get();
	if (pos == 4096) perror("Max buffer limit reached (till())");
	buf[pos] = 0;
	string s;
	if (buf && strlen(buf)) s = buf;
	return s;
}

term* readlist() {
	get();
	vector<term*> items;
	while (skip(), (good() && peek() != ')'))
		items.push_back(readany()), skip();
	return get(), skip(), mkterm(items);
}

term* readany() {
	if (skip(), !good()) return 0;
	if (peek() == '(') return readlist();
	string s = till();
	if (s == "fin" && peek() == '.') return get(), done = true, (term*)0;
	return s.size() ? mkterm(s.c_str()) : 0;
}

const term *readterm() {
	term *s, *p, *o;
	if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
		return 0;
	const term *r = mktriple(s, p, o);
	if (skip(), peek() == '.') get(), skip();
	return r;
}

void readdoc(bool query) { // TODO: support prefixes
	const term *t;
	done = false;
	while (good() && !done) {
		body_t body;
		switch (skip(), peek()) {
		case '{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && peek() != '}')
				body.push_back(new premise(readterm()));
			get(), skip(), get('='), get('>'), skip(), get('{'), skip();
			if (peek() == '}')
				rules.push_back(new rule(0, body)), skip();
			else
				while (good() && peek() != '}')
					rules.push_back(new rule(readterm(), body));
			get();
			break;
		case '#':
			getline();
			break;
		default:
			if ((t = readterm()))
				rules.push_back(query ? new rule(0, t) : new rule(t));
			else if (!done)
				THROW("parse error: term expected", "");
		}
		if (done) return;
		if (skip(), peek() == '.') get(), skip();
	}
}

// output
ostream &premise::operator>>(ostream &os) const { return (*t) >> os; }
ostream &rule::operator>>(ostream &os) const {
	os << '{';
	for (auto t : body) *t >> os << ' ';
	return os << "} => { ", (head ? *head >> os : os << ""), os << " }.";
}
