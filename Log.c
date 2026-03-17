#include <stdarg.h>
#include <stdio.h>

#include "Log.h"

void log_info( const char* fmt, const char* file, int line, ... )
{
   printf( "\033[36m[INFO]\033[0m  " );

   va_list args;
   va_start( args, line );
   vprintf( fmt, args );
   va_end( args );

   printf( " | %s:%d\n", file, line );
}

void log_warn( const char* fmt, const char* file, int line, ... )
{
   printf( "\033[38;5;208m[WARN]\033[0m  " );

   va_list args;
   va_start( args, line );
   vprintf( fmt, args );
   va_end( args );

   printf( " | %s:%d\n", file, line );
}
