#pragma once

#include <memory>
#include <utility>
#include <iostream>
#include <map>

/**
 * Типы данных для печати
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
   bool operator<( const TestData& t ) { return i < t.i; }
};

std::ostream& operator<<( std::ostream& os, const TestData& td ) { 
   return os << td.i; 
}

std::ostream& operator<<( std::ostream& os, const std::shared_ptr< TestData >& td ) { 
   return os << td->i; 
}

bool operator<( const TestData& t1, const TestData& t2 ) {
   return t1.i < t2.i;
}

struct ZeroDesc { 
   std::string name{ "ZeroDesc" }; 
   std::unique_ptr< TestData > val{ new TestData{ 0 } };
   
   ZeroDesc() = default;
   ZeroDesc( const ZeroDesc& desc ) : name( desc.name ) {}
};

struct OneDesc { 
   std::string name{ "OneDesc" }; 
   std::unique_ptr< TestData > val{ new TestData{ 1 } }; 
   
   OneDesc() = default;
   OneDesc( const OneDesc& desc ) : name( desc.name ) {}
};

struct TwoDesc { 
   std::string name{ "TwoDesc" }; 
   std::unique_ptr< TestData > val{ new TestData{ 2 } }; 
   
   TwoDesc() = default;
   TwoDesc( const TwoDesc& desc ) : name( desc.name ) {}
};

/**
 * Классы принтеры, используются в фабрике для печати аргументов любого типов
 */
class Printer {
public:
   /**
    * Перечисление задает порядок печати для наследников принтера
    */
   enum { Zero, One, Two, Nan };
   
   /**
    * Виртуальный декструктор позволяет освободить ресурсы у наследуемых принтеров
    */
   virtual ~Printer() {}
   
   /**
    * Метод используется как отправная точка в SFINAE для поиска нужной реализации
    */
   bool print() { 
      std::cout << "!" << std::endl;
      return false; 
   }
};



template< int N = Printer::Nan >
class PrinterImpl : public Printer {};

template<>
class PrinterImpl< Printer::Zero > : public Printer {
public:
   bool print( const ZeroDesc& val ) { std::cout << val.name << std::endl; return false; }
   bool print( const int& val ) { std::cout << "int"; return false; }
   bool print( const char& val ) { std::cout << "char"; return false; }
   
private:
   std::unique_ptr< TestData > d{ new TestData( Printer::Zero ) };
};

template<>
class PrinterImpl< Printer::One > : public Printer {
public:
   bool print( const OneDesc& one, const TwoDesc& two ) { std::cout << one.name << " = " << one.val->i << std::endl; return false; }
   bool print( const int& val ) { std::cout << " = "; return false; }
   bool print( const char& val ) { std::cout << " = "; return false; }
   
private:
   std::unique_ptr< TestData > d{ new TestData( Printer::One ) };
};

template<>
class PrinterImpl< Printer::Two > : public Printer {
public:
   bool print( const ZeroDesc& val ) { std::cout << val.val->i << std::endl; return false; }
   bool print( const OneDesc& one, const TwoDesc& two ) { std::cout << two.name << " = " << two.val->i << std::endl; return false; }
   bool print( const int& val ) { std::cout << val << std::endl; return true; }
   bool print( const char& val ) { std::cout << val << std::endl; return true; }
   
private:
   std::unique_ptr< TestData > d{ new TestData( Printer::Two ) };
};

/**
 * Класс проверяет наличие у принтера метода для печати указанных аргументов
 */
template< int N = Printer::Nan >
class PrinterPrivate {
   using PrinterPtr = PrinterImpl< N >*;

public:
   PrinterPrivate( Printer* printer ) {
      m_printer = static_cast< PrinterPtr >( printer );
   }

   /**
    * Метод использует SFINAE для поиска реализации метода с подходящими типами аргументов
    */
   template< typename ...T >
   auto test_print( T&& ...val ) -> decltype( std::declval< PrinterPtr >()->print( std::forward< T >( val )... ) ) {
      return m_printer->print( std::forward< T >( val )... );
   }

   /**
    * Сюда подставляются все остальные типы аргументов, для которых нет реализации
    */
   bool test_print( ... ) {
      return m_printer->Printer::print();
   }
   
private:
   PrinterPtr m_printer;
};

/**
 * Шаблонная фабрика, использует наследников класса принтера для печати аргументов любого типа
 */
class PrinterFactory {
public:
   /**
    * Главный метод фабрики, который умеет печатать любые типы аргументов
    * для которых определена реализация в любом из классов принтеров
    */
   template< int N = Printer::Zero, typename ...T > 
   void print( T&& ...val ) {
      auto& printer = printers[ N ];
      if ( !printer )
         printer.reset( new PrinterImpl< N >() );
      if ( !PrinterPrivate< N >( printer.get() ).test_print( std::forward< T >( val )... ) )
         print< N + 1, T... >( std::forward< T >( val )... );
   }
   
   /**
    * Удаляются все принтеры в рантайме, затем их можно пересоздать заново
    */
   void clear() { 
      printers.clear(); 
   }

private:
   std::map< int, std::unique_ptr< Printer > > printers;
};

/**
 * Специализации функции фабрики для различных комбинаций типов аргументов
 */
template<> void PrinterFactory::print< Printer::Nan >( ZeroDesc&& zero ) {}
template<> void PrinterFactory::print< Printer::Nan >( ZeroDesc&& zero ) {}
template<> void PrinterFactory::print< Printer::Nan >( OneDesc&& one, TwoDesc&& two ) {}
template<> void PrinterFactory::print< Printer::Nan >( int&& val ) {}
template<> void PrinterFactory::print< Printer::Nan >( char&& val ) {}

/**
 * 
 */
static void testFactory() {
   PrinterFactory factory;
   ZeroDesc z;
   factory.print( z );
   factory.print( ZeroDesc() );
   factory.print( OneDesc(), TwoDesc() );
   factory.print( int( 111 ) );
   factory.print( char('a') );
   factory.clear();
}
