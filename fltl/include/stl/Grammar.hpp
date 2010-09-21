/*
 * Grammar.hpp
 *
 *  Created on: Sep 17, 2010
 *      Author: Peter Goodman
 *     Version: $Id$
 */

#ifndef FLTL_STL_GRAMMAR_HPP_
#define FLTL_STL_GRAMMAR_HPP_

#include <map>
#include <vector>

#include "fltl/include/mpl/Max.hpp"
#include "fltl/include/mpl/Sequence.hpp"
#include "fltl/include/mpl/Query.hpp"
#include "fltl/include/mpl/Unit.hpp"

#include "fltl/include/preprocessor/TEMPLATE_VARIABLE_LIMIT.hpp"
#include "fltl/include/preprocessor/REPEAT_LEFT.hpp"
#include "fltl/include/preprocessor/STATIC_ASSERT.hpp"

#include "fltl/include/trait/PolyadicOperator.hpp"
#include "fltl/include/trait/Uncopyable.hpp"
#include "fltl/include/trait/Query.hpp"

#include "fltl/include/stl/BlockAllocator.hpp"

#define FLTL_GRAMMAR_GET_OPERATOR_ARITY(n, _) \
    , (GrammarOperatorArity<\
           typename op_sequence_t::template At<n>::type_t \
       >::VALUE)

namespace fltl { namespace stl {

    namespace {

        /// the arity of an operator that a grammar will use. this is used to
        /// figure out what the maximum arity of operators used in the
        /// grammar is.
        template <typename T>
        class GrammarOperatorArity {
        public:
            enum {
                VALUE = T::ARITY
            };
        };

        /// operator types are stored in a sequence, which fills empty space
        /// with the Unit type. The unit type is obviously not an operator,
        /// so we wrap it up and pretend that it's a nullary operator.
        template <>
        class GrammarOperatorArity<mpl::Unit> {
        public:
            enum {
                VALUE = 0
            };
        };
    }

    /// types for queries
    namespace grammar { namespace query {

        //template <typename T>

    }}

    /// base grammar type.
    ///
    /// The base grammar template type imposes the following restrictions
    /// on its parameterized types:
    ///
    ///     - Parameter types must have default constructors, copy
    ///       constructors, and copy assignment operators. Terminal and
    ///       Non-terminal types are expected to behave with value semantics.
    ///     - The default construction of AcceptNonTermT must be the epsilon
    ///       non-terminal that accepts an empty word. By default,
    ///       AcceptNonTermT is NonTermT.
    ///     - the terminal and non-terminal types must have a well-defined
    ///       strict weak ordering. It does not matter if the orderings
    ///       imposed are not meaningful.
    ///
    /// The base grammar template type assumes the following:
    ///
    ///     - disjunction (defined by DisjunctionOperatorT) is a
    ///       commutative, associative, binary operator
    ///     - disjunction does not distribute over any of the non-disjunction
    ///       operators in OperatorSequenceT.
    ///     - disjunction has the lowest precedence of all operators used
    ///       in defining rewrite rules and so productions can be "split"
    ///       by occurrences of the disjunction operator without changing
    ///       the semantics of the grammar. The following transformation
    ///       must maintain the language generated by the grammar for all
    ///       valid rewrite rules W, X, Y, and Z:
    ///
    ///                  A : W | X   <==>  A  : W | $A | X
    ///                    | Y | Z         $A : X | Y
    ///
    template <
        // terminal type, e.g. character, token, etc.
        typename TermT,

        // non-terminal type, i.e. a rewrite rule type / production
        typename NonTermT,

        // disjunction operator; the grammar needs to know about disjunction.
        typename DisjunctionOperatorT,

        // sequence of unary and binary operators defined over TermT
        // and NonTermT.
        typename OperatorSequenceT,

        // basic allocator that the grammar type will use for allocating
        // internal expression trees.
        template<typename> class AllocatorT = stl::Block<1024>::Allocator
    >
    class Grammar : private trait::Uncopyable {
    private:

        typedef Grammar<
            TermT,
            NonTermT,
            DisjunctionOperatorT,
            OperatorSequenceT,
            AllocatorT
        > self_t;

        /// represents an identity operator for symbols,
        /// i.e. (non-)terminals
        class TerminalOperator : public trait::PolyadicOperator<1> { };
        class NonTerminalOperator : public trait::PolyadicOperator<1> { };

        /// make sure the disjunction and symbol operators are in the
        /// operator type sequence, and make sure that all types in the
        /// sequence are distinct.
        typedef typename OperatorSequenceT::UniqueTypes::type_t:: \
                template Insert<DisjunctionOperatorT>::type_t:: \
                template Insert<TerminalOperator>::type_t:: \
                template Insert<NonTerminalOperator>::type_t op_sequence_t;

        enum {

            /// number of operators, including disjunction and symbol
            /// identity
            NUM_OPERATORS = op_sequence_t::Length::VALUE,

            /// the maximum arity of all of the operators
            MAX_OPERATOR_ARITY = mpl::Max<
                NonTerminalOperator::ARITY
                FLTL_REPEAT_LEFT(
                    FLTL_TEMPLATE_VARIABLE_LIMIT,
                    FLTL_GRAMMAR_GET_OPERATOR_ARITY,
                    void
                )
            >::VALUE
        };

        /// expression tree representation for a grammar rule.
        class Expression {
        public:
            unsigned operator_id;
            unsigned symbol_id;
            Expression *nodes[MAX_OPERATOR_ARITY];
        };

        /// the next id to be assigned for (non-)terminals.
        unsigned next_symbol_id[2];

        /// mappings of (non-)terminals to identifiers. the sets of
        /// mapped identifiers by these maps are disjoint
        std::map<TermT, const unsigned> terminal_map;
        std::map<NonTermT, const unsigned> non_terminal_map;

        /// mappings of non-terminals to expressions. this vector represents
        /// a forest, wherein each tree represents all rules for a particular
        /// non-terminal.
        std::vector<Expression *> rules;

        /// expression allocator
        AllocatorT<Expression> allocator;

    protected:

        /// class used for building up a single rewrite rule at runtime.
        /// the builder expects to be fed symbols (terminals, non-termianls)
        /// and operators in reverse-polish notation, i.e operands precede
        /// operators.
        class RuntimeProductionBuilder : private trait::Uncopyable {
        private:

            friend class Grammar<
                TermT,
                NonTermT,
                DisjunctionOperatorT,
                OperatorSequenceT,
                AllocatorT
            >;

            /// the grammar that owns this production builder
            self_t &owner;

            /// private constructor so that only a grammar can make
            /// production builders.
            RuntimeProductionBuilder(self_t &owned_by) throw()
             : owner(owned_by) { }

        public:

            ~RuntimeProductionBuilder(void) throw() { }

            /// notify the production builder that stack can be collapsed
            /// by an operator
            template <typename RuleOperatorT>
            void collapse(void) {

                // make sure that the operator type passed in is actually
                // a valid operator for this grammar
                FLTL_STATIC_ASSERT(mpl::SizeOf<typename
                    op_sequence_t::template Select<RuleOperatorT>::type_t
                >::VALUE > 0);


            }

            void addTerminal(const TermT &symbol) {
                (void) symbol;
            }

            void addNonTerminal(const NonTermT &symbol) {
                (void) symbol;
            }
        };

        /// exposes expressions to subclasses as opaque types.
        typedef Expression *expression_t;

        /// sub-classes of grammar have access to the builder in order to
        /// be able to build grammar from user input
        RuntimeProductionBuilder builder;

    public:

        /// default constructor
        Grammar(void)
         : terminal_map()
         , non_terminal_map()
         , rules()
         , allocator()
         , builder(*this) { }
    };


}}

namespace fltl { namespace trait {

    template <
        typename TermT,
        typename NonTermT,
        typename DisjunctionOperatorT,
        typename OperatorSequenceT,
        template<typename> class AllocatorT
    >
    class Query<
        stl::Grammar<
            TermT,NonTermT,DisjunctionOperatorT,
            OperatorSequenceT,AllocatorT
        >
    > {

    };
}}

#endif /* FLTL_STL_GRAMMAR_HPP_ */
