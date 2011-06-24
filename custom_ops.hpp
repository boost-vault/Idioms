#pragma once

//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*

Copyright 2008 Stefan Dragnev, tailsu at abv stop bg
Suggestions are welcome.

	(Almost) Custom operators
=================================

Introduction:

	Long story cut short, this library allows for syntax sugar like this:

		class A; class B;
		...
		int val = A /~+!- B;

	where the /~+!- part is our custom operator. These custom operators consist
	of any overloadable binary operator plus an arbitrary string of unary operators.

	The above operator could be defined like this:

		BOOST_CUSTOM_OP(int, const A&, a, /, ~+!, -, const B&, b)
		{
			return a.value() * 3 + b.as_int() * 2;
		}

	This defines an operator that returns an int and takes two const references named 'a' and 'b'.
	The operator string ( /~+!- in our example) is defines as the binary op /, the intermediate
	string ~+! and the last operator -. This splitting is required due to implementation issues.
	When used, the library defines a unary operator that takes the second type as an argument.
	This implies all kinds of restrictions on the second type; for example it
	can't be a fundamental type (int, double, some_class*, etc.)(see notes)
	and also can't have a unary operator like the rightmost operator in the operator string.

Synopsis:

	BOOST_CUSTOM_OP(rettype, param1type, param1name, binop, ops, lastop, param2type, param2name)
	{
		// user implementation
	}

	rettype - the return type of the operator
	param1type - the type of the left-hand parameter
	param1name - the name of the left-hand parameter
	binop - the first character of the operator string, most C++ binary operators
	ops - the intermediate operators, any string of valid C++ prefix unary operators
	lastop - the last operator in the string, any valid C++ prefix unary operator
	param2type - the type of the right-hand parameter
	param2name - the name of the right-hand parameter

Notes:

	* The library depends on Boost.Typeof

	* if you want the second type parameter to be a fundamental type, you must
	wrap it with boost::ref() or boost::cref() at the call site. The internally
	used reference wrapper type isn't boost::reference_wrapper<T>, so, the above
	limitations on unary operators aren't exposed.

	* prefix ++ and -- are supported as well. Mind that when using them you'll have to
	treat them like single operators, i.e. to define the operator |++*--, you'll write:
		BOOST_CUSTOM_OP(..., |, ++*, --, ...)

	* operator comma (,) can be used like this:
		#define COMMA ,
		BOOST_CUSTOM_OP(..., COMMA, +*, -, ...)

		The library defines the more verbose BOOST_CUSTOM_OP_COMMA for this purpose

	* supported prefix unary operators are + - & * ++ -- ! ~

A full example:

	#include "custom_ops.hpp"

	struct A
	{
		int value() const { return a; }
		int a;
	};

	struct B
	{
		int as_int() const { return b; }
		int b;
	};

	BOOST_CUSTOM_OP(int, const A&, a, /, ~+, -, const B&, b)
	{
		return a.value() * 2 + b.as_int() * 3;
	}

	int main()
	{
		A a; B b;
		a.a = 5; b.b = 7;

		int val = a /~+- b;
		cout << val << endl;
	}
*/

#include <boost/typeof/typeof.hpp>
#include <boost/preprocessor/cat.hpp>

#include <boost/ref.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/add_const.hpp>

namespace boost {
namespace custom_ops {

struct plus_tag {};
struct minus_tag {};
struct amp_tag {};
struct asterisk_tag {};
struct increment_tag {};
struct decrement_tag {};
struct excl_tag {};
struct tilde_tag {};

struct
{
	plus_tag operator + () const { return plus_tag(); }
	minus_tag operator - () const { return minus_tag(); }
	amp_tag operator & () const { return amp_tag(); }
	asterisk_tag operator * () const { return asterisk_tag(); }
	increment_tag operator ++ () const { return increment_tag(); }
	decrement_tag operator -- () const { return decrement_tag(); }
	excl_tag operator ! () const { return excl_tag(); }
	tilde_tag operator ~ () const { return tilde_tag(); }
} tag_from_op;

#define BOOST_COPS_ITERATE_OPS(F) \
	F(+) F(-) F(&) F(*) F(++) F(--) F(!) F(~)

#define BOOST_COPS_OPTAG(OP)\
	BOOST_TYPEOF(OP tag_from_op)

template <class T, class Tag>
struct wrapped;

template <class W>
struct unwrap
{
	typedef W type;
};

template <class W, class Tag>
struct unwrap<wrapped<W, Tag>>
{
	typedef typename unwrap<W>::type type;
};

template <class T, class Tag>
struct wrapped
{
	typedef typename unwrap<T>::type type;
	explicit wrapped(type t)
		: value(t)
	{}
	template <class U, class Tag>
	explicit wrapped(wrapped<U, Tag> u)
		: value(u.value)
	{}

	type value;
};

static struct one_t {} one;

template <class T>
struct type_finder_impl
{
	typedef T type;

	type_finder_impl(one_t) {}

#define BOOST_COPS_MAKE_TYPE_FINDER(OP) \
	type_finder_impl<wrapped<T, BOOST_COPS_OPTAG(OP)>> operator OP () const { return one; }
	BOOST_COPS_ITERATE_OPS(BOOST_COPS_MAKE_TYPE_FINDER)
#undef BOOST_COPS_MAKE_TYPE_FINDER
};

template <class T>
struct type_finder
{
	static type_finder_impl<T> f;
};

template <class T> type_finder_impl<T> type_finder<T>::f;

#define BOOST_COPS_MAKE_WRAPPING_OPERATORS(OP) \
	template <class T, class Tag> \
	wrapped<wrapped<T, Tag>, BOOST_COPS_OPTAG(OP)> operator OP (wrapped<T, Tag> w) \
	{ \
		return wrapped<wrapped<T, Tag>, BOOST_COPS_OPTAG(OP)>(w); \
	}

BOOST_COPS_ITERATE_OPS(BOOST_COPS_MAKE_WRAPPING_OPERATORS)

#undef BOOST_COPS_MAKE_WRAPPING_OPERATORS

#undef BOOST_COPS_ITERATE_OPS
#undef BOOST_COPS_OPTAG

template <class T>
struct cop_reference_wrapper
	: reference_wrapper<T>
{
	cop_reference_wrapper(reference_wrapper<T> w)
		: reference_wrapper<T>(w)
	{}
};

template <class T_>
struct reasonable_type_for_unary_operator_overload
{
	typedef typename mpl::eval_if<
		is_reference<T_>,
		remove_reference<T_>,
		add_const<T_>
	>::type T;

	typedef typename mpl::if_<
		mpl::or_<is_fundamental<T>, is_pointer<T>>,
		cop_reference_wrapper<T>,
		T_
	>::type type;
};

}
}

#define BOOST_CUSTOM_OP(rettype, param1type, param1name, binop, ops, firstop, param2type, param2name) \
	boost::custom_ops::wrapped<param2type, BOOST_TYPEOF(firstop boost::custom_ops::tag_from_op)> operator firstop (boost::custom_ops::reasonable_type_for_unary_operator_overload<param2type>::type w) \
	{ \
		return boost::custom_ops::wrapped<param2type, BOOST_TYPEOF(firstop boost::custom_ops::tag_from_op)>(w); \
	} \
	static rettype BOOST_PP_CAT(boost_custom_ops_implementation_, __LINE__)(param1type, param2type); \
	rettype operator binop (param1type a, BOOST_TYPEOF(ops firstop boost::custom_ops::type_finder<param2type>::f)::type b) \
	{ \
		return BOOST_PP_CAT(boost_custom_ops_implementation_, __LINE__)(a, b.value); \
	} \
	static rettype BOOST_PP_CAT(boost_custom_ops_implementation_, __LINE__)(param1type param1name, param2type param2name)

#define BOOST_CUSTOM_OP_COMMA ,
