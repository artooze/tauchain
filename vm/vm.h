#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <wctype.h>
#include "cfpds.h"

typedef struct term term;
typedef struct rule rule;
typedef struct premise premise;
extern term *terms;
extern rule *rules;
extern int nts, nrs;

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

// Following 3 structs store the kb&query.
//
// int* arrays below (args, c) have first int as length, and
// are also zero terminated. hence empty list consists of two zeros.
// struct term encapsulates triples as well, for efficiency and other reasons.

enum etype { 	LIST 	= '.',	VAR 	= '?',
		BNODE 	= '_',	IRI 	= 'I',
		LITERAL = 'L',	TRIPLE 	= 'T' };
typedef enum etype etype;

// resource (IRI/literal/variable/list) or triple (where args contain spo)
struct term {
	etype type; 
	union {
		const wchar_t *value; // resource's value
		const int *args; // list's elements ids
	};
};

struct premise {
	int p; 	// term (triple) id
	uf **e;	// equality relations (calculated from premises
		// and conclusions). e[r][k] is the union-find data 
		// structure of the k'th conclusion in the r's rule.
};

struct rule {
	int *c; // conclusions, disjuncted. every int is a term (triple) id
	premise *p; // premises, conjuncted
	int np; // number of premises
};

// in ir.c
int mkterm(const wchar_t* s, char type); // constucts a nonlist term
int mkrule(premise *premises, int num_of_premises, int *conclusions);
int mktriple(int s, int p, int o); // returns a list term with spo as its items
void print(const term* r); // print term
void printts(const int *t); // print array of terms
void printps(const premise *p, int num); // print premises
void printr(const rule* r); // print rule
void printa(int *a, int sz); // print array of integers
void printc(int **c); // print equivalence relation
// complete input is read into here first, then free'd by the callse after a parser pass
extern wchar_t *input; 
// in n3.c
void parse();
