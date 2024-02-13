#pragma once

#include <QTextStream>
#include <QDebug>


enum Encoding {
   HEX,
};


/**
 *
 */
void tr2unicode( Encoding enc ) {

   QTextStream stream( stdin );
   QString line;

   while ( stream.readLineInto( &line ) ) {
      switch ( enc ) {
         case HEX:
            qDebug() << hex2unicode( line );
            break;
         default:
            qDebug() << "unsupported text encoding";
            break;
      }
   }
}


/**
 *
 */
QString hex2unicode( const QString &text ) {

   QString ht = text;
   return ht.replace( "\\xd0\\x90", "А" )
         .replace( "\\xd0\\x91", "Б" )
         .replace( "\\xd0\\x92", "В" )
         .replace( "\\xd0\\x93", "Г" )
         .replace( "\\xd0\\x94", "Д" )
         .replace( "\\xd0\\x95", "Е" )
         .replace( "\\xd0\\x96", "Ж" )
         .replace( "\\xd0\\x97", "З" )
         .replace( "\\xd0\\x98", "И" )
         .replace( "\\xd0\\x99", "Й" )
         .replace( "\\xd0\\x9a", "К" )
         .replace( "\\xd0\\x9b", "Л" )
         .replace( "\\xd0\\x9c", "М" )
         .replace( "\\xd0\\x9d", "Н" )
         .replace( "\\xd0\\x9e", "О" )
         .replace( "\\xd0\\x9f", "П" )
         .replace( "\\xd0\\xa0", "Р" )
         .replace( "\\xd0\\xa1", "С" )
         .replace( "\\xd0\\xa2", "Т" )
         .replace( "\\xd0\\xa3", "У" )
         .replace( "\\xd0\\xa4", "Ф" )
         .replace( "\\xd0\\xa5", "Х" )
         .replace( "\\xd0\\xa6", "Ц" )
         .replace( "\\xd0\\xa7", "Ч" )
         .replace( "\\xd0\\xa8", "Ш" )
         .replace( "\\xd0\\xa9", "Щ" )
         .replace( "\\xd0\\xaa", "Ъ" )
         .replace( "\\xd0\\xab", "Ы" )
         .replace( "\\xd0\\xac", "Ь" )
         .replace( "\\xd0\\xad", "Э" )
         .replace( "\\xd0\\xae", "Ю" )
         .replace( "\\xd0\\xaf", "Я" )
         .replace( "\\xd0\\xb0", "а" )
         .replace( "\\xd0\\xb1", "б" )
         .replace( "\\xd0\\xb2", "в" )
         .replace( "\\xd0\\xb3", "г" )
         .replace( "\\xd0\\xb4", "д" )
         .replace( "\\xd0\\xb5", "е" )
         .replace( "\\xd0\\xb6", "ж" )
         .replace( "\\xd0\\xb7", "з" )
         .replace( "\\xd0\\xb8", "и" )
         .replace( "\\xd0\\xb9", "й" )
         .replace( "\\xd0\\xba", "к" )
         .replace( "\\xd0\\xbb", "л" )
         .replace( "\\xd0\\xbc", "м" )
         .replace( "\\xd0\\xbd", "н" )
         .replace( "\\xd0\\xbe", "о" )
         .replace( "\\xd0\\xbf", "п" )
         .replace( "\\xd1\\x80", "р" )
         .replace( "\\xd1\\x81", "с" )
         .replace( "\\xd1\\x82", "т" )
         .replace( "\\xd1\\x83", "у" )
         .replace( "\\xd1\\x84", "ф" )
         .replace( "\\xd1\\x85", "х" )
         .replace( "\\xd1\\x86", "ц" )
         .replace( "\\xd1\\x87", "ч" )
         .replace( "\\xd1\\x88", "ш" )
         .replace( "\\xd1\\x89", "щ" )
         .replace( "\\xd1\\x8a", "ъ" )
         .replace( "\\xd1\\x8b", "ы" )
         .replace( "\\xd1\\x8c", "ь" )
         .replace( "\\xd1\\x8d", "э" )
         .replace( "\\xd1\\x8e", "ю" )
         .replace( "\\xd1\\x8f", "я" )
         .replace( "\\xd1\\x01", "Ё" )
         .replace( "\\xd1\\x91", "ё" );
}
