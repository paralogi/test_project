#pragma once

template< int N > struct Res {
   static const int res = N;
};

// Is the function defined to indicate the presence or absence of the state
template< int N > 
struct Status { 
   friend constexpr auto stateExist( Status< N > );
};

template< int N, class ResultType > 
struct State {
   friend constexpr auto stateExist( Status< N > ) { return ResultType{}; }
   static constexpr int state = N;
};

template<> 
struct State< 1, Res< 1 > > {
   friend constexpr auto stateExist( Status< 1 > ) { return Res< 1 >{}; }
   static constexpr int state = 1;
};

// Searching for results for the most recent status
template< int N, int = decltype( stateExist( Status< N >{} ) )::res > 
static constexpr int search( int, Status< N >, int res = search( N + 1, Status< N + 1 >{} ) ) {
   return res;
} 

// is already in the next state of the latest state, returns results and adds states
template< int N, int res = decltype( stateExist( Status< N - 1 >{} ) )::res >
static constexpr int search( float, Status< N >, int newS = State< N, Res< N * res > >::state ) {                         
   return res;
}

// compile time factorial iterator
template< int N = 1 >
static constexpr int fact_iter( int res = search( N, Status< N >{} ) ) {
   return res;
}

void test() {
   static_assert(fact_iter() == 1, "1");
   static_assert(fact_iter() == 2, "2");
   static_assert(fact_iter() == 6, "6");
   static_assert(fact_iter() == 24, "24");
   static_assert(fact_iter() == 120, "120");
}