#pragma once

#include <iostream>

// Number of Bits our counter is using. Lower number faster compile time,
// but less distinct values. With 16 we have 2^16 distinct values.
#define MAX_DEPTH 16

// Used for counting.
template< int N >
struct flag {
   friend constexpr int adl_flag( flag< N > );
};

// Used for noting how far down in the binary tree we are.
// depth<0> equales leaf nodes. depth<MAX_DEPTH> equals root node.
template< int N > struct depth {
};

// Creating an instance of this struct marks the flag<N> as used.
template< int N >
struct mark {
   friend constexpr int adl_flag( flag< N > ) {
      return N;
   }

   static constexpr int value = N;
};

// Heart of the expression. The first two functions are for inner nodes and
// the next two for termination at leaf nodes.

// char[noexcept( adl_flag(flag<N>()) ) ? +1 : -1] is valid if flag<N> exists.
template< int D, int N, class = char[ noexcept ( adl_flag( flag< N >() ) ) ? +1 : -1 ] >
int constexpr binary_search_flag( int, depth< D >, flag< N >, int next_flag = binary_search_flag( 0, depth< D - 1 >(), flag< N + ( 1 << ( D - 1 ) ) >() ) ) {
   return next_flag;
}

template< int D, int N >
int constexpr binary_search_flag( float, depth< D >, flag< N >, int next_flag = binary_search_flag( 0, depth< D - 1 >(), flag< N - ( 1 << ( D - 1 ) ) >() ) ) {
   return next_flag;
}

template< int N, class = char[ noexcept ( adl_flag( flag< N >() ) ) ? +1 : -1 ] >
int constexpr binary_search_flag( int, depth< 0 >, flag< N > ) {
   return N + 1;
}

template< int N >
int constexpr binary_search_flag( float, depth< 0 >, flag< N > ) {
   return N;
}

// The actual expression to call for increasing the count.
template< int next_flag = binary_search_flag( 0, depth< MAX_DEPTH - 1 >(), flag< ( 1 << ( MAX_DEPTH - 1 ) ) >() ) >
int constexpr counter_id( int value = mark< next_flag >::value ) {
   return value;
}

static constexpr int a = counter_id();
static constexpr int b = counter_id();
static constexpr int c = counter_id();

void test() {
   std::cout << "Value a: " << a << std::endl;
   std::cout << "Value b: " << b << std::endl;
   std::cout << "Value c: " << c << std::endl;
}
