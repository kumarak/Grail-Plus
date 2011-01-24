/*
 * StaticStorage.hpp
 *
 *  Created on: Jan 22, 2011
 *      Author: Peter Goodman
 *     Version: $Id$
 *
 * Copyright 2011 Peter Goodman, all rights reserved.
 */

#ifndef FLTL_STATICSTORAGE_HPP_
#define FLTL_STATICSTORAGE_HPP_

#include <cstring>
#include <new>

#include "fltl/include/preprocessor/FORCE_INLINE.hpp"

#include "fltl/include/helper/UnsafeCast.hpp"

namespace fltl { namespace helper {

    /// storage chain meant for static usage where the order of the call of
    /// the static destructors is significant
    ///
    /// works by representing type T as an array of some type, and only
    /// deallocating T when its predecessor in the chain has been
    /// deallocated.
    ///
    /// this is not safe for use on the stack, i.e. it is safe when used
    /// in static memory because the destructor of a storage chain might
    /// be called multiple times, and if it were on the stack then we risk
    /// the data changing or being marked as invalid by a tool like valgrind.
    template <typename T>
    class StorageChain {
    private:

        typedef long double storage_type;
        typedef StorageChain<T> self_type;

        enum {
            OBJECT_SIZE = sizeof(T[2]) / 2,
            STORAGE_SIZE = sizeof(storage_type[2]) / 2,
            NUM_SLOTS = (OBJECT_SIZE + (STORAGE_SIZE - 1)) / STORAGE_SIZE
        };

        storage_type storage[NUM_SLOTS];
        typedef void (deallocate_func_type)(void *);

        void *free_first;
        deallocate_func_type *free_first_func;
        bool is_free;

    public:

        /// top of a chain
        StorageChain(void)
            : free_first(0)
            , is_free(false)
        {
            memset(storage, 0, sizeof(storage_type) * NUM_SLOTS);
        }

        /// initialize this storage into a chain
        template <typename U>
        StorageChain(U &_free_first) throw()
            : free_first(reinterpret_cast<void *>(&_free_first))
            , free_first_func(&U::deallocate_static)
            , is_free(false)
        {
            memset(storage, 0, sizeof(storage_type) * NUM_SLOTS);
            new (reinterpret_cast<void *>(storage)) T();
        }

        /// deallocate this storage chain type
        static void deallocate_static(void *_ptr) throw() {
            self_type *ptr(reinterpret_cast<self_type *>(_ptr));
            ptr->~StorageChain<T>();
        }

        /// destructor, clean up all parents first.
        ~StorageChain(void) throw() {
            if(0 != free_first) {
                free_first_func(free_first);
                free_first = 0;
            }

            if(!is_free) {
                (*this)->~T();
                is_free = true;
            }
        }

        FLTL_FORCE_INLINE T *operator->(void) throw() {
            return helper::unsafe_cast<T *>(&(storage[0]));
        }
    };

}}

#endif /* FLTL_STATICSTORAGE_HPP_ */
