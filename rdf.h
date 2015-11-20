#ifndef __RDF_DATA__
#define __RDF_DATA__

#include <list>
#include <set>

#include "json_object.h"

namespace old{


typedef std::map<string, string> ssmap;
typedef std::shared_ptr<ssmap> pssmap;




class node;
typedef std::shared_ptr<node> pnode;
typedef std::map<string, pnode> snmap;
typedef std::shared_ptr<snmap> psnmap;

class node {
public:
	pstring value, datatype, lang;	
	pnode next = 0;
	
	enum node_type { LITERAL, IRI, BNODE } _type;
	
	
	node() {}	
	node ( const node_type& t ) : _type ( t ) { }
	
	
	string tostring() const;


	bool operator<(const node& x) const { return tostring() < x.tostring(); }
	bool operator==(const node& x) const { 
		return tostring()==x.tostring();
		//*value == *x.value && *datatype == *x.datatype && *lang == *x.lang && _type == x._type;
	}
};




inline std::wostream& operator<<(std::wostream& o, const node& n) { return o << n.tostring(); }





pnode mkliteral ( pstring value, pstring datatype, pstring language );
pnode mkiri ( pstring iri );
pnode mkbnode ( pstring attribute );






class quad {
	quad ( string subj, string pred, pnode object, string graph );

public:
	//Definition
	pnode subj, pred, object, graph;

	//Constructors
	quad ( string subj, string pred, string object, string graph );
	quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph );
	quad ( pnode subj, pnode pred, pnode object, string graph = L"@default" );
	quad ( pnode subj, pnode pred, pnode object, pnode graph);

	quad(){}
	quad(const quad& q) : subj(q.subj), pred(q.pred), object(q.object), graph(q.graph) {}


	string tostring ( ) const;
};



//typedef std::shared_ptr<quad> p_quad;
//typedef std::list<pquad> lp_quad;
//typedef std::shared_ptr<lp_quad> plp_quad;

//typedef std::list<pnode> qdbList;

//typedef string contextName;
//typedef string listBNodeName;

//typedef std::map<contextName, plp_quad> qdbQuads;
//typedef std::map<listBNodeName, qdbList> qdbLists;

//typedef std::pair<qdb_quads,qdb_lists> qdb;

typedef std::shared_ptr<quad> pquad;
typedef std::list<pquad> qlist;
typedef std::shared_ptr<qlist> pqlist;
typedef std::pair<std::map<string, pqlist>, std::map<string, std::list<pnode>>> qdb;




qdb merge_qdbs(const std::vector<qdb> qdbs);

extern const pnode first;
extern const pnode rest;
extern const pnode nil;

pqlist mk_qlist();

std::wostream& operator<< ( std::wostream& o, const qdb& );
std::wostream& operator<< ( std::wostream& o, const qlist& );
#ifndef NOPARSER
int readqdb (qdb& r, std::wistream& is);
#endif
#ifdef JSON
class jsonld_api;
class rdf_db: public qdb {
	ssmap context;
	jsonld_api& api;
public:
	using qdb::first;
	using qdb::second;
	rdf_db ( jsonld_api& );

	string tostring();

	void setNamespace ( string ns, string prefix );
	string getNamespace ( string ns );
	void clearNamespaces();
	ssmap& getNamespaces();
	somap getContext();

	void parse_ctx ( pobj contextLike );

	void graph_to_rdf ( string graph_name, somap& graph );

private:
	pnode obj_to_rdf ( pobj item );
};

typedef std::shared_ptr<rdf_db> prdf_db;
#endif

//Does this need it's own class?
#ifndef NOPARSER
class nqparser {
private:
	typedef std::list<pnode> plist;

	wchar_t *t;
	const wchar_t *s;
	std::list<quad> r;
	int pos;
	
	std::map<string, pnode> prefixes;
	std::map<string, std::list<pnode>> qlists;
	std::list<std::tuple<pnode, pnode, pnode>> lists;

	int nq_to_qdb(qdb& kb, std::wistream& is);

	pnode readcurly();
	pnode readlist();
	pnode readany(bool lit = true);
	pnode readiri();
	pnode readlit();
	pnode readvar();
	pnode readbnode();
	void readprefix();
public:
	nqparser();
	~nqparser();
	int nq_to_qdb(qdb& kb, std::wistream& is);
	//std::pair<std::list<quad>, std::map<string, std::list<pnode>>> operator()(const wchar_t* _s, string ctx = L"@default");
};
qlist merge ( const qdb& q );

#endif


}


#endif

