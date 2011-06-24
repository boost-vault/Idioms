/*
Version 2 highlights:
	*  BOOST_PREDICATED_CONSTRUCTOR creates a reference to the object named 'name'. Thus, now you can write name.a, instead of name->a.
	* Fixed alignment of former.
	* Fixed MSVC preprocessor issues
*/


/*

Motivating example:

Suppose you have a sentry class that enables wireframe rendering mode
in some Direct3D context and upon destruction reverts solid fill.

struct WireframeSentry 
{
	WireframeSentry(IDirect3DDevice9* device)
		: m_Device(device)
	{
		m_Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}

	~WireframeSentry()
	{
		m_Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}

private:
	IDirect3DDevice9* m_Device;
};

The question is - how can you enable wireframe rendering through
the sentry if it's optional, as in you want to enable wireframe
rendering based on some external predicate. Obviously you can't
optionally skip the constructor of a locally instantiated object.

	if (renderInWireframe)
		WireframeSentry(device);

We can do so if, for example we provide a second bool parameter
in the constructor:

	WireframeSentry(IDirect3DDevice9* device, bool reallyEnable=true)

Clearly this solution doesn't scale, and it doesn't attack the
problem at hand - how to conditionally create objects *and*
reap the fruit of the destructor-on-scope-exit that the compiler
guarantees for local objects.

Solution:

Enter predicated construction. Our example will be superceded by:

bool renderInWireframe = ...;
BOOST_PREDICATED_ANONYMOUS_CONSTRUCTOR(renderInWireframe, WireframeSentry, (device));

If renderInWireframe is true then an unnamed object will be created and destroyed
on scope exit. Otherwise nothing will happer (save for an 'if').


*/

//(C) Copyright Stefan Dragnev 2007.
//Use, modification and distribution are subject to the
//Boost Software License, Version 1.0. (See accompanying file
//LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/type_traits/alignment_of.hpp>
#include <boost/aligned_storage.hpp>

namespace boost {
namespace detail {

template <class T>
struct predicated_constructee_storage
{
	predicated_constructee_storage(T* t)
		: _t(t)
	{}

	~predicated_constructee_storage()
	{
		if (_t)
			_t->~T();
	}

	T* operator -> () const
	{
		return _t;
	}

	T& operator * () const
	{
		return *_t;
	}

private:
	T* _t;
};

}
}

#include <boost/preprocessor/cat.hpp>

#define BOOST_PREDICATED_ANONYMOUS_CONSTRUCTOR(condition, obj, params) \
	::boost::aligned_storage< \
		sizeof(obj), ::boost::alignment_of<obj>::value \
	>::type BOOST_PP_CAT(_mem_##obj,__LINE__); \
	::boost::detail::predicated_constructee_storage<obj> BOOST_PP_CAT(_storage_##obj,__LINE__)((condition) ? new (&BOOST_PP_CAT(_mem_##obj,__LINE__)) obj params : 0)

#define BOOST_PREDICATED_CONSTRUCTOR(condition, name, obj, params) \
	::boost::aligned_storage< \
		sizeof(obj), ::boost::alignment_of<obj>::value \
	>::type BOOST_PP_CAT(_mem_##obj,__LINE__); \
	::boost::detail::predicated_constructee_storage<obj> BOOST_PP_CAT(_storage_##obj,__LINE__)((condition) ? new (&BOOST_PP_CAT(_mem_##obj,__LINE__)) obj params : 0); \
	obj& name = *BOOST_PP_CAT(_storage_##obj,__LINE__)

#define BOOST_ANONYMOUS_CONSTRUCTOR(obj, params) \
	obj BOOST_PP_CAT(anonymous##obj,__LINE__) params;
