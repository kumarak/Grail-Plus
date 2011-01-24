/*
 * Production.hpp
 *
 *  Created on: Jan 20, 2011
 *      Author: Peter Goodman
 *     Version: $Id$
 *
 * Copyright 2011 Peter Goodman, all rights reserved.
 */

#ifndef FLTL_PRODUCTION_HPP_
#define FLTL_PRODUCTION_HPP_

namespace fltl { namespace lib { namespace cfg {

    // forward declarations
    template <typename, const unsigned short> class StaticProduction;
    template <typename> class DynamicProduction;

    /// production of a grammar
    ///
    /// Note: - the first symbol in a production is a reference counter
    ///         and as such cannot be treated as a normal symbol. this is
    ///         so that, from a string point of view, symbol strings and
    ///         production symbol lists are equivalent.
    ///
    ///       - the second symbol is the length of the production.
    template <typename AlphaT>
    class Production {
    private:

        friend class CFG<AlphaT>;
        friend class Variable<AlphaT>;
        friend class OpaqueProduction<AlphaT>;

        typedef Production<AlphaT> self_type;

        /// productions chain in to a doubly-linked list
        self_type *prev;
        self_type *next;

        /// variable of this production
        Symbol<AlphaT> var;

        /// symbols of this production
        SymbolString<AlphaT> symbols;

        /// reference counter
        uint32_t ref_count;

        /// get the number of symbols in this production
        inline unsigned length(void) const throw() {
            return symbols.length();
        }

        /// check equivalence of two productions
        bool is_equivalent_to(const self_type &that) const throw() {
            if(this == &that) {
                return true;
            }

            return (var == that.var) && (symbols == that.symbols);
        }

        inline static void hold(self_type *prod) throw() {
            assert(
                0 != prod &&
                "Cannot hold non-existant production."
            );

            ++(prod->ref_count);
        }

        inline static void release(self_type *prod) throw() {
            assert(
                0 != prod &&
                "Cannot release non-existant production."
            );

            assert(
                0 != prod->ref_count &&
                "Cannot release invalid production."
            );

            if(0 == --(prod->ref_count)) {
                prod->symbols.clear();
                CFG<AlphaT>::production_allocator->deallocate(prod);
                prod = 0;
            }
        }

    public:

        /// constructor
        Production(void) throw()
            : prev(0)
            , next(0)
            , var()
            , symbols()
            , ref_count(0)
        { }

        /// destructor
        ~Production(void) throw() {
            prev = 0;
            next = 0;
        }

        inline static self_type **get_next_pointer(self_type *self) throw() {
            return &(self->next);
        }
    };

}}}

#endif /* FLTL_PRODUCTION_HPP_ */