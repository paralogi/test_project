#pragma once
#include <type_traits>
#include <tuple>

/**
 * 
 */
template< typename Type, typename Next >
struct Entry {
   using value = Type;
   using next = Next;
};

struct Empty {};

/**
 * 
 */
template < typename List >
struct Size {
   static constexpr size_t value = 1 + Size< typename List::next >::value;
};

template <>
struct Size< Empty > : std::integral_constant< size_t, 0 > {};

/**
 * 
 */
template< typename List, size_t I >
struct Item {
   using value = typename Item< typename List::next, I - 1 >::value;
};

template< typename List >
struct Item< List, 0 > {
   using value = List;
};

template< size_t I >
struct Item< Empty, I > {
   using value = Empty;
};

/**
 * 
 */
template< typename List, typename T >
struct Push {
   using value = Entry< 
         typename List::value, 
         typename Push< typename List::next, T >::value >;
};

template< typename T >
struct Push< Empty, T > {
   using value = Entry< T, Empty >;
};

/**
 * 
 */
template < typename Tuple, typename T >
struct TuplePush;

template< typename Type, typename... Args >
struct TuplePush< std::tuple< Args... >, Type > { 
   using type = std::tuple< Type, Args... >; 
};

template< typename List >
struct Path {
   using value = typename TuplePush< 
         typename Path< typename List::next >::value, 
         typename List::value >::type;
};

template<>
struct Path< Empty > {
   using value = std::tuple<>;
};

/**
 * 
 */
template < typename Tuple >
struct TupleGet;

template< typename Type, typename... Args >
struct TupleGet< std::tuple< Type, Args... > > { 
   using type = Type; 
   using next = std::tuple< Args... >;
};

template < typename Tuple >
struct Walk {
   using value = Entry< 
         typename TupleGet< Tuple >::type, 
         typename Walk< typename TupleGet< Tuple >::next >::value >;
};

template <>
struct Walk< std::tuple<> > {
   using value = Empty;
};

/**
 * 
 */
void testTypeList() {
   
   using l1 = typename Push< Empty, bool >::value;
   using l2 = typename Push< l1, char >::value;
   using l3 = typename Push< l2, int >::value;
   using ltest = Entry< bool, Entry< char, Entry< int, Empty > > >;
   static_assert( std::is_same< l3, ltest >::value, "" );
   static_assert( Size< l3 >::value == Size< ltest >::value, "" );
   
   using i3 = typename Item< l3, 1 >::value;
   using itest = typename Item< ltest, 1 >::value;
   static_assert( std::is_same< i3, itest >::value, "" );
   
   using t3 = typename Path< l3 >::value;
   using ttest = std::tuple< bool, char, int >;
   static_assert( std::is_same< t3, ttest >::value, "" );
   
   using w3 = typename Walk< t3 >::value;
   using wtest = Entry< bool, Entry< char, Entry< int, Empty > > >;
   static_assert( std::is_same< w3, wtest >::value, "" );
}
