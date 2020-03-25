//-----------------------------------------------------------------------------
// Copyright (C) Doppelgamer LLC, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#ifndef PCH_H
#define PCH_H
#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdarg>
#include <filesystem>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max

constexpr size_t cpu_bits = sizeof(size_t) * 8;
constexpr bool is_32_bit = cpu_bits == 32;
constexpr bool is_64_bit = cpu_bits == 64;

// Workaround for issue where attributes and their [[ ]] break visual studio's intellisense
#ifdef __INTELLISENSE__
   #define NODISCARD
   #define UNUSED
   #define nodisc
#else
   #define NODISCARD [[nodiscard]]
   #define nodisc [[nodiscard]]
   #define UNUSED [[maybe_unused]]
#endif

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using smax = intmax_t;
using imax = intmax_t;
using umax = uintmax_t;
using intmax = intmax_t;
using uintmax = uintmax_t;

using longlong  = long long;
using llong     = long long;
using uchar     = unsigned char;
using ushort    = unsigned short;
using uint      = unsigned int;
using ulong     = unsigned long;
using ulonglong = unsigned long long;
using ullong    = unsigned long long;
using ldouble   = long double;

using wchar = wchar_t;

constexpr double unit_nano  = 1000000000;
constexpr double unit_micro = 1000000;
constexpr double unit_milli = 1000;

NODISCARD constexpr s8  operator""_s8 (ulonglong x) { return (s8)x; }
NODISCARD constexpr s16 operator""_s16(ulonglong x) { return (s16)x; }
NODISCARD constexpr s32 operator""_s32(ulonglong x) { return (s32)x; }
NODISCARD constexpr s64 operator""_s64(ulonglong x) { return (s64)x; }

NODISCARD constexpr u8  operator""_u8 (ulonglong x) { return (u8)x; }
NODISCARD constexpr u16 operator""_u16(ulonglong x) { return (u16)x; }
NODISCARD constexpr u32 operator""_u32(ulonglong x) { return (u32)x; }
NODISCARD constexpr u64 operator""_u64(ulonglong x) { return (u64)x; }

NODISCARD constexpr size_t operator""_zu(ulonglong x) { return (size_t)x; }
NODISCARD constexpr size_t operator""_sz(ulonglong x) { return (size_t)x; }

// Memory ---------------------------------------------------------------------
constexpr auto KB = 1 << 10;    // 2 ^ 10, 1024
constexpr auto MB = 1 << 20;    // 2 ^ 20, KB * 1024
constexpr auto GB = 1 << 30;    // 2 ^ 30, MB * 1024
constexpr auto TB = 1LL << 40u; // 2 ^ 40, GB * 1024

NODISCARD constexpr auto operator""_KB(ulonglong x) { return x*KB; }
NODISCARD constexpr auto operator""_MB(ulonglong x) { return x*MB; }
NODISCARD constexpr auto operator""_GB(ulonglong x) { return x*GB; }
NODISCARD constexpr auto operator""_TB(ulonglong x) { return x*TB; }

// Template Convenience -------------------------------------------------------
#define TCT    template <class T>
#define TCTS   template <class... Ts>
#define TCT2   template <class T1, class T2>
#define TCT3   template <class T1, class T2, class T3>
#define TCU    template <class U>
#define TCTU   template <class T, class U>
#define TCTTS  template <class T, class... Ts>
#define TCTN   template <class T, auto N>
#define TCN    template <auto N>

// Looping --------------------------------------------------------------------
#define loop_(i,m) for(int i=0; i<(int)(m); i++)
#define loopi(m) loop_(i,m)
#define loopj(m) loop_(j,m)
#define loopk(m) loop_(k,m)
#define loopx(m) loop_(x,m)
#define loopy(m) loop_(y,m)
#define loopz(m) loop_(z,m)
#define loopij(m,n) loopi(m) loopj(n)
#define loopxy(m,n) loopy(n) loopx(m)
#define loopxyz(m,n,o) loopz(o) loopy(n) loopx(m)

// Reverse looping ------------------------------------------------------------
#define rloop_(i,m) for(int i=(int)(m-1); i>=0; i--)
#define rloopi(m) rloop_(i,m)
#define rloopj(m) rloop_(j,m)
#define rloopk(m) rloop_(k,m)
#define rloopx(m) rloop_(x,m)
#define rloopy(m) rloop_(y,m)
#define rloopz(m) rloop_(z,m)

//-----------------------------------------------------------------------------
#define INIT_ON_ACCESS_PTR(t, n, ...)  t& n { static t* v=new t(__VA_ARGS__); return *v; }
#define INIT_ON_ACCESS(t, n, ...)      t& n { static t v {__VA_ARGS__}; return v; }
#define STATIC_VAR(t, n, ...)          static INIT_ON_ACCESS(t, n, __VA_ARGS__)
#define GLOBAL_VAR(t, n, ...)          inline INIT_ON_ACCESS(t, n, __VA_ARGS__)
#define FORMAT                         _Printf_format_string_

TCT constexpr auto type_name() { return typeid(T).name(); }

using namespace std;
using namespace std::literals;
using namespace std::literals::string_view_literals;

using cstr  = const char*;
using wcstr = const wchar_t*;
using ccstr = const char* const;
using sview = string_view;
using kview = const string_view&;

cstr sformat(FORMAT cstr fmt, ...);
cstr svformat(FORMAT cstr fmt, va_list args);

struct gerror : public exception
{
   gerror(): exception() {}
   gerror(cstr s): exception(s) {}
   gerror(const string& s): exception(s.c_str()) {}

   template <class... Args>
   gerror(cstr fmt, Args&&... args): exception(sformat(fmt, forward<Args>(args)...)) {}
};

#define DEFINE_EXCEPTION(tname) struct tname : gerror { using gerror::gerror; }

void log(FORMAT cstr fmt, ...);
void warn(FORMAT cstr fmt, ...);
void error(FORMAT cstr fmt, ...);

#define SYS_PRINT(t, fmt)\
   va_list args;\
   va_start(args, fmt);\
   printf("%s: %s", t, svformat(fmt, args));\
   va_end(args)

inline void log(FORMAT cstr fmt, ...) { SYS_PRINT("", fmt); }
inline void warn(FORMAT cstr fmt, ...) { SYS_PRINT("warning", fmt); }
inline void error(FORMAT cstr fmt, ...) { SYS_PRINT("error", fmt); }

void OnFail(cstr func, cstr file, int line, cstr title, cstr fmt, ...);

#ifndef NDEBUG
   #define DEBUG_BREAK __debugbreak() //DebugBreak()
#else
   #define DEBUG_BREAK exit(12345)
#endif

#define ON_FAIL(title, ...) (void)((OnFail(__FUNCSIG__, __FILE__, __LINE__, title, __VA_ARGS__), 0) || (DEBUG_BREAK, 0))
#define assert_fail(...) ON_FAIL("FAILURE", __VA_ARGS__)

#define rassert(exp) (void)((!!(exp)) || (ON_FAIL("ASSERTION FAILED", #exp), 0))

//#define assert(exp) (void)((!!(exp)) || (_wassert(#exp, __FILE__, __LINE__), 0))

#ifdef NDEBUG
   #define assert(exp) ((void)0)
#else
   #define assert rassert
#endif

#define check_index_action(act,x,...) (void)((check_index(x,__VA_ARGS__)) || (act(sformat("Bad index: " #x "= %s", x)),0))

//#define assert_action(exp, a) if (!(exp)) a("assertion failed: " #exp)
#define assert_action(exp, a) (void)((!(exp)) || (a("ASSERTION FAILED: " #exp), 0))
#define assert_msg(exp, ...)  (void)((!(exp)) || (assert_fail(__VA_ARGS__)))
#define assert_warn(exp)      assert_action(exp, warn)
#define assert_throw(exp)     if (!(exp)) throw gerror("ASSERTION FAILED: " #exp)
#define assert_ptr(ptr)       assert(ptr)
#define assert_range(i, ...)  assert(check_index(i, __VA_ARGS__))
#define assert_index(i, sz)   assert(check_index(i, sz))

//-----------------------------------------------------------------------------
inline void MakeLower(string& v) { loopi(v.size()) v[i] = tolower(v[i]); }//{ transform(v.begin(), v.end(), v.begin(), tolower); }
inline string ToLower(kview s) { string ret{s}; MakeLower(ret); return ret; }

//-----------------------------------------------------------------------------
cstr svformat(FORMAT cstr fmt, va_list args);
cstr sformat(FORMAT cstr fmt, ...);
cstr sformat_print(kview sv);
cstr sformat_print(char c, size_t count);
void sformat_put(kview sv);
void sformat_put(char c);
cstr sformat_ptr();

//-----------------------------------------------------------------------------
inline cstr str(char c)       { return sformat("%c", c); }
inline cstr str(short i)      { return sformat("%hd", i); }
inline cstr str(int i)        { return sformat("%d", i); }
inline cstr str(long i)       { return sformat("%ld", i); }
inline cstr str(longlong i)   { return sformat("%lld", i); }
inline cstr str(uchar c)      { return sformat("%hhu", c); }
inline cstr str(ushort i)     { return sformat("%hu", i); }
inline cstr str(uint i)       { return sformat("%u", i); }
inline cstr str(ulong i)      { return sformat("%lu", i); }
inline cstr str(ulonglong i)  { return sformat("%llu", i); }
inline cstr str(float f)      { return sformat("%g", f); }
inline cstr str(double f)     { return sformat("%g", f); }
inline cstr str(ldouble f)    { return sformat("%Lg", f); }
inline cstr str(bool b)       { return b ? "true" : "false"; }
inline cstr str(cstr s)       { return s; }
inline cstr str(kview s)      { return sformat_print(s); }

//-----------------------------------------------------------------------------
struct sformatter
{
   char buf[2_MB] {0};
   size_t pos = 0;
   size_t numWraps = 0;

   auto left() const { return sizeof(buf) - pos; }
   auto ptr() { return buf + pos; }
   
   // Returns true if wrap occured, false if str will fit
   bool check_wrap(size_t len)
   {
      assert(len < sizeof buf - 1);
      if (len < left()) return false;
      buf[pos] = 0;
      buf[0] = 0;
      pos = 0;
      numWraps++;
      return true;
   }

   void put(char c)
   {
      check_wrap(1);
      buf[pos++] = c;
   }

   void put(char c, size_t count)
   {
      if (count == 0) return;
      check_wrap(count);
      loopi(count) buf[pos++] = c;
   }

   void put(kview sv)
   {
      check_wrap(sv.size());
      memcpy(ptr(), sv.data(), sv.size());
      pos += sv.size();
   }

   cstr print(kview sv)
   {
      auto start = ptr();
      check_wrap(sv.size() + 1);
      memcpy(ptr(), sv.data(), sv.size());
      pos += sv.size();
      buf[pos++] = 0;
      return start;
   }

   cstr fill(char c, size_t count)
   {
      auto start = ptr();
      put(c, count);
      put(0);
      return start;
   }

   cstr vformat(FORMAT cstr fmt, va_list args)
   {
      auto start = ptr();
      auto len = vsnprintf(start, left(), fmt, args);
      if (len < 0) throw gerror("Invalid format string: "s + fmt);
      if (check_wrap(len))
      {
         start = ptr();
         vsnprintf(start, left(), fmt, args);
      }
      pos += (size_t)len + 1;
      return start;
   }
};

// Is this needed?
inline sformatter& sfmt() { static thread_local sformatter sf; return sf; }
inline cstr svformat(FORMAT cstr fmt, va_list args) { return sfmt().vformat(fmt, args); }
inline cstr sformat_print(kview sv) { return sfmt().print(sv); }
inline cstr sformat_print(char c, size_t count) { return sfmt().fill(c, count); }
inline void sformat_put(char c) { sfmt().put(c); }
inline void sformat_put(kview sv) { sfmt().put(sv); }
inline cstr sformat_ptr() { return sfmt().ptr(); }

inline cstr sformat(FORMAT cstr fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   cstr s = sfmt().vformat(fmt, args);
   va_end(args);
   return s;
}

inline void OnFail(cstr func, cstr file, int line, cstr title, cstr fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   cstr msg = sformat(
      "******************************************************************\n"
      " %s: %s\n"
      " \n"
      "  func: %s\n"
      "  file: %s\n"
      "  line: %d\n"
      "******************************************************************\n",
      title, svformat(fmt, args), func, file, line);

   va_end(args);
   printf(msg);
   OutputDebugStringA(msg);
   #ifdef NDEBUG
      MessageBoxA(0, msg, "Error", MB_OK);
   #endif
}

#endif