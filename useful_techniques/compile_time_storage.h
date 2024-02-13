#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

/**
 *
 */
struct TestData {
   int i = 0;
   TestData() : i( 0 ) { std::cout << "TestData();\n"; }
   TestData( int j ) : i( j ) { std::cout << "TestData( " << j << " );\n"; }
   TestData( const TestData& t ) : i( t.i ) { std::cout << "TestData( const " << t.i << "& );\n"; }
   TestData( TestData&& t ) : i( t.i ) { std::cout << "TestData( " << t.i << "&& );\n"; t.i = 0; }
   ~TestData() { std::cout << "~TestData( " << i << " );\n"; i = 0; }
   TestData& operator=( const TestData& t ) { new ( this ) TestData( t ); return *this; }
   TestData& operator=( TestData&& t ) { new ( this ) TestData( std::move( t ) ); return *this; }
};

std::ostream& operator<<( std::ostream& os, const TestData& td ) {
   return os << "TestData" << td.i;
}

std::ostream& operator<<( std::ostream& os, const std::shared_ptr< TestData >& td ) {
   return os << "TestData" << td->i;
}

/**
 *
 */
template< typename Type >
struct TypeStorage : public Type::Storage {};

/**
 *
 */
template< class Type, typename ...Args >
static auto getTypePtr( int id, Args&& ...args ) -> typename TypeStorage< Type >::Storage::mapped_type {

   auto it = TypeStorage< Type >::instance().emplace( id, nullptr );
   if ( it.second && sizeof...( args ) > 0 ) {
      std::cout << "...1" << std::endl;
      it.first->second.reset( new Type{ id, std::forward< Args >( args )... } );
   }
   else if ( it.second ) {
     std::cout << "...2" << std::endl;
     it.first->second.reset( new Type{} );
   }
   else if ( sizeof...( args ) > 0 ) {
      std::cout << "...3" << std::endl;
      it.first->second->~Type();
      new ( it.first->second.get() ) Type{ id, std::forward< Args >( args )... };
   }
   else {
      std::cout << "...4" << std::endl;
   }
   return it.first->second;
}

template< class Type >
static auto getTypePtr() -> typename TypeStorage< Type >::Storage::mapped_type {

   auto id = Type::typeId();
   auto it = TypeStorage< Type >::instance().emplace( id, nullptr );
   if ( it.second ) {
      std::cout << "...5" << std::endl;
      it.first->second.reset( new Type{} );
   }
   else if ( it.first->second->id != id ) {
      std::cout << "...6" << std::endl;
      Type* ptr = static_cast< Type* >( it.first->second.get() );
      ptr->~Type();
      new ( ptr ) Type{};
   }
   else {
      std::cout << "...7" << std::endl;
   }
   return it.first->second;
}

/**
 *
 */
struct CType {
   int id;
   TestData data;
   std::vector< std::weak_ptr< CType > > array;

   void print() {
      std::cout << "print: " << id << " " << data << "{";
      std::for_each( array.begin(), array.end(), []( const std::weak_ptr< CType >& ct ) { 
         std::cout << "\n   "; ct.lock()->print(); } );
      std::cout << "}" << std::endl;
   }
};

template<>
struct TypeStorage< CType > {
   using Storage = std::map< int, std::shared_ptr< CType > >;
   static Storage& instance() {
      static Storage st;
      return st;
   }
};

/**
 *
 */
namespace N1 {
struct CTypeImpl100 : public CType {
   using Storage = TypeStorage< CType >;
   static int typeId() { return 100; }
   CTypeImpl100();
};
} // N3

namespace N2 {
struct CTypeImpl1000 : public CType {
   using Storage = TypeStorage< CType >;
   static int typeId() { return 1000; }
   CTypeImpl1000();
};
} // N2

N1::CTypeImpl100::CTypeImpl100() : CType{ typeId(), typeId() } {
   array.push_back( getTypePtr< N2::CTypeImpl1000 >() ); }
N2::CTypeImpl1000::CTypeImpl1000() : CType{ typeId(), typeId() } {}

template< typename T >
struct type_deleter {
   void operator ()( T* p) { 
      delete[] p; 
   }
};

/**
 * 
 */
void testInplaceConstruction() {
   // 1.
   getTypePtr< CType >( N1::CTypeImpl100::typeId() );
   getTypePtr< N1::CTypeImpl100 >();
   
   // 2.
//   auto ptr = getTypePtr< CType >( N1::CTypeImpl100::typeId() );
//   ptr->id = N1::CTypeImpl100::typeId();
//   ptr->data = { N1::CTypeImpl100::typeId() };
//   ptr->array.push_back( getTypePtr< N2::CTypeImpl1000 >() );
   
   // 3.
//   std::shared_ptr< CType > ptr;
//   ptr.reset( static_cast< CType* >( operator new( sizeof( CType ) ) ) );
//   std::shared_ptr< CType > ptr1 = ptr;
//   ptr1->print();
//   new( ptr.get() ) CType{ 42, 42 };
//   ptr1->print();
}