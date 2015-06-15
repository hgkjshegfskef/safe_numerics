#ifndef BOOST_NUMERIC_SAFE_BASE_HPP
#define BOOST_NUMERIC_SAFE_BASE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include <type_traits> // is_integral

#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/and.hpp>

// don't use constexpr so we can debug
#define SAFE_NUMERIC_CONSTEXPR constexpr

#include "safe_compare.hpp"

namespace boost {
namespace numeric {

template<
    class Stored,
    class Derived,
    class P, // promotion policy
    class E  // exception policy
>
class safe_base {
    SAFE_NUMERIC_CONSTEXPR const Derived &
    derived() const {
        return static_cast<const Derived &>(*this);
    }
    Stored m_t;
protected:
    // note: Rule of Three.  Don't specify custom move, copy etc.
    ////////////////////////////////////////////////////////////
    // constructors
    // default constructor
    SAFE_NUMERIC_CONSTEXPR safe_base() {}

    // copy constructor 
    SAFE_NUMERIC_CONSTEXPR safe_base(const safe_base & t) :
        m_t(t.m_t)
    {}
    template<class T>
    SAFE_NUMERIC_CONSTEXPR safe_base(T & t) :
        m_t(t)
    {
        // verify that this is convertible to the storable type
        static_assert(
            std::is_convertible<T, Stored>::value,
            "Constructor argument is convertible to the storable type"
        );
        if(! derived().validate(t)){
            E::range_error(
                "Invalid value"
            );
        }
    }

    bool validate() const {
        if(! derived().validate(m_t)){
            E::range_error(
                "Invalid value"
            );
        }
    }

public:
    // used to implement stream i/o operators
    Stored & get_stored_value() {
        return m_t;
    }
    SAFE_NUMERIC_CONSTEXPR const Stored & get_stored_value() const {
        return m_t;
    }
    
    /////////////////////////////////////////////////////////////////
    // modification binary operators
    template<class T>
    Derived & operator=(const T & rhs){
        if(! derived().validate(rhs)){
            E::range_error(
                "Invalid value passed on assignment"
            );
        }
        m_t = rhs;
        return derived();
    }
    template<class T>
    Derived & operator+=(const T & rhs){
        // validate?
        m_t = derived() + rhs;
        return derived();
    }
    template<class T>
    Derived & operator-=(const T & rhs){
        *this = *this - rhs;
        return derived();
    }
    template<class T>
    Derived & operator*=(const T & rhs){
        *this = *this * rhs;
        return derived();
    }
    template<class T>
    Derived & operator/=(const T & rhs){
        *this = *this / rhs;
        return derived();
    }
    template<class T>
    Derived & operator%=(const T & rhs){
        *this = *this % rhs;
        return derived();
    }
    template<class T>
    Derived & operator|=(const T & rhs){
        *this = *this | rhs;
        return derived();
    }
    template<class T>
    Derived & operator&=(const T & rhs){
        *this = *this & rhs;
        return derived();
    }
    template<class T>
    Derived & operator^=(const T & rhs){
        *this = *this * rhs;
        return derived();
    }
    template<class T>
    Derived & operator>>=(const T & rhs){
        *this = *this >> rhs;
        return derived();
    }
    template<class T>
    Derived & operator<<=(const T & rhs){
        *this = *this << rhs;
        return derived();
    }
    // unary operators
    Derived operator++(){
        // this checks for overflow
        *this = *this + 1;
        return derived();
    }
    Derived operator--(){
        // this checks for overflow
        *this = *this - 1;
        return derived();
    }
    Derived operator++(int){ // post increment
        Stored t = m_t;
        if(! derived().validate(*this + 1)){
            E::overflow_error(
                "Overflow on increment"
            );
        }
        ++t;
        return derived();
    }
    Derived & operator--(int){ // post decrement
        Stored t = m_t;
        if(! derived().validate(*this - 1)){
            E::overflow_error(
                "Overflow on increment"
            );
        }
        --t;
        return derived();
    }
    Derived operator-() const { // unary minus
        static_assert(
            std::numeric_limits<Stored>::is_signed,
            "Application of unary minus to unsigned value is an error"
        );
        *this = 0 - *this; // this will check for overflow
        return derived();
    }
    Derived operator~() const {
        static_assert(
            std::numeric_limits<Stored>::is_signed,
            "Bitwise inversion of unsigned value is an error"
        );
        if(! derived().validate(~m_t)){
            E::overflow_error(
                "Overflow on increment"
            );
        }
        return derived();
    }

    /////////////////////////////////////////////////////////////////
    // binary comparison operators
    template<class U>
    bool operator<(const U & rhs) const {
        return boost::numeric::safe_compare::less_than(m_t, rhs);
    }
    template<class U>
    bool operator>(const U & rhs) const {
        return boost::numeric::safe_compare::greater_than(m_t, rhs);
    }
    template<class U>
    bool operator==(const U & rhs) const {
        return boost::numeric::safe_compare::equal(m_t, rhs);
    }
    template<class U>
    bool inline operator!=(const U & rhs) const {
        return ! boost::numeric::safe_compare::equal(m_t,rhs);
    }
    template<class U>
    bool inline operator>=(const U & rhs) const {
        return ! boost::numeric::safe_compare::less_than(m_t, rhs);
    }
    template<class U>
    bool inline operator<=(const U & rhs) const {
        return ! boost::numeric::safe_compare::greater_than(m_t, rhs);
    }

/*
    /////////////////////////////////////////////////////////////////
    // subtraction
    template<class T, class U>
    struct no_subtraction_overflow_possible : public
        boost::mpl::and_<
            typename boost::mpl::greater<
                typename boost::mpl::sizeof_< decltype(Stored() - U()) >,
                typename boost::mpl::max<
                    boost::mpl::sizeof_<U>,
                    boost::mpl::sizeof_<Stored>
                >::type
            >,
            boost::numeric::is_signed<decltype(Stored() - U())>
        >
    {};

    template<class T, class U>
    struct no_subtraction_overflow_possible;

    // case 1 - no overflow possible

    template<class U>
    typename boost::enable_if<
        no_subtraction_overflow_possible<Stored, U>,
        decltype(Stored() - U())
    >::type
    inline operator-(const U & rhs) const {
        return m_t - rhs;
    }

    template<class U>
    typename boost::disable_if<
        no_subtraction_overflow_possible<Stored, U>,
        decltype(Stored() - U())
    >::type
    inline operator-(const U & rhs) const {
        return detail::check_subtraction_overflow<
            boost::numeric::is_signed<Stored>::value,
            boost::numeric::is_signed<U>::value
        >::subtract(m_t, boost::numeric::safe_cast<decltype(Stored() - U())>(rhs));
    }

    /////////////////////////////////////////////////////////////////
    // multiplication

    template<class U>
    decltype(U() * Stored())
    inline operator*(const U & rhs) const {
        return detail::check_multiplication_overflow(m_t, rhs);
    }

    /////////////////////////////////////////////////////////////////
    // division
    template<class U>
    decltype(U() / Stored())
    inline operator/(const U & rhs) const {
        return detail::check_division_overflow(m_t, rhs);
    }

    /////////////////////////////////////////////////////////////////
    // modulus
    template<class U>
    decltype(Stored() % U())
    inline operator%(const U & rhs) const {
        if(0 == rhs)
            throw std::domain_error("Divide by zero");
        return detail::check_modulus_overflow(m_t, rhs);
    }

    /////////////////////////////////////////////////////////////////
    // logical operators
    template<class U>
    decltype(Stored() | U())
    inline operator|(const U & rhs) const {
        // verify that U is an integer type
        static_assert(
            std::numeric_limits<U>::is_integer,
            "right hand side is not an integer type"
        );
        return m_t | rhs;
    }
    template<class U>
    decltype(Stored() & U())
    inline operator&(const U & rhs) const {
        // verify that U is an integer type
        static_assert(
            std::numeric_limits<U>::is_integer,
            "right hand side is not an integer type"
        );
        return m_t & rhs;
    }
    template<class U>
    decltype(Stored() ^ U())
    inline operator^(const U & rhs) const {
        // verify that U is an integer type
        static_assert(
            std::numeric_limits<U>::is_integer,
            "right hand side is not an integer type"
        );
        return m_t ^ rhs;
    }
    template<class U>
    Stored inline operator>>(const U & rhs) const {
        // verify that U is an integer type
        static_assert(
            std::numeric_limits<U>::is_integer,
            "right hand side is not an integer type"
        );
        if(m_t < 0)
            boost::numeric::overflow("right shift of negative number undefined");
        typedef decltype(Stored() >> U()) result_type;
        if(rhs > boost::numeric::bits<Stored>::value)
            boost::numeric::overflow("conversion of negative value to unsigned");

        return m_t >> rhs;
    }
    template<class U>
    Stored inline operator<<(const U & rhs) const {
        // verify that U is an integer type
        static_assert(
            std::numeric_limits<U>::is_integer,
            "right hand side is not an integer type"
        );
        if(m_t < 0)
            boost::numeric::overflow("right shift of negative number undefined");
        typedef decltype(Stored() >> U()) result_type;
        if(rhs > boost::numeric::bits<Stored>::value)
            boost::numeric::overflow("conversion of negative value to unsigned");
        return m_t << rhs;
    }

*/
    /////////////////////////////////////////////////////////////////
    // casting operators for intrinsic integers
    explicit SAFE_NUMERIC_CONSTEXPR operator const Stored & () const {
        return m_t;
    }
};

} // numeric
} // boost

namespace boost {
namespace numeric {

// default implementations for required meta-functions
template<typename T>
struct is_safe : public std::false_type
{};

template<typename T>
struct base_type {
    typedef T type;
};

template<class T>
SAFE_NUMERIC_CONSTEXPR const typename base_type<T>::type & base_value(const T & t) {
    return static_cast<const typename base_type<T>::type & >(t);
}

template<typename T>
struct get_promotion_policy {
    typedef void type;
};

template<typename T>
struct get_exception_policy {
    typedef void type;
};
} // numeric
} // boost

#endif // BOOST_NUMERIC_SAFE_BASE_HPP