/*
http://www.w3.org/2000/10/swap/grammar/bnf
http://www.w3.org/2000/10/swap/grammar/n3.n3
*/

#include <iostream>
#include <fstream>

#include "prover.h"
#include "object.h"
#include "cli.h"
#include "rdf.h"
#include "misc.h"
#include "jsonld.h"

#include "marpa_tau.h"

extern "C" {
#include "marpa.h"
#include "marpa_codes.c"
}

#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

typedef std::vector <resid> resids;

resids ask(prover *prover, resid s, const pnode p) {    /* s and p in, a list of o's out */
    resids r = resids();
    prover::termset query;
    prover::termid o_var = prover->tmpvar();
    prover::termid iii = prover->make(p, prover->make(s), o_var);
    query.emplace_back(iii);

    //dout << "query: "<< prover->format(query) << endl;;

    (*prover)(query);

    //dout << "substs: "<< std::endl;

    for (auto x : prover->substs) {
        prover::subst::iterator binding_it = x.find(prover->get(o_var).p);
        if (binding_it != x.end()) {
            r.push_back(prover->get((*binding_it).second).p);
            //prover->prints(x); dout << std::endl;
        }
    }

    //dout << r.size() << ":" << std::endl;

    prover->substs.clear();
    prover->e.clear();
    return r;
}

resids ask(prover *prover, const pnode p, resid o) {
    resids r = resids();
    prover::termset query;
    prover::termid s_var = prover->tmpvar();
    prover::termid iii = prover->make(p, s_var, prover->make(o));
    query.emplace_back(iii);

    //dout << "query: "<< prover->format(query) << endl;;

    (*prover)(query);

    //dout << "substs: "<< std::endl;

    for (auto x : prover->substs) {
        prover::subst::iterator binding_it = x.find(prover->get(s_var).p);
        if (binding_it != x.end()) {
            r.push_back(prover->get((*binding_it).second).p);
            //prover->prints(x); dout << std::endl;
        }
    }

    //dout << r.size() << ":" << std::endl;

    prover->substs.clear();
    prover->e.clear();
    return r;
}

resid ask1(prover *prover, resid s, const pnode p) {
    auto r = ask(prover, s, p);
    if (r.size() > 1)
        throw wruntime_error(L"well, this is weird");
    if (r.size() == 1)
        return r[0];
    else
        return 0;
}

resid ask1(prover *prover, const pnode p, resid o) {
    auto r = ask(prover, p, o);
    if (r.size() > 1)
        throw wruntime_error(L"well, this is weird, more than one match");
    if (r.size() == 1)
        return r[0];
    else
        return 0;
}

std::vector<resid> get_list(prover *prover, resid head, prover::proof& p)
{
    auto r = prover->get_list(prover->make(head), p);
    std::vector<resid> rr;
    for (auto rrr: r)
        rr.push_back(prover->get(rrr).p);
    return rr;
}


const pnode has_value = mkiri(pstr(L"http://idni.org/marpa#has_value"));
const pnode is_parse_of = mkiri(pstr(L"http://idni.org/marpa#is_parse_of"));
const resid pcomma = dict[mkliteral(pstr(L","), pstr(L"XSD_STRING"), pstr(L"en"))];
const pnode rdfs_nil = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
const pnode rdfs_rest = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#rest"));
const pnode rdfs_first = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#first"));
const pnode bnf_matches = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#matches"));
const pnode bnf_document = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#document"));
const pnode bnf_whitespace = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#whitespace"));
const pnode bnf_mustBeOneSequence = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence"));
const pnode bnf_commaSeparatedListOf = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#commaSeparatedListOf"));


typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;
typedef std::vector <sym> syms;
typedef std::pair <string::const_iterator, string::const_iterator> tokt;


class terminal {
public:
    resid thing;
    string name, regex_string;
    boost::wregex regex;

    terminal(resid thing_, string name_, string regex_) {
        name = name_;
        thing = thing_;
        regex_string = regex_;
        regex = boost::wregex(regex_);
    }
};

typedef std::shared_ptr <terminal> pterminal;

struct Marpa {
    Marpa_Grammar g;
    bool precomputed = false;
    map <sym, pterminal> terminals;
    map <sym, string> literals;
    map <resid, sym> done;
    map <rule, sym> rules;
    prover *grmr;
    prover *dest;
    string whitespace = L"";

    resid sym2resid(sym s) {
        for (auto it = done.begin(); it != done.end(); it++)
            if (it->second == s)
                return it->first;
        throw std::runtime_error("this sym isnt for a rule..this shouldnt happen");
    }

    resid rule2resid(rule r) {
        for (auto rr : rules)
            if (rr.first == r) {
                for (auto rrr: done)
                    if (rr.second == rrr.second)
                        return rrr.first;
            }
        throw std::runtime_error("and this shouldnt happen either");
    }

    string sym2str_(sym s) {
        if (literals.find(s) != literals.end())
            return L": \"" + literals[s] + L"\"";
        if (terminals.find(s) != terminals.end())
            return terminals[s]->name + L" - " + terminals[s]->regex_string;
        return *dict[sym2resid(s)].value;
    }

    string sym2str(sym s) {
        std::wstringstream sss;
        sss << L"(" << s << L")" << sym2str_(s);
        return sss.str();
    }

    ~Marpa() {
        marpa_g_unref(g);
    }

    Marpa(prover *_grmr, resid language, prover::proof prf) {
        if (marpa_check_version(MARPA_MAJOR_VERSION, MARPA_MINOR_VERSION, MARPA_MICRO_VERSION) != MARPA_ERR_NONE)
            throw std::runtime_error("marpa version...");
        Marpa_Config marpa_config;
        marpa_c_init(&marpa_config);
        g = marpa_g_new(&marpa_config);
        if (!g) {
            stringstream ssss("marpa_g_new: error ");
            ssss << marpa_c_error(&marpa_config, NULL);
            throw std::runtime_error(ssss.str());
        }
        if (marpa_g_force_valued(g) < 0)
            throw std::runtime_error("marpa_g_force_valued failed?!?!?");

        grmr = dest = _grmr;

        resid whitespace_ = ask1(grmr, language, bnf_whitespace);
        if (whitespace_)
            whitespace = value(whitespace_);

        resid root = ask1(grmr, language, bnf_document);

        sym start = add(grmr, root, prf);

        start_symbol_set(start);
    }

    string value(resid val) {
        return *dict[val].value;
    }

    //create marpa symbols and rules from grammar description in rdf
    sym add(prover *grmr, resid thing, prover::proof prf) {
        string thingv = value(thing);
        dout << "thingv:" << thingv << std::endl;

        if (done.find(thing) != done.end())
            return done[thing];

        if (dict[thing]._type == node::LITERAL) {
            //dout << "itsa str"<<std::endl;
            for (auto t: literals)
                if (t.second == thingv)
                    return t.first;

            sym symbol = symbol_new();
            //dout << "adding " << thingv << std::endl;

            literals[symbol] = thingv;
            return symbol;
        }

        //dout << "adding " << thingv << std::endl;
        sym symbol = symbol_new_resid(thing);

        resid bind;
        if ((bind = ask1(grmr, thing, bnf_matches))) {
            //dout << "terminal: " << thingv << std::endl;
            terminals[symbol] = pterminal(new terminal(thing, thingv, value(bind)));
            return symbol;
        }

        // mustBeOneSequence is a list of lists

        resid ll;
        if ((ll = ask1(grmr, thing, bnf_mustBeOneSequence))) {
            std::vector <resid> lll = get_list(grmr, ll, prf);
            if (!ll)
                throw wruntime_error(L"mustBeOneSequence empty");

            for (auto l:lll) {
                syms rhs;
                std::vector <resid> seq = get_list(grmr, ll, prf);
                for (auto rhs_item: seq)
                    rhs.push_back(add(grmr, rhs_item, prf));
                rule_new(symbol, rhs);
            }
        }
        else if ((ll = ask1(grmr, thing, bnf_commaSeparatedListOf))) {
            seq_new(symbol, add(grmr, ll, prf), add(grmr, pcomma, prf), 0, MARPA_PROPER_SEPARATION);
        }
        else if (thingv == L"http://www.w3.org/2000/10/swap/grammar/bnf#eof") { }//so what?
        else
            throw wruntime_error(L"whats " + thingv + L"?");

        dout << "added sym " << symbol << std::endl;
        return symbol;
    }


    int check_int(int result) {
        if (result == -2)
            error();
        return result;
    }

    void check_null(void *result) {
        if (result == NULL)
            error();
    }

    void error_clear() {
        marpa_g_error_clear(g);
    }

    sym symbol_new() {
        return check_int(marpa_g_symbol_new(g));

    }

    sym symbol_new_resid(resid thing) {
        return done[thing] = symbol_new();

    }

    void rule_new(sym lhs, syms rhs) {
        dout << sym2str(lhs) << L" ::= ";
        for (sym s: rhs)
            dout << sym2str(s);
        dout << std::endl;
        rules[check_int(marpa_g_rule_new(g, lhs, &rhs[0], rhs.size()))] = lhs;
        /*if (r == -2)
        {
            int e = marpa_g_error(g, NULL);
            if (e == MARPA_ERR_DUPLICATE_RULE)
            {
        */
    }

    void seq_new(sym lhs, sym rhs, sym separator, int min, int flags) {
        int r = marpa_g_sequence_new(g, lhs, rhs, separator, min, flags);
        if (r == -2) {
            int e = marpa_g_error(g, NULL);
            if (e == MARPA_ERR_DUPLICATE_RULE)
                dout << sym2str(lhs) << L" ::= sequence of " << sym2str(rhs) << std::endl;
        }
        rules[check_int(r)] = lhs;
    }


    void print_events() {

        int count = check_int(marpa_g_event_count(g));
        if (count > 0) {
            dout << count << " events" << std::endl;
            Marpa_Event e;
            for (int i = 0; i < count; i++) {
                int etype = check_int(marpa_g_event(g, &e, i));
                dout << L" " << etype << ", " << e.t_value << std::endl;
            }
        }

    }

    void start_symbol_set(sym s) {
        check_int(marpa_g_start_symbol_set(g, s));
    }

    void error() {

        int e = marpa_g_error(g, NULL);
        stringstream s;
        s << e << ":" << marpa_error_description[e].name << " - " << marpa_error_description[e].suggested;
        throw(std::runtime_error(s.str()));// + ": " + errors[e]));

    }


    bool is_ws(wchar_t x) {
        string wss = L"\n\r \t";

        for (auto ws: wss)
            if (x == ws)
                return true;
        return false;
    }


    pnode parse(const string inp) {
        if (!precomputed)
            check_int(marpa_g_precompute(g));

        print_events();

        dout << "terminals:\n";
        for (auto t:terminals)
            dout << "(" << t.first << ")" << t.second->name << ": " << t.second->regex << std::endl;

        dout << "tokenizing..\n";

        std::vector <tokt> toks; // ranges of individual tokens within the input string
        toks.push_back(tokt(inp.end(), inp.end()));

        Marpa_Recognizer r = marpa_r_new(g);
        check_null(r);
        marpa_r_start_input(r);

        auto pos = inp.begin();

        std::vector <sym> expected;
        expected.resize(check_int(marpa_g_highest_symbol_id(g)));

        boost::wregex whitespace_regex = boost::wregex(whitespace);

        while (pos < inp.end()) {
            if (is_ws(*pos)) {
                pos++;
                continue;
            }

            boost::wsmatch what;

            if ((whitespace.size()) && (regex_search(pos, inp.end(), what, whitespace_regex, boost::match_continuous))) {
                if (what.size()) {
                    int llll = what[0].length();
                    dout << L"skipping " << llll << L" comment chars" << std::endl;
                    pos += llll;
                    continue;
                }
            }

            std::vector <sym> best_syms;
            size_t best_len = 0;
            expected.clear();
            int num_expected = check_int(marpa_r_terminals_expected(r, &expected[0]));
            for (int i = 0; i < num_expected; i++) {
                sym e = expected[i];
                if (literals.find(e) != literals.end()) {
                    if (boost::starts_with(string(pos, inp.end()), literals[e])) {
                        if (literals[e].size() > best_len) {
                            best_syms.clear();
                            best_syms.push_back(e);
                            best_len = literals[e].size();
                        }
                        else if (literals[e].size() == best_len)
                            best_syms.push_back(e);
                    }
                }
                else {
                    if (terminals.find(e) != terminals.end()) {
                        auto t = terminals[e];

                        if (!regex_search(pos, inp.end(), what, t->regex, boost::match_continuous)) continue;
                        if (what.empty()) continue;
                        size_t l = what.length();
                        if (l > best_len) {
                            best_len = l;
                            best_syms.clear();
                            best_syms.push_back(e);
                        }
                        else if (l == best_len)
                            best_syms.push_back(e);
                    }
                }
            }

            if (best_len) {
                if (best_syms.size() > 1) {
                    dout << L"cant decide between:" << std::endl;
                    for (auto ccc: best_syms)
                        dout << L" " << sym2str(ccc) << std::endl;
                }
                assert(best_syms.size());
                toks.push_back(tokt(pos, pos + best_len));
                dout << std::distance(inp.begin(), pos) << L"-" << std::distance(inp.begin(), pos + best_len) <<
                L" \"" << string(pos, pos + best_len) << L"\" - " << sym2str(best_syms[0]) << std::endl;
                check_int(marpa_r_alternative(r, best_syms[0], toks.size(), 1));
                check_int(marpa_r_earleme_complete(r));
                pos += best_len;
            }
            else {
                auto pre(pos);
                size_t charnum = 0;
                while (pre != inp.begin() && *pre != '\n') {
                    pre -= 1;
                    charnum++;
                }
                //pre -= 10;
                auto post(pos);
                while (post != inp.end() && *post != '\n')
                    post += 1;
                //post += 10;
                dout << L"at line " << std::count(inp.begin(), pos, '\n') << L", char " << charnum << L":" << std::endl;
                dout << string(pre, pos - 1) << L"<HERE>" << string(pos, post) << std::endl;
//                        dout << L"..\"" << string(pre, pos-1) << L"<HERE>" << string(pos, post) << L"\"..." << std::endl;


                dout << "expecting:" << std::endl;
                for (int i = 0; i < num_expected; i++) {
                    sym e = expected[i];
                    dout << sym2str(e) << std::endl;
                }


                throw(std::runtime_error("no parse"));
            }
        }

        dout << "evaluating.." << std::endl;

        //marpa allows variously ordered lists of ambiguous parses, we just grab the default
        Marpa_Bocage b = marpa_b_new(r, -1);
        check_null(b);
        Marpa_Order o = marpa_o_new(b);
        check_null(o);
        Marpa_Tree t = marpa_t_new(o);
        check_null(t);
        check_int(marpa_t_next(t));
        Marpa_Value v = marpa_v_new(t);
        check_null(v);

        //"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched,
        //we build the value/parse/ast in the stack, bottom-up

        map <int, pnode> stack;
        map <int, string> sexp;

        while (1) {
            Marpa_Step_Type st = check_int(marpa_v_step(v));
            print_events();
            if (st == MARPA_STEP_INACTIVE) break;
            switch (st) {
                case MARPA_STEP_TOKEN: {
                    sym symbol = marpa_v_symbol(v);
                    size_t token = marpa_v_token_value(v) - 1;
                    string token_value = string(toks[token].first, toks[token].second);
                    sexp[marpa_v_result(v)] = token_value;
                    pnode xx;
                    if (terminals.find(symbol) != terminals.end()) {
                        xx = mkbnode(jsonld_api::gen_bnode_id());
                        dest->addrules(quad(xx, bnf_matches, dict[terminals[symbol]->thing]));
                        dest->addrules(has_value, xx, mkliteral(pstr(token_value), pstr(L"XSD_STRING"), pstr(L"en"))););
                    }
                    else // it must be a literal
                        xx = mkliteral(pstring(token_value]));
                    stack[marpa_v_result(v)] = xx;
                    break;
                }
                case MARPA_STEP_RULE: {
                    string sexp_str = L"( ";

                    resid res = rule2resid(marpa_v_rule(v));
                    sexp_str += value(res) + L" ";

                    std::vector <pnode> args;

                    for (int i = marpa_v_arg_0(v); i <= marpa_v_arg_n(v); i++) {
                        if (stack[i]) {
                            args.push_back(stack[i]);
                            sexp_str += sexp_str[i] + L" ";
                        }
                    }
                    sexp_str[marpa_v_result(v)] = sexp_str + L") ";


                    pnode xx;
                    // if its a sequence
                    if (check_int(marpa_g_sequence_min(g, marpa_v_rule(v))) != -1)
                        xx = list_to_term(args);
                    else {
                        xx = mkbnode(jsonld_api::gen_bnode_id());
                        int rhs_item_index = 0;
                        for (auto arg: args) {
                            sym arg_sym = check_int(marpa_g_rule_rhs(g, marpa_v_rule(v), rhs_item_index));
                            pnode arg_node;

                            pnode pred = 0;

                            if (literals.find(arg_sym) != literals.end()) { }
                            else if (terminals.find(arg_sym) != terminals.end()) { }
                            else {
                                pred = dict[sym2resid(rhs_sym)];
                            }

                            if (pred)
                                dest->addrules(pquad(quad(pred, xx, arg));


                            wstringstream arg_pred;
                            arg_pred << L"arg" << rhs_item_index;
                            pnode pred2 = mkliteral(pstr(arg_pred.str()), pstr(L"XSD_STRING"), pstr(L"en"));

                            dest->addrules(pquad(quad(pred2, xx, arg));
                        }
                    }

                    dest->addrules(pquad(quad(is_parse_of, xx, res)));

                    stack[marpa_v_result(v)] = xx;

                    break;
                }
                case MARPA_STEP_NULLING_SYMBOL:
                    sexp[marpa_v_result(v)] = L"0";
                    stack[marpa_v_result(v)] = 0;
                    break;
                default:
                    dout << marpa_step_type_description[st].name << std::endl;


            }
        }
        marpa_v_unref(v);
        marpa_t_unref(t);
        marpa_o_unref(o);
        marpa_b_unref(b);

        dout << sexp[0] << std::endl;
        return stack[0];
    }


};


std::string load_n3_cmd::desc() const { return "load n3"; }

std::string load_n3_cmd::help() const {
    stringstream ss("Hilfe! Hilfe!:");
    return ss.str();
}


string load_file(string fname) {
    std::ifstream t(ws(fname));
    if (!t.is_open())
        throw std::runtime_error("couldnt open file");// \"" + fname + L"\"");

    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return ws(str);
}

int load_n3_cmd::operator()(const strings &args) {
    prover prover(*load_quads(L"n3-grammar.nq"));
    pnode parser = ask1(prover, marpa_parser, mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#language")));
    pnode input = mkliteral(pstr(load_file(args[2])), pstr(L"XSD_STRING"), pstr(L"en"));
    std::list l{parser, input};
    resid raw = ask1(prover, marpa_parse, prover->get(prover->list_term(l)));
    if (!raw)
        throw std::runtime_error("oopsie, something went wrong with your tau.");
    //...
    return 0;
}


void *marpa_parser(prover p, resid language, prover::proof prf) {
    return Marpa(prover, language, prf);
}

pnode marpa_parse(void *marpa, string input) {
    return ((Marpa *) marpa)->parse(input);
}



/*
---------------------


?M :marpa_for_grammar http://www.w3.org/2000/10/swap/grammar/n3#language; ?X :parsed (?M ?FN).


->


?X a n3:document
?X n3:declarations [ ("@base" "xxx")  ("@prefix" "xxx" "yyy")  ("@keywords" ("aaa" "bbb" "ccc")) ]
?X n3:universals

?X n3:statements [ 



*/


/*
resources

https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/


