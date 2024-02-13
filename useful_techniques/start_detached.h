#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <QString>

/**
 * Собственная реализация функции QProcess::startDetached, которая не умеет перенаправлять ввод/вывод.
 * Запускает команду program с заданными аргументами arguments в новом процессе и отцепляется от него.
 * Перенаправляет весь ввод и вывод исполняемой команды в файлы inputFile и outputFile соответственно.
 * Если файл не указан, то потоки перенавляются в /dev/null.
 */
void startDetached( const QString &program, const QStringList &arguments = QStringList(),
      const QString &inputFile = QString(), const QString &outputFile = QString() ) {

   pid_t forkPid = fork();
   if ( forkPid == 0 ) {
      setsid();
      signal( SIGCHLD, SIG_IGN );
      signal( SIGPIPE, SIG_IGN );

      pid_t doubleForkPid = fork();
      if ( doubleForkPid == 0 ) {
         chdir( "/" );
         umask( 0 );

         QFile in( !inputFile.isEmpty() ? inputFile : QStringLiteral( "/dev/null" ) );
         if ( in.open( QIODevice::ReadOnly ) ) {
            dup2( in.handle(), STDIN_FILENO );
            in.close();
         }

         QFile out( !outputFile.isEmpty() ? outputFile : QStringLiteral( "/dev/null" ) );
         out.setPermissions( static_cast< QFileDevice::Permission >( 0x6666 ) );
         if ( out.open( QIODevice::WriteOnly ) ) {
            dup2( out.handle(), STDOUT_FILENO );
            dup2( STDOUT_FILENO, STDERR_FILENO );
            out.close();
         }

         int argc = arguments.size() + 2;
         char **argv = new char*[ argc ];
         argv[ 0 ] = qstrdup( program.toUtf8().constData() );
         argv[ argc - 1 ] = 0;

         for ( int i = 0; i < arguments.size(); ++i ) {
             argv[i + 1] = qstrdup( arguments.at( i ).toUtf8().constData() );
         }

         if ( execvp( argv[ 0 ], argv ) == -1 ) {
            std::exit( 1 );
         }
      }

      else if ( doubleForkPid == -1 ) {
         std::exit( 1 );
      }

      std::exit( 0 );
   }

   else if ( forkPid == -1 ) {
      return;
   }

   waitpid( forkPid, nullptr, 0 );
}
