#pragma once

#include <sstream>
#include <cassert>

namespace rhi
{
	inline void combineStrSS(std::stringstream& ss)
	{
	}

	template <typename ArgType>
	void combineStrSS(std::stringstream& ss, const ArgType& Arg)
	{
		ss << Arg;
	}

	template <typename FirstArgType, typename... RemainArgsType>
	void combineStrSS(std::stringstream& ss, const FirstArgType& firstArg, const RemainArgsType&... remainArgs)
	{
		combineStrSS(ss, firstArg);
		combineStrSS(ss, remainArgs...); // recursive call using pack expansion syntax
	}

	template <typename... ArgsType>
	std::string combineString(const ArgsType&... remainArgs)
	{
		std::stringstream ss;
		combineStrSS(ss, remainArgs...);
		return ss.str();
	}

	// A type cast that is safer than static_cast in debug builds, and is a simple static_cast in release builds.
	// Used for downcasting various ISomething* pointers to their implementation classes in the backends.
	template <typename T, typename U>
	T checked_cast(U u)
	{
		static_assert(!std::is_same<T, U>::value, "Redundant checked_cast");
#ifdef _DEBUG
		if (!u) return nullptr;
		T t = dynamic_cast<T>(u);
		if (!t) assert(!"Invalid type cast");  // NOLINT(clang-diagnostic-string-conversion)
		return t;
#else
		return static_cast<T>(u);
#endif
	}


#define ENUM_CLASS_FLAG_OPERATORS(T) \
    inline constexpr T operator | (T a, T b) { return T(uint32_t(a) | uint32_t(b)); } \
	inline constexpr T operator |= (T a, T b) {return T(a = a | b);}\
    inline constexpr T operator & (T a, T b) { return T(uint32_t(a) & uint32_t(b)); } /* NOLINT(bugprone-macro-parentheses) */ \
	inline constexpr T operator &= (T a, T b) {return T(a = a & b);}\
    inline constexpr T operator ~ (T a) { return T(~uint32_t(a)); } /* NOLINT(bugprone-macro-parentheses) */ \
    inline constexpr bool operator !(T a) { return uint32_t(a) == 0; } \
    inline constexpr bool operator ==(T a, uint32_t b) { return uint32_t(a) == b; } \
    inline constexpr bool operator !=(T a, uint32_t b) { return uint32_t(a) != b; }	\
	

	template <typename T>
	bool isPowerOfTwo(T val)
	{
		return val > 0 && (val & (val - 1)) == 0;
	}

	template <typename T1, typename T2>
	inline typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type AlignUp(T1 val, T2 alignment)
	{
		static_assert(std::is_unsigned<T1>::value == std::is_unsigned<T2>::value, "both types must be either signed or unsigned");
		static_assert(!std::is_pointer<T1>::value && !std::is_pointer<T2>::value, "types must not be pointers");
		assert(isPowerOfTwo(alignment));

		using ResultType = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type;
		return (static_cast<ResultType>(val) + static_cast<ResultType>(alignment - 1)) & ~static_cast<ResultType>(alignment - 1);
	}


	template<typename T> [[nodiscard]] bool arraysAreDifferent(const T* a, uint32_t a_len, const T* b, uint32_t b_len)
	{
		if (a_len != b_len)
		{
			return true;
		}

		for (uint32_t i = 0; i < a_len; i++)
		{
			if (a[i] != b[i])
				return true;
		}

		return false;
	}
}