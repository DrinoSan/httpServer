#include <stdarg.h>
#include <stdio.h>

#include "Log.h"

void log_info( const char* fmt, const char* file, int line, ... )
{
   printf( "[INFO] " );

   va_list args;
   va_start( args, line );
   vprintf( fmt, args );
   va_end( args );

   printf( " | %s:%d\n", file, line );
}
