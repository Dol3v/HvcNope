
#include "Log.h"

// Singleton logger instance
Logger& GetLogger() {
    static Logger logger( DEBUG ); // Set the default log level here
    return logger;
}

void Log( LogLevel level, const char* format, ... ) 
{
    char buffer[1024];
    va_list args;
    va_start( args, format );
    vsnprintf( buffer, sizeof( buffer ), format, args );
    va_end( args );

    GetLogger().Log( level, buffer );
}

void Logger::Log( LogLevel level, const std::string& message )
{
    if (level >= logLevel) {
        std::stringstream logStream;
        logStream << LogLevelToString( level ) << ": " << message;

        // Log to std::cout and std::cerr based on log level
        if (level == FAIL || level == WARN) {
            std::cerr << logStream.str();
        }
        else {
            std::cout << logStream.str();
        }

        // Log to debug output
        OutputDebugStringA( logStream.str().c_str() );
    }
}

