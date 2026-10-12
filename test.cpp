/*
    reliable

    Copyright © 2017 - 2026, Más Bandwidth LLC

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

#include "reliable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

extern "C" void reliable_test();

// red-first canary for the estate floating point floor (CMakeLists.txt:64/:66,
// BUILDING.md). a * b + c rounds twice with -ffp-contract=off and once when the
// compiler contracts it to a fused multiply-add, and the two answers differ in the
// low bits. the inputs are read through volatile so the compiler cannot fold the
// expression to the correctly rounded result at compile time and hide the
// contraction: only a real fma instruction, or its absence, decides the bits. the
// expected zero was measured with clang 21 on arm64 (Apple) and gcc 13 on x86-64;
// -ffp-contract=on, =fast and the compiler default give 0x33800000 (float) and
// 0x3C90000000000000 (double) instead.
static void test_fp_contract_off()
{
    printf( "test_fp_contract_off\n" );

    // float: a = b = 1 + 2^-12, c = -( 1 + 2^-11 ).
    {
        volatile uint32_t a_bits = 0x3F800800u;
        volatile uint32_t b_bits = 0x3F800800u;
        volatile uint32_t c_bits = 0xBF801000u;

        float a, b, c;
        uint32_t bits;

        bits = a_bits; memcpy( &a, &bits, sizeof( a ) );
        bits = b_bits; memcpy( &b, &bits, sizeof( b ) );
        bits = c_bits; memcpy( &c, &bits, sizeof( c ) );

        volatile float result = a * b + c;
        float result_value = result;
        memcpy( &bits, &result_value, sizeof( bits ) );

        if ( bits != 0x00000000u )
        {
            printf( "check failed: ( float a * b + c == 0x00000000 ), got 0x%08x, function %s, file %s, line %d\n",
                    bits, __FUNCTION__, __FILE__, __LINE__ );
            fflush( stdout );
            exit( 1 );
        }
    }

    // double: a = b = 1 + 2^-27, c = -( 1 + 2^-26 ).
    {
        volatile uint64_t a_bits = 0x3FF0000002000000ull;
        volatile uint64_t b_bits = 0x3FF0000002000000ull;
        volatile uint64_t c_bits = 0xBFF0000004000000ull;

        double a, b, c;
        uint64_t bits;

        bits = a_bits; memcpy( &a, &bits, sizeof( a ) );
        bits = b_bits; memcpy( &b, &bits, sizeof( b ) );
        bits = c_bits; memcpy( &c, &bits, sizeof( c ) );

        volatile double result = a * b + c;
        double result_value = result;
        memcpy( &bits, &result_value, sizeof( bits ) );

        if ( bits != 0x0000000000000000ull )
        {
            printf( "check failed: ( double a * b + c == 0x0000000000000000 ), got 0x%016llx, function %s, file %s, line %d\n",
                    (unsigned long long) bits, __FUNCTION__, __FILE__, __LINE__ );
            fflush( stdout );
            exit( 1 );
        }
    }
}

int main( int argc, char ** argv )
{
	(void) argc;
	(void) argv;

   printf( "\n" );

   reliable_init();

   reliable_test();

   test_fp_contract_off();

   reliable_term();

   printf( "\n*** ALL TESTS PASSED ***\n\n" );

   return 0;
}
