#include "vm.h"

term *terms = 0;
rule *rules = 0; 
int nts = 0, nrs = 0;
const size_t chunk = 64;

int mkterm(const wchar_t* s, char type) {
	if (s && !type) {
		if (*s == L'?') type = '?';
		else if (*s == L'_') type = '_';
	}
	if (!(nts % chunk)) terms = realloc(terms, chunk * (nts / chunk + 1) * sizeof(term));
	return terms[nts].type = type, terms[nts].value = s, nts++;
}

int mktriple(int s, int p, int o) {
	int t = mkterm(0, 'T'), *a;
	terms[t].args = a = malloc(sizeof(int) * 3), a[0] = p, a[1] = s, a[2] = o;
	return t;
}

int mkrule(premise *p, int np, int *c) {
	if (!(nrs % chunk)) rules = realloc(rules, chunk * (nrs / chunk + 1) * sizeof(rule));
	return rules[nrs].c = c, rules[nrs].p = p, rules[nrs].np = np, nrs++;
}

void _print(int t) { print(&terms[t]); }

void print(const term* r) {
	if (!r) return;
	if (r->type == 'T') {
		_print(r->args[1]), putwchar(L' '), _print(r->args[0]),
		putwchar(L' '), _print(r->args[2]), putwchar(L'.');
		return;
	}
	if (r->type != '.') { wprintf(r->value); return; }
	wprintf(L"( ");
	const int *a = r->args;
	while (*++a) print(&terms[*a]), putwchar(L' ');
	putwchar(L')');
}

void printps(const premise *p, int np) {
	if (!p) return;
	wprintf(L"{ ");
	for (int n = 0; n < np; ++n) print(&terms[p[n].p]);
	putwchar(L'}');
}

void printr(const rule* r) {
	if (!r) return;
	printps(r->p, r->np), wprintf(L" => {"); 
	if (r->c) for (int* t = r->c; *t; ++t) _print(*t);
	putwchar(L'}');
}

void printa(int *a, int l) {
	for (int n = 0; n < l; ++n) wprintf(L"%d ", a[n]);
}

void printc(int **c) {
	if (!c) return;
	static wchar_t s[1024], ss[1024];
	for (int r = *s = 0; r < **c; ++r) {
		*s = 0;
		swprintf(s, 1024, L"row %d:\t", r);
		for (int n = 0; n < (r ? c[0][r] : **c + 1); ++n)
			swprintf(ss, 1024, L"%d\t", c[r][n], wcscat(s, ss));
		fputws(s, stdout);
	}
}
