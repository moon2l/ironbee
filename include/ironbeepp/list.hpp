/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee++ &mdash; List
 *
 * This file defines (Const)List, a wrapper for ib_list_t.
 *
 * @remark Developers should be familiar with @ref ironbeepp to understand
 * aspects of this code, e.g., the public/non-virtual inheritance.
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __IBPP__LIST__
#define __IBPP__LIST__

#include <ironbeepp/common_semantics.hpp>
#include <ironbeepp/exception.hpp>
#include <ironbeepp/memory_pool.hpp>
#include <ironbeepp/internal/throw.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <ostream>

#include <ironbee/list.h>

namespace IronBee {

/// @cond Internal
namespace Internal {

/**
 * Iterate through an ib_list_t, casting values to pointers.
 *
 * This template implements non-mutable bidirectional iterators for ib_list_t,
 * where the values are cast to a pointer type, @a T, via reinterpret_cast
 * (contrast to list_const_iterator).
 *
 * It is generated by List and ConstList when their value type is a pointer.
 *
 * Note that the reference type for this iterator is @a T not @c const @a T&.
 *
 * @sa list_const_iterator
 * @sa List
 * @sa ConstList
 *
 * @tparam T Value to cast @c void* list value to.
 **/
template <typename T>
class pointer_list_const_iterator :
    public boost::iterator_facade<
        pointer_list_const_iterator<T>,
        T,
        boost::bidirectional_traversal_tag,
        T
    >
{
public:
    //! Construct from ib_list_node_t.
    explicit
    pointer_list_const_iterator(ib_list_node_t* n) :
        m_node(n)
    {
        init_node(m_past_the_end);
        init_node(m_before_the_beginning);
    }

    //! Default constructor; all operations except assignment undefined.
    pointer_list_const_iterator() :
        m_node(NULL)
    {
        init_node(m_past_the_end);
        init_node(m_before_the_beginning);
    }

    //! Copy constructor.
    pointer_list_const_iterator(
        const pointer_list_const_iterator& other
    )
    {
        init_node(m_past_the_end);
        init_node(m_before_the_beginning);

        if (other.m_node == &other.m_before_the_beginning) {
            m_before_the_beginning.next = other.m_before_the_beginning.next;
            m_node = &m_before_the_beginning;
        }
        else if (other.m_node == & other.m_past_the_end) {
            m_past_the_end.prev = other.m_past_the_end.prev;
            m_node = &m_past_the_end;
        }
        else {
            m_node = other.m_node;
        }
    }

    //! Provide underlying ib_list_node_t.
    ib_list_node_t* node() const
    {
        if (m_node != &m_past_the_end && m_node != &m_before_the_beginning) {
            return m_node;
        }
        else {
            return NULL;
        }
    }

private:
    friend class boost::iterator_core_access;

    //! Initialize @a n to all NULLs.
    static void init_node(ib_list_node_t& n)
    {
        n.next = NULL;
        n.prev = NULL;
        n.data = NULL;
    }

    //! Increment iterator.
    void increment()
    {
        if (m_node != &m_past_the_end) {
            if (m_node->next) {
                m_node = m_node->next;
            }
            else {
                // moving past end
                m_past_the_end.prev = m_node;
                m_node = &m_past_the_end;
            }
        }
    }

    //! Decrement iterator.
    void decrement()
    {
        if (m_node != &m_before_the_beginning) {
            if (m_node->prev) {
                m_node = m_node->prev;
            }
            else {
                // moving before beginning
                m_before_the_beginning.next = m_node;
                m_node = &m_before_the_beginning;
            }
        }
    }

    //! Compare iterators.
    bool equal(
        const pointer_list_const_iterator& other
    ) const
    {
        return
            (m_node == other.m_node) ||
            (
                m_node == &m_before_the_beginning &&
                other.m_node == &other.m_before_the_beginning
            ) ||
            (
                m_node == &m_past_the_end &&
                other.m_node == &other.m_past_the_end
            )
            ;
    }

    //! Dereference iterator.  Note return of copy.
    T dereference() const
    {
        return reinterpret_cast<T>(m_node->data);
    }

    ib_list_node_t  m_before_the_beginning;
    ib_list_node_t  m_past_the_end;
    ib_list_node_t* m_node;
};

/**
 * Iterate through an ib_list_t, casting values to IronBee++ objects.
 *
 * This template implements non-mutable bidirectional iterators for ib_list_t,
 * where the values are converted to IronBee++ objects.  Recall that IronBee++
 * objects behave much like pointers.
 *
 * It is generated by List and ConstList when their value type is not a
 * pointer.
 *
 * Note that the reference type for this iterator is @a T not @c const @a T&.
 *
 * @sa pointer_list_const_iterator
 * @sa List
 * @sa ConstList
 *
 * @remark This template is adapted from pointer_list_iterator<T::ib_type>.
 *
 * @tparam T Value to convert list value to.  Must define @c ib_type member
 *           typedef and a constructor from @c ib_type.
 **/
template <typename T>
class list_const_iterator :
    public boost::iterator_adaptor<
        list_const_iterator<T>,
        pointer_list_const_iterator<typename T::ib_type>,
        T,
        boost::bidirectional_traversal_tag,
        T // Note: Copy not reference.
    >
{
public:
    //! Default constructor; all operations except assignment undefined.
    list_const_iterator() :
        list_const_iterator::iterator_adaptor_(0)
    {
        // nop
    }

    //! Construct from ib_list_node_t.
    explicit
    list_const_iterator(
        ib_list_node_t* n
    )  :
        list_const_iterator::iterator_adaptor_(
            pointer_list_const_iterator<typename T::ib_type>(n)
        )
    {
        // nop
    }

    //! Copy constructor.
    list_const_iterator(
        const list_const_iterator& other
    ) :
        list_const_iterator::iterator_adaptor_(other.base())
    {
        // nop
    }

    //! Provide underlying ib_list_node_t.
    ib_list_node_t* node() const
    {
        return this->base().node();
    }

private:
    friend class boost::iterator_core_access;

    //! Operator-> support.
    T* operator->() const
    {
        static T m_dummy;
        m_dummy = T(*this->base());
        return &m_dummy;
    }

    //! Dereference.  Note returns copy not reference.
    T dereference() const
    {
        return T(*this->base());
    }
};

/**
 * Metafunction to calculate appropriate iterator type.
 *
 * If @a T is a pointer then pointer_list_const_iterator is used, otherwise
 * list_const_iterator is used.
 *
 * @tparam T Desired value type of iterator.
 **/
template <typename T>
struct make_list_const_iterator
{
    typedef list_const_iterator<T> type;
};

//! Overload of previous to make it work for pointers.
template <typename Base>
struct make_list_const_iterator<Base*>
{
    typedef pointer_list_const_iterator<Base*> type;
};

/**
 * Converts pointers to themselves and IronBee objects to their ib().
 **/
template <typename T>
void* value_as_void(T v)
{
    return v.ib();
}

//! Overload of previous to make it work for pointers.
template <typename Base>
void* value_as_void(Base* v)
{
    return reinterpret_cast<void*>(
        const_cast<typename boost::remove_const<Base>::type*>(v)
    );
}

} // Internal
/// @endcond

template <typename T>
class ConstList;

template <typename T>
class List;

/**
 * Metafunction to determine if @a T is a ConstList or List.
 *
 * Inherits from true_type if @a T is a ConstList or List and false_type
 * otherwise.
 *
 * @tparam T Type to test.
 **/
template <typename T>
struct is_list : public boost::false_type {};
//! Implementation detail of is_list.
template <typename U>
struct is_list<ConstList<U> > : public boost::true_type {};
//! Implementation detail of is_list.
template <typename U>
struct is_list<List<U> > : public boost::true_type {};

/**
 * Const List; equivalent to a const pointer to ib_list_t.
 *
 * Provides operators ==, !=, <, >, <=, >= and evaluation as a boolean for
 * singularity via CommonSemantics.
 *
 * See List for discussion of lists.
 *
 * @tparam T Value type for list.
 *
 * @sa List
 * @sa ironbeepp
 * @sa ib_list_t
 * @nosubgrouping
 **/
template <typename T>
class ConstList :
    public CommonSemantics<ConstList<T> >
{
public:
    //! C Type.
    typedef const ib_list_t* ib_type;

    /**
     * Construct singular ConstList.
     *
     * All behavior of a singular ConstList is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    ConstList() :
        m_ib(NULL)
    {
        // nop
    }

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! const ib_list_t accessor.
    // Intentionally inlined.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct List from ib_list_t.
    explicit
    ConstList(ib_type ib_list) :
        m_ib(ib_list)
    {
        // nop
    }

    ///@}

    /**
     * @name STL types.
     *
     * These typedefs provide useful information and compatibility with STL
     * oriented code.
     *
     * Note that iterator and const_iterator are identical (similar to
     * std::set) and that reference is the same as value_type (i.e., iterators
     * dereference to copy).
     *
     * In addition to the usual iterator operations, all iterators provide a
     * node() method that returns the underlying @c ib_list_node_t* or NULL
     * for iterators that do not refer to actual list nodes, e.g., end().
     **/
    ///@{
    //! Iterator; bidirectional input iterator.
    typedef typename Internal::make_list_const_iterator<T>::type iterator;
    //! Const iterator; same as iterator.
    typedef iterator const_iterator;
    //! Reference; same as value_type.
    typedef typename iterator::reference reference;
    //! Const reference; same as reference, i.e., value_type.
    typedef reference const_reference;
    //! Size type.
    typedef size_t size_type;
    //! Different type.
    typedef typename iterator::difference_type difference_type;
    //! Value type, i.e., @a T.
    typedef T value_type;
    //! Reverse iterator; bidirectional input iterator.
    typedef std::reverse_iterator<iterator> reverse_iterator;
    //! Const reverse iterator; same as reverse_iterator.
    typedef reverse_iterator const_reverse_iterator;
    ///@}

    //! Iterator to beginning of list.
    iterator begin() const
    {
        return iterator(IB_LIST_FIRST(ib()));
    }

    //! Iterator just past end of list.
    iterator end() const
    {
        if (IB_LIST_ELEMENTS(ib()) == 0) {
            return begin();
        }
        else {
            return boost::next(iterator(IB_LIST_LAST(ib())));
        }
    }

    //! Reverse iterator at last element of list.
    reverse_iterator rbegin() const
    {
        return reverse_iterator(end());
    }

    //! Reverse iterator just past beginning of list.
    reverse_iterator rend() const
    {
        return reverse_iterator(begin());
    }

    //! True iff list is empty.
    bool empty() const
    {
        return ! IB_LIST_FIRST(ib());
    }

    //! Number of elements in list. O(1)
    size_type size() const
    {
        return ib_list_elements(ib());
    }

    /**
     * Front element of list.
     *
     * @return Front element of list.
     * @throw enoent if list is empty.
     **/
    T front() const
    {
        if (empty()) {
            BOOST_THROW_EXCEPTION(
              enoent() << errinfo_what(
                "Trying to fetch front of empty list."
              )
            );

        }
        return *begin();
    }

    /**
     * Back element of list.
     *
     * @return  Back element of list.
     * @throw enoent if list is empty.
     **/
    T back() const
    {
        if (empty()) {
            BOOST_THROW_EXCEPTION(
              enoent() << errinfo_what(
                "Trying to fetch back of empty list."
              )
            );

        }
        return *rbegin();
    }

    //! Memory pool used by list.
    MemoryPool memory_pool() const
    {
        return ib()->m_pool;
    }

private:
    ib_type m_ib;
};

/**
 * List; equivalent to a pointer to ib_list_t.
 *
 * Lists can be treated as ConstLists.  See @ref ironbeepp for
 * details on IronBee++ object semantics.
 *
 * IronBee lists are lists of generic pointers (@c void*).  This template
 * provides an interface to an IronBee list that assumes all values are the
 * same actual type, @a T.
 *
 * Only a subset of the C API operations are provided.  Additional operations
 * may be added in the future as needed.  Of that subset, many operations are
 * renamed.  In general, this template and ConstList attempt to provide a
 * std::list like interface rather than the ib_list_t interface.
 *
 * @sa ironbeepp
 * @sa ib_list_t
 * @sa ConstList
 * @nosubgrouping
 **/
template <typename T>
class List :
    public ConstList<T>
{
public:
    typedef ib_list_t* ib_type;

    /**
     * Remove the constness of a ConstList
     *
     * @warning This is as dangerous as a @c const_cast, use carefully.
     *
     * @param[in] list ConstList to remove const from.
     * @returns List pointing to same underlying list as @a list.
     **/
    static List remove_const(ConstList<T> list)
    {
        return List(const_cast<ib_type>(list.ib()));
    }

    /**
     * Construct singular List.
     *
     * All behavior of a singular List is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    List() :
        m_ib(NULL)
    {
        // nop
    }

    /**
     * Create new list.
     *
     * Creates a new empty list using @a memory_pool for memory.
     *
     * @param[in] memory_pool Memory pool to use.
     * @return Empty List.
     **/
    static List create(MemoryPool memory_pool)
    {
        ib_list_t* ib_list;
        Internal::throw_if_error(ib_list_create(&ib_list, memory_pool.ib()));
        return List(ib_list);
    }

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! ib_list_t accessor.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct List from ib_list_t.
    explicit
    List(ib_type ib_list) :
        ConstList<T>(ib_list),
        m_ib(ib_list)
    {
        // nop
    }

    ///@}

    /**
     * Push @a value to back of list.
     *
     * @param[in] value Value to push to back of list.
     **/
    void push_back(T value) const
    {
        Internal::throw_if_error(
            ib_list_push(m_ib, Internal::value_as_void(value))
        );
    }

    /**
     * Push @a value to front of list.
     *
     * @param[in] value Value to push to back of list.
     **/
    void push_front(T value) const
    {
        Internal::throw_if_error(
            ib_list_enqueue(m_ib, Internal::value_as_void(value))
        );
    }


    //! Pop last element off list.
    void pop_back() const
    {
        Internal::throw_if_error(ib_list_pop(m_ib, NULL));
    }

    //! Pop first element off list.
    void pop_front() const
    {
        Internal::throw_if_error(ib_list_shift(m_ib, NULL));
    }

    //! Clear list.
    void clear() const
    {
        ib_list_clear(m_ib);
    }

private:
    ib_type m_ib;
};

} // IronBee

#endif