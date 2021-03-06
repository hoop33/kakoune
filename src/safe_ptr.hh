#ifndef safe_ptr_hh_INCLUDED
#define safe_ptr_hh_INCLUDED

// #define SAFE_PTR_TRACK_CALLSTACKS

#include "assert.hh"
#include "ref_ptr.hh"

#include <type_traits>
#include <utility>

#ifdef SAFE_PTR_TRACK_CALLSTACKS
#include "backtrace.hh"
#include "vector.hh"
#include <algorithm>
#endif

namespace Kakoune
{

// *** SafePtr: objects that assert nobody references them when they die ***

class SafeCountable
{
public:
#ifdef KAK_DEBUG
    SafeCountable() : m_count(0) {}
    ~SafeCountable()
    {
        kak_assert(m_count == 0);
        #ifdef SAFE_PTR_TRACK_CALLSTACKS
        kak_assert(m_callstacks.empty());
        #endif
    }

private:
    friend struct SafeCountablePolicy;
    #ifdef SAFE_PTR_TRACK_CALLSTACKS
    struct Callstack
    {
        Callstack(void* p) : ptr(p) {}
        void* ptr;
        Backtrace bt;
    };

    mutable Vector<Callstack> m_callstacks;
    #endif
    mutable int m_count;
#endif
};

struct SafeCountablePolicy
{
#ifdef KAK_DEBUG
    static void inc_ref(const SafeCountable* sc, void* ptr)
    {
        ++sc->m_count;
        #ifdef SAFE_PTR_TRACK_CALLSTACKS
        sc->m_callstacks.emplace_back(ptr);
        #endif
    }

    static void dec_ref(const SafeCountable* sc, void* ptr)
    {
        --sc->m_count;
        kak_assert(sc->m_count >= 0);
        #ifdef SAFE_PTR_TRACK_CALLSTACKS
        auto it = std::find_if(sc->m_callstacks.begin(), sc->m_callstacks.end(),
                               [=](const Callstack& cs) { return cs.ptr == ptr; });
        kak_assert(it != sc->m_callstacks.end());
        sc->m_callstacks.erase(it);
        #endif
    }

    static void ptr_moved(const SafeCountable* sc, void* from, void* to)
    {
        #ifdef SAFE_PTR_TRACK_CALLSTACKS
        auto it = std::find_if(sc->m_callstacks.begin(), sc->m_callstacks.end(),
                               [=](const Callstack& cs) { return cs.ptr == from; });
        kak_assert(it != sc->m_callstacks.end());
        it->ptr = to;
        #endif
    }
#else
    static void inc_ref(const SafeCountable*, void* ptr) {}
    static void dec_ref(const SafeCountable*, void* ptr) {}
    static void ptr_moved(const SafeCountable*, void*, void*) {}
#endif
};

template<typename T>
using SafePtr = RefPtr<T, SafeCountablePolicy>;

}

#endif // safe_ptr_hh_INCLUDED
