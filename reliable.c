/*
    reliable.io reference implementation

    Copyright © 2017, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <reliable.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>

#define RELIABLE_ENABLE_LOGGING 1

#ifndef RELIABLE_ENABLE_TESTS
#define RELIABLE_ENABLE_TESTS 1
#endif // #ifndef RELIABLE_ENABLE_TESTS

// ------------------------------------------------------------------

static int log_level = 0;

void reliable_log_level( int level )
{
    log_level = level;
}

#if RELIABLE_ENABLE_LOGGING

void reliable_printf( int level, const char * format, ... ) 
{
    if ( level > log_level )
        return;
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    va_end( args );
}

#else // #if RELIABLE_ENABLE_LOGGING

void reliable_printf( int level, const char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if RELIABLE_ENABLE_LOGGING

// ---------------------------------------------------------------

int reliable_init()
{
    // ...

    return 1;
}

void reliable_term()
{
    // ...
}

// ---------------------------------------------------------------

#if RELIABLE_ENABLE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static void check_handler( char * condition, 
                           char * function,
                           char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                                      \
do                                                                                              \
{                                                                                               \
    if ( !(condition) )                                                                         \
    {                                                                                           \
        check_handler( #condition, (char*) __FUNCTION__, (char*) __FILE__, __LINE__ );          \
    }                                                                                           \
} while(0)

static void test_endian()
{
    uint32_t value = 0x11223344;

    char * bytes = (char*) &value;

#if RELIABLE_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if RELIABLE_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if RELIABLE_LITTLE_ENDIAN
}

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

void reliable_test()
{
    printf( "\n" );

    //while ( 1 )
    {
        RUN_TEST( test_endian );
        // ...
    }

    printf( "\n*** ALL TESTS PASSED ***\n\n" );
}

#endif // #if RELIABLE_ENABLE_TESTS
