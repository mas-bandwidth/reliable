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

// libFuzzer harness for coverage-guided fuzzing (used by OSS-Fuzz).
//
// The fuzz input is interpreted as a script of operations against a pair of
// endpoints connected back to back: send a packet with attacker-chosen contents,
// or inject raw bytes straight into receive. This reaches the packet header
// parser, fragment reassembly, ack processing, and stale/duplicate rejection.
// Payloads are passed as exact-size heap copies so sanitizer redzones catch
// out-of-bounds reads.
//
// Build with a clang that ships libFuzzer:
//
//     clang -g -O1 -fsanitize=fuzzer,address,undefined -DRELIABLE_DEBUG -I. reliable.c fuzz_target.c -o fuzz_target
//
// Or build the standalone smoke driver (no libFuzzer required, runs random scripts):
//
//     cc -g -fsanitize=address,undefined -DRELIABLE_DEBUG -DRELIABLE_FUZZ_STANDALONE -I. reliable.c fuzz_target.c -o fuzz_target_standalone

#include "reliable.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define FUZZ_TARGET_MAX_PACKET_BYTES (16*1024)

struct fuzz_target_context_t
{
    struct reliable_endpoint_t * client;
    struct reliable_endpoint_t * server;
};

static struct fuzz_target_context_t fuzz_target_context;

static void fuzz_target_transmit_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) sequence;

    if ( id == 0 )
    {
        reliable_endpoint_receive_packet( fuzz_target_context.server, packet_data, packet_bytes );
    }
    else
    {
        reliable_endpoint_receive_packet( fuzz_target_context.client, packet_data, packet_bytes );
    }
}

static int fuzz_target_process_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    return 1;
}

int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    double time = 100.0;

    struct reliable_config_t config;

    reliable_default_config( &config );

    config.fragment_above = 500;                // fragment aggressively so reassembly is exercised by small inputs
    config.context = &fuzz_target_context;
    config.transmit_packet_function = &fuzz_target_transmit_packet_function;
    config.process_packet_function = &fuzz_target_process_packet_function;

    config.id = 0;
    fuzz_target_context.client = reliable_endpoint_create( &config, time );

    config.id = 1;
    fuzz_target_context.server = reliable_endpoint_create( &config, time );

    // the input is a sequence of operations: [op:1][length:2][payload:length]

    const uint8_t * p = data;
    size_t remaining = size;

    while ( remaining >= 3 )
    {
        int op = p[0] % 4;
        int length = 1 + ( ( ( (int) p[1] ) | ( ( (int) p[2] ) << 8 ) ) % FUZZ_TARGET_MAX_PACKET_BYTES );
        p += 3;
        remaining -= 3;

        if ( (size_t) length > remaining )
        {
            length = (int) remaining;
        }

        if ( length > 0 )
        {
            uint8_t * packet_data = (uint8_t*) malloc( length );
            memcpy( packet_data, p, length );

            switch ( op )
            {
                case 0:
                    reliable_endpoint_send_packet( fuzz_target_context.client, packet_data, length );
                    break;
                case 1:
                    reliable_endpoint_send_packet( fuzz_target_context.server, packet_data, length );
                    break;
                case 2:
                    reliable_endpoint_receive_packet( fuzz_target_context.client, packet_data, length );
                    break;
                case 3:
                    reliable_endpoint_receive_packet( fuzz_target_context.server, packet_data, length );
                    break;
            }

            free( packet_data );

            p += length;
            remaining -= length;
        }

        time += 0.01;

        reliable_endpoint_update( fuzz_target_context.client, time );
        reliable_endpoint_update( fuzz_target_context.server, time );

        reliable_endpoint_clear_acks( fuzz_target_context.client );
        reliable_endpoint_clear_acks( fuzz_target_context.server );
    }

    reliable_endpoint_destroy( fuzz_target_context.client );
    reliable_endpoint_destroy( fuzz_target_context.server );

    return 0;
}

#ifdef RELIABLE_FUZZ_STANDALONE

// standalone smoke driver: runs the harness on random scripts, so the harness
// itself is exercised in CI without needing libFuzzer

#include <stdio.h>
#include <time.h>

int main( int argc, char ** argv )
{
    printf( "[fuzz_target standalone]\n" );

    int num_iterations = 1000;
    unsigned int seed = (unsigned int) time( NULL );

    if ( argc >= 2 )
        num_iterations = atoi( argv[1] );
    if ( argc >= 3 )
        seed = (unsigned int) strtoul( argv[2], NULL, 10 );

    printf( "seed = %u\n", seed );
    srand( seed );

    reliable_init();

    static uint8_t data[64*1024];

    int i;
    for ( i = 0; i < num_iterations; ++i )
    {
        size_t size = (size_t) ( rand() % (int) sizeof( data ) );
        size_t j;
        for ( j = 0; j < size; ++j )
        {
            data[j] = (uint8_t) rand();
        }
        LLVMFuzzerTestOneInput( data, size );
    }

    reliable_term();

    printf( "passed\n" );

    return 0;
}

#endif // #ifdef RELIABLE_FUZZ_STANDALONE
