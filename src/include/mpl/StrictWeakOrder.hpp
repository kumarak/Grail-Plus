/*
 * StrictWeakOrder.hpp
 *
 *  Created on: Sep 14, 2010
 *      Author: Peter Goodman
 *     Version: $Id$
 */

#ifndef CFTL_MPL_STRICTWEAKORDER_HPP_
#define CFTL_MPL_STRICTWEAKORDER_HPP_

namespace cftl { namespace mpl {

    /// Less-than operator for some type. Defines a strict weak ordering
    /// for elements of that type.
    template <typename T>
    class StrictWeakOrder {
    public:
        bool operator()(const T &left, const T &right) const {
            return left < right;
        }
    };

    template <typename T>
    class StrictWeakOrder<T &> {
    public:
        bool operator()(const T &left, const T &right) const {
            return left < right;
        }
    };
}}

#endif /* CFTL_MPL_STRICTWEAKORDER_HPP_ */
