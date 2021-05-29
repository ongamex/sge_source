#pragma once

#if defined(__GNUC__)
#include <signal.h>
#endif

#include <cstddef> // for size_t

namespace sge {

//------------------------------------------------------------------------
// Warnings disable
//------------------------------------------------------------------------
#ifdef WIN32
#define SGE_NO_WARN_BEGIN __pragma(warning(push, 0))
#define SGE_NO_WARN_END __pragma(warning(pop))
#else
#define SGE_NO_WARN_BEGIN
#define SGE_NO_WARN_END
#endif

//------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------
typedef unsigned char ubyte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char sbyte;
typedef signed short sint16;
typedef signed int sint32;
typedef signed long long sint64;
typedef signed long long int64;
typedef signed long long i64;

template <typename T, size_t N>
char (&SGE_TArrSize_Safe(T (&)[N]))[N];
#define SGE_ARRSZ(A) (sizeof(SGE_TArrSize_Safe(A)))

} // namespace sge

////#ifndef SGE_FORCE_INLINE
//	#ifdef _MSC_VER
//			#define	SGE_FORCE_INLINE        __forceinline
//	#elif __GNUC__
//			#define SGE_FORCE_INLINE        __always_inline
//	#else
//		#warning "SGE_FORCE_INLINE is not implemented correctly. Please implement it for this platoform."
//		SGE_FORCE_INLINE                  inline
//	#endif
////#endif

#if defined(SGE_USE_DEBUG)
#if defined(__EMSCRIPTEN__)
#define SGE_BREAK() void(0) // does nothing, because emscripten disables asm.js vailation if we use __builtin_debugtrap()
#elif defined(_WIN32)
#define SGE_BREAK() __debugbreak()
#elif defined(__clang__)
#define SGE_BREAK() __builtin_debugtrap()
#else
#define SGE_BREAK() raise(SIGTRAP)
#endif
#else
#define SGE_BREAK() void(0)
#endif

int assertAskDisable(const char* const file, const int line, const char* expr);

#if defined(SGE_USE_DEBUG)
#define sgeAssert(__exp)                                                     \
	do {                                                                     \
		static int isBrakPointDisabled = 0;                                  \
		if (!isBrakPointDisabled && (false == bool(__exp))) {                \
			int responseCode = assertAskDisable(__FILE__, __LINE__, #__exp); \
			isBrakPointDisabled = !isBrakPointDisabled && responseCode != 0; \
			if (responseCode != 2) {                                         \
				SGE_BREAK();                                                 \
			}                                                                \
		}                                                                    \
	} while (false)
#ifdef __clang__
#define if_checked(expr) if ((expr))
#else
#define if_checked(expr) if ((expr) ? true : (SGE_BREAK(), false))
#endif
#else
#define sgeAssert(__exp) \
	{}
#define if_checked(expr) if (expr)
#endif

#define sgeAssertFalse(msg) sgeAssert(false && msg)

#define SGE_MACRO_STR_IMPL(m) #m
#define SGE_MACRO_STR(m) SGE_MACRO_STR_IMPL(m)

//#if defined _MSC_VER
//	#define SGE_EXTERN_CONST          __declspec(selectany) extern const
//#else
//	#define SGE_EXTERN_CONST          __attribute__((selectany)) extern const
//#endif

#define UNUSED(x)

#define SGE_CAT_IMPL(s1, s2) s1##s2
#define SGE_CAT(s1, s2) SGE_CAT_IMPL(s1, s2)
#define SGE_ANONYMOUS(x) SGE_CAT(x, __COUNTER__)