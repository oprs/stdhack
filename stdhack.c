
/*
 * Copyright (c)2014 Olivier Piras <stdhack@oprs.eu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define IOBLEN  1024
#define PATHLEN 4096

static void fdloop   ( int din, int dout, int derr );
static void pathexec ( char* argv[], char* envp[] );


int main( int argc, char *argv[], char *envp[] )
{
   pid_t  pid;
   int    status;
   int    pin[2], pout[2], perr[2];

   if( argc < 2 ) {
      (void)fprintf( stderr, "usage: %s prog [args...]\n", argv[0] );
      exit( EXIT_FAILURE );
   }

   ++argv; --argc;

   if(( pipe( pin ) < 0 ) || ( pipe( pout ) < 0 ) || ( pipe( perr ) < 0 )) {
      perror( "pipe()" );
      exit( EXIT_FAILURE );
   }

   if(( pid = fork()) < 0 ) {
      perror( "fork()" );
      exit( EXIT_FAILURE );
   }

   if( pid > 0 ) {
      (void)close(  pin[0] ); //  stdin: close the read end
      (void)close( pout[1] ); // stdout: close the write end
      (void)close( perr[1] ); // stderr: close the write end

      fdloop( pin[1], pout[0], perr[0] );

      (void)waitpid( pid, &status, 0 );
      exit( WEXITSTATUS( status ));
   }

   /* now in child */

   (void)close(  pin[1] ); //  stdin: close the write end
   (void)close( pout[0] ); // stdout: close the read end
   (void)close( perr[0] ); // stderr: close the read end

   (void)dup2( pin[0], STDIN_FILENO  );
   (void)close( pin[0] );

   (void)dup2( pout[1], STDOUT_FILENO );
   (void)close( pout[1] );

   (void)dup2( perr[1], STDERR_FILENO );
   (void)close( perr[1] );

   pathexec( argv, envp );

   /* never reached */
   return EXIT_FAILURE;
}


void fdloop( din, dout, derr )
{
   fd_set rset;
   size_t n;
   int rv;

   int dmax = dout > derr ? dout : derr;

   char *x = (char *)malloc( IOBLEN );
   if( x == NULL ) {
      perror( "malloc()" );
      exit( EXIT_FAILURE );
   }

   FD_ZERO( &rset );

   for( ;; ) {
      FD_SET( 0,    &rset );
      FD_SET( dout, &rset );
      FD_SET( derr, &rset );

      rv = select( dmax + 1, &rset, NULL, NULL, NULL );

      if( rv < 0 ) {
         perror( "select()" );
         exit( EXIT_FAILURE );
      }

      if( rv == 0 ) {
         (void)printf( "EOF\n" );
         break;
      }

      if( FD_ISSET( 0, &rset )) {
         n = read( 0, x, IOBLEN );
         if( n == 0 ) {
            (void)close( din );
            break;
         }

         if( write( din, x, n ) != n ) break;
      }

      if( FD_ISSET( dout, &rset )) {
         n = read( dout, x, IOBLEN );
         if( n == 0 ) break;
         if( write( 1, x, n ) != n ) break;
      }

      if( FD_ISSET( derr, &rset )) {
         n = read( derr, x, IOBLEN );
         if( n == 0 ) break;
         if( write( 2, x, n ) != n ) break;
      }
   }

   free( x );
}


void pathexec( char *argv[], char *envp[] )
{
   int len;

   char *path = argv[0];
   if( strchr( path, '/' )) {
      execve( path, argv, envp );
      perror( path );
      _exit( EXIT_FAILURE );
   }

   char *x = (char*)malloc( PATHLEN );
   if( x == NULL ) {
      perror( "malloc()" );
      _exit( EXIT_FAILURE );
   }

   char *loff = getenv( "PATH" );
   if( !loff ) loff = "/bin:/usr/bin";

   char *roff = strchr( loff, ':' );

   while( roff != NULL ) {
      len = roff - loff;
      if( len > 0 ) {
         (void)snprintf( x, PATHLEN, "%.*s/%s", len, loff, path );
         (void)execve( x, argv, envp );
      }
      loff += len + 1;
      roff  = strchr( loff, ':' );
   }

   if( loff[0] == '\0' ) {
      errno = ENOENT;
   } else {
      (void)snprintf( x, PATHLEN, "%s/%s", loff, path );
      (void)execve( x, argv, envp );
   }

   perror( path );
   _exit( EXIT_FAILURE );
}

/*EoF*/
