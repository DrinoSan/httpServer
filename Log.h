#pragma once

// Macro to have my lines and file names included
#define LOG_INFO( fmt, ... ) log_info( fmt, __FILE__, __LINE__, ##__VA_ARGS__ )

void log_info( const char* fmt, const char* file, int line, ... );
