/*
 * CFG_TO_LL1.hpp
 *
 *  Created on: Jan 25, 2012
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef Grail_Plus_CFG_TO_LL1_HPP_
#define Grail_Plus_CFG_TO_LL1_HPP_

#include <vector>
#include <utility>
#include <map>

#include "fltl/include/CFG.hpp"

#include "grail/include/cfg/compute_null_set.hpp"
#include "grail/include/cfg/compute_first_set.hpp"
#include "grail/include/cfg/compute_follow_set.hpp"

#include "grail/include/io/CommandLineOptions.hpp"
#include "grail/include/io/fread_cfg.hpp"
#include "grail/include/io/error.hpp"

namespace grail { namespace cli {

    template <typename AlphaT>
    class CFG_TO_LL1 {
    public:

        FLTL_CFG_USE_TYPES(fltl::CFG<AlphaT>);

        static const char * const TOOL_NAME;

        static void declare(io::CommandLineOptions &opt, bool in_help) throw() {
            //io::option_type in(opt.declare("stdin", io::opt::OPTIONAL, io::opt::NO_VAL));
            if(!in_help) {
                opt.declare_min_num_positional(1);
                opt.declare_max_num_positional(1);
            }
        }

        static void help(void) throw() {
            //  "  | |                              |                                             |"
            printf(
                "  %s:\n"
                "    Computes the LL1 parser table for a Context-free Grammar (CFG) and outputs\n"
                "    a C++ program capable of parsing the language generated by the grammar, or\n"
                "    potentially a subset of the language because of first/first and first/follow\n"
                "    conflicts.\n\n"
                "  basic use options for %s:\n"
                "    <file>                         read in a CFG from <file>.\n\n",
                TOOL_NAME, TOOL_NAME
            );
        }

        static bool in_follow(
            std::vector<std::vector<bool> *> &follow,
            variable_type V, terminal_type a
        ) throw() {
            return (follow[V.number()])->at(a.number());
        }

        static void add_to_table(
            cfg_type &cfg,
            std::map<production_type, unsigned> &production_ids,
            std::vector<production_type> &ids_to_productions,
            std::map<std::pair<unsigned, unsigned>, unsigned> &table,
            variable_type V, terminal_type a, production_type p
        ) throw() {
            std::pair<unsigned, unsigned> cell(V.number(), a.number());

            if(table.count(cell)) {
                production_type conflict(ids_to_productions[table[cell]]);

                io::warning(
                    "The following two productions conflict when trying to decide "
                    "which production of %s to parse on input %s. The latter production "
                    "has been chosen.\n",
                    cfg.get_name(V),
                    cfg.get_name(a)
                );

                fprintf(stdout, "    ");
                io::fprint(stdout, cfg, p);

                fprintf(stdout, "    ");
                io::fprint(stdout, cfg, conflict);

            } else {
                table[cell] = production_ids[p];
            }
        }

        static bool all_nullable(std::vector<bool> &nullable, symbol_string_type ss) throw() {
            for(unsigned i(0); i < ss.length(); ++i) {
                if(ss.at(i).is_terminal()) {
                    return false;
                }

                variable_type v(ss.at(i));
                if(!nullable[v.number()]) {
                    return false;
                }
            }
            return true;
        }

        static int main(io::CommandLineOptions &options) throw() {

            using fltl::CFG;

            // run the tool
            FILE *fp(0);
            io::option_type file(options[0U]);
            const char *file_name(file.value());
            fp = fopen(file_name, "r");

            if(0 == fp) {

                options.error(
                    "Unable to open file containing context-free "
                    "grammar for reading."
                );
                options.note("File specified here:", file);

                return 1;
            }

            int ret(0);
            cfg_type cfg;

            std::map<production_type, unsigned> production_ids;
            std::vector<production_type> ids_to_productions;
            std::map<std::pair<unsigned, unsigned>, unsigned> table;

            std::vector<bool> nullable;
            std::vector<std::vector<bool> *> first;
            std::vector<std::vector<bool> *> follow;

            // add numberings to the productions
            production_type prod;
            generator_type productions(cfg.search(~prod));

            // build the table, report errors as warnings and resolve in some
            // way or another
            variable_type A;
            terminal_type a;
            symbol_string_type w;
            generator_type As(cfg.search(~A));
            generator_type as(cfg.search(~a));
            generator_type A_related(cfg.search(~prod, A --->* ~w));

            // empty set of all terminals
            std::vector<bool> empty_set(cfg.num_terminals() + 2, false);

            // can't bring in the cfg :(
            if(!io::fread(fp, cfg, file_name)) {
                ret = 1;
                goto done;
            }

            grail::cfg::compute_null_set(cfg, nullable);
            grail::cfg::compute_first_set(cfg, nullable, first);
            grail::cfg::compute_follow_set(cfg, first, follow);

            // map productions to integers
            for(unsigned prod_id(0); productions.match_next(); ++prod_id) {
                production_ids[prod] = prod_id;
                ids_to_productions.push_back(prod);
            }

            for(; As.match_next(); ) {
                for(as.rewind(); as.match_next(); ) {
                    for(A_related.rewind(); A_related.match_next(); ) {

                        // easy case
                        if(w.is_empty()) {
                            if(in_follow(follow, A, a)) {
                                add_to_table(cfg, production_ids, ids_to_productions, table, A, a, prod);
                            }

                        // tricky case, need to check nullability
                        } else if(w.at(0).is_variable()) {

                            variable_type W(w.at(0));
                            std::vector<bool> *check_set(&empty_set);

                            // succeed quickly
                            if(!nullable[W.number()]) {
                                check_set = first[W.number()];

                            } else if(all_nullable(nullable, w)){
                                check_set = follow[W.number()];
                            }

                            if(check_set->at(a.number())) {
                                add_to_table(cfg, production_ids, ids_to_productions, table, A, a, prod);
                            }

                        } else if(w.at(0) == a) {
                            add_to_table(cfg, production_ids, ids_to_productions, table, A, a, prod);
                        }
                    }
                }
            }

        done:
            fclose(fp);

            // clean up

            production_ids.clear();
            ids_to_productions.clear();
            ids_to_productions.resize(0);

            for(unsigned i(0); i < first.size(); ++i) {
                delete first[i];
                delete follow[i];

                first[i] = 0;
                follow[i] = 0;
            }

            first.clear();
            follow.clear();

            return ret;
        }
    };

    template <typename AlphaT>
    const char * const CFG_TO_LL1<AlphaT>::TOOL_NAME("cfg-to-ll1");
}}

#endif /* Grail_Plus_CFG_TO_LL1_HPP_ */
