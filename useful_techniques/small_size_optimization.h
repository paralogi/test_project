#pragma once

#include <assert.h>
#include <QDebug>

// dynamic array for heap allocation
struct DynamicBuf {
   char* m_data;
   size_t m_size;

   void alloc( size_t size ) {
      m_data = new char( size );
   }

   void dealloc() {
      free( m_data );
   }

   void assign( const char *data, size_t size ) {
      memcpy( m_data, data, size );
      m_size = size;
   };

   char* data() const {
      return m_data;
   }

   size_t size() const {
      return m_size;
   }
};

// static array 16 bytes
struct StaticBuf {
   char m_data[ sizeof( DynamicBuf ) ];

   void assign( const char *data, size_t size ) {
      memcpy( m_data, data, size );
   };

   char* data() const {
      return const_cast< char* >( m_data );
   }

   size_t size() const {
      return sizeof( DynamicBuf );
   }
};

// small size optimization
union SmallBuf {
   DynamicBuf dbuf;
   StaticBuf sbuf;
};

// internal data container
struct Data {
   size_t m_capacity = 0;
   SmallBuf m_buffer;

   Data( const char *data ) {
      assert( *data );
      size_t size = strlen( data );
      reserve( size );
      assign( data, size );
   }

   Data( const char *data, size_t size ) {
      assert( *data && size > 0 );
      reserve( size );
      assign( data, size );
   }

   ~Data() {
      dealloc();
   }

   void reserve( size_t capacity ) {
      if ( capacity > sizeof( DynamicBuf ) ) {
         m_buffer.dbuf.alloc( capacity );
         m_capacity = capacity;
      }
   }

   size_t capacity() const {
      return m_capacity;
   }

   void alloc( size_t size ) {
      if ( size > sizeof( DynamicBuf ) ) {
         m_buffer.dbuf.alloc( size );
      }
   }

   void dealloc() {
      if ( m_capacity > 0 ) {
         m_buffer.dbuf.dealloc();
      }
   }

   void assign( const char *data, size_t size ) {
      if ( size > sizeof( DynamicBuf ) ) {
         m_buffer.dbuf.assign( data, size );
      }
      else {
         m_buffer.sbuf.assign( data, size );
      }
   };

   char* data() const {
      if ( m_capacity > 0 ) {
         return m_buffer.dbuf.data();
      }
      else {
         return m_buffer.sbuf.data();
      }
   }

   size_t size() const {
      if ( m_capacity > 0 ) {
         return m_buffer.dbuf.size();
      }
      else {
         return m_buffer.sbuf.size();
      }
   }
};

// my byte array
class MyByteArray {
public:
   MyByteArray() : d( nullptr ) {}
   MyByteArray( const char* data ) : d( new Data( data ) ) {}
   MyByteArray( const char* data, size_t size ) : d( new Data( data, size ) ) {}
   MyByteArray( MyByteArray&& other ) noexcept : d( std::move( other.d ) ) {}
   ~MyByteArray() {}

   void reserve( size_t capacity ) {
      d.reserve( capacity );
   }

   void assign( const char *data, size_t size ) {
      d.assign( data, size );
   }

   size_t capacity() {
      return d.capacity();
   }

   const char* data() {
      return d.data();
   }

   size_t size() {
      return d.size();
   }

   void print() {
      qDebug() << capacity() << size() << data();
   }

private:
   Data d;
};
