/*
 * Transition.hpp
 *
 *  Created on: Feb 26, 2011
 *      Author: Peter Goodman
 *     Version: $Id$
 *
 * Copyright 2011 Peter Goodman, all rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FLTL_TRANSITION_HPP_
#define FLTL_TRANSITION_HPP_

namespace fltl { namespace pda {

    template <typename AlphaT>
    class Transition {
    private:

        typedef Transition<AlphaT> self_type;
        typedef Symbol<AlphaT> symbol_type;
        typedef OpaqueState<AlphaT> state_type;

        friend class PDA<AlphaT>;
        friend class OpaqueTransition<AlphaT>;
        friend class TransitionGenerator<AlphaT>;
        friend class Pattern<AlphaT>;

        template <
            typename, typename,
            typename, typename,
            typename, typename
        > friend class PatternGenerator;

        template <typename, typename, typename>
        friend class pda::detail::FindNextTransition;

        template <typename, typename>
        friend class detail::ResetPatternGenerator;

        /// reference count
        unsigned ref_count;

        /// the symbol read from input
        symbol_type sym_read;

        /// the symbol that must be on the top of the stack for this
        /// transition to be followed. this symbol is popped off of
        /// the stack.
        symbol_type sym_pop;

        /// symbol to push onto the stack after the transition has been
        /// followed
        symbol_type sym_push;

        /// the next transition that shares the same source state.
        self_type *next;
        self_type *prev;

        /// source and sink states, respectively.
        state_type source_state;
        state_type sink_state;

        /// was this transition deleted?
        bool is_deleted;

        /// the PDA of this production
        PDA<AlphaT> *pda;

        static void hold(self_type *trans) throw() {
            assert(0 != trans);
            ++(trans->ref_count);
        }

        static void release(self_type *trans) throw() {
            assert(0 != trans);
            if(0 == --(trans->ref_count)) {

                if(0 != trans->pda) {

                    if(0 != trans->next) {
                        trans->next->prev = trans->prev;
                    }

                    if(0 != trans->prev) {
                        trans->prev->next = trans->next;
                    } else {

                        //assert(false);
                        trans->pda->state_transitions.set(
                            trans->source_state.id,
                            trans->next
                        );
                    }
                }

                PDA<AlphaT>::transition_allocator->deallocate(trans);
            }
        }

        bool operator==(const self_type &that) const throw() {
            return source_state == that.source_state
                && sink_state == that.sink_state
                && sym_read == that.sym_read
                && sym_pop == that.sym_pop
                && sym_push == that.sym_push;
        }

        bool operator<(const self_type &that) const throw() {
            if(source_state < that.source_state) {
                return true;
            } else if(source_state > that.source_state) {
                return false;
            }

            if(sym_read < that.sym_read) {
                return true;
            } else if(sym_read > that.sym_read) {
                return false;
            }

            if(sym_pop < that.sym_pop) {
                return true;
            } else if(sym_pop > that.sym_pop) {
                return false;
            }

            if(sym_push < that.sym_push) {
                return true;
            } else if(sym_push > that.sym_push) {
                return false;
            }

            if(sink_state < that.sink_state) {
                return true;
            }

            return false;
        }

    public:

        Transition(void) throw()
            : ref_count(0)
            , next(0)
            , prev(0)
            , is_deleted(false)
            , pda(0)
        { }

        ~Transition(void) throw() {
            pda = 0;
            next = 0;
            prev = 0;
        }
    };

}}

#endif /* FLTL_TRANSITION_HPP_ */
