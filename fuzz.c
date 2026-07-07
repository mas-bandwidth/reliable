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

// This fuzzer drives two endpoints exchanging real traffic across an unreliable
// link, so it exercises the full send -> fragment -> reassemble -> ack path (the
// interesting parser and reassembly code), not just receive() of random bytes.
//
// On top of valid traffic it layers adversarial behaviour: packet loss,
// reordering, duplication, single-bit corruption of otherwise-valid packets, and
// injection of fully random packets. Run it under AddressSanitizer / UBSan to
// catch memory errors and undefined behaviour:
//
//     clang -g -O1 -fsanitize=address,undefined -DRELIABLE_DEBUG \
//           -I. reliable.c fuzz.c -o fuzz && ./fuzz 1000000 <seed>
//
// Usage: fuzz [num_iterations] [seed]   (num_iterations <= 0 runs until Ctrl-C)

#include "reliable.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <signal.h>
#include <inttypes.h>
#include <time.h>

#define MAX_PACKET_BYTES (16*1024)

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

int random_int( int a, int b )
{
    assert( a <= b );
    int result = a + rand() % ( b - a + 1 );
    assert( result >= a );
    assert( result <= b );
    return result;
}

double global_time = 100.0;

// Simulated network link: packets a transmit callback hands us are copied into a
// bounded queue, then flushed (with reordering / loss / duplication / corruption)
// into the destination endpoint once per iteration.

#define MAX_QUEUE 8192

struct queued_packet_t
{
    uint8_t * data;
    int bytes;
};

struct link_t
{
    struct queued_packet_t packets[MAX_QUEUE];
    int num_packets;
};

static void link_queue( struct link_t * link, uint8_t * data, int bytes )
{
    if ( link->num_packets == MAX_QUEUE || bytes <= 0 )
        return;
    uint8_t * copy = (uint8_t*) malloc( bytes );
    assert( copy );
    memcpy( copy, data, bytes );
    link->packets[link->num_packets].data = copy;
    link->packets[link->num_packets].bytes = bytes;
    link->num_packets++;
}

static void link_clear( struct link_t * link )
{
    int i;
    for ( i = 0; i < link->num_packets; ++i )
        free( link->packets[i].data );
    link->num_packets = 0;
}

// Flush the link into an endpoint, applying loss, reordering, duplication and
// single-bit corruption so the parser sees plausible-but-broken packets.

static void link_flush( struct link_t * link, struct reliable_endpoint_t * to )
{
    // fisher-yates shuffle for reordering
    int i;
    for ( i = link->num_packets - 1; i > 0; --i )
    {
        int j = rand() % ( i + 1 );
        struct queued_packet_t tmp = link->packets[i];
        link->packets[i] = link->packets[j];
        link->packets[j] = tmp;
    }

    for ( i = 0; i < link->num_packets; ++i )
    {
        uint8_t * data = link->packets[i].data;
        int bytes = link->packets[i].bytes;

        if ( random_int( 0, 99 ) < 10 )                             // 10% packet loss
            continue;

        if ( random_int( 0, 99 ) < 20 )                            // 20% single-bit corruption
            data[random_int( 0, bytes - 1 )] ^= (uint8_t) ( 1 << random_int( 0, 7 ) );

        int copies = random_int( 0, 99 ) < 5 ? 2 : 1;              // 5% duplication
        int c;
        for ( c = 0; c < copies; ++c )
            reliable_endpoint_receive_packet( to, data, bytes );
    }

    link_clear( link );
}

struct fuzz_context_t
{
    struct reliable_endpoint_t * client;
    struct reliable_endpoint_t * server;
    struct link_t client_to_server;
    struct link_t server_to_client;
};

static struct fuzz_context_t ctx;

void test_transmit_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) sequence;
    if ( id == 0 )
        link_queue( &ctx.client_to_server, packet_data, packet_bytes );
    else
        link_queue( &ctx.server_to_client, packet_data, packet_bytes );
}

int test_process_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;
    return 1;
}

static struct reliable_endpoint_t * make_endpoint( uint64_t id, const char * name )
{
    struct reliable_config_t config;
    reliable_default_config( &config );
    reliable_copy_string( config.name, name, sizeof( config.name ) );
    config.context = &ctx;
    config.id = id;
    config.transmit_packet_function = &test_transmit_packet_function;
    config.process_packet_function = &test_process_packet_function;
    return reliable_endpoint_create( &config, global_time );
}

void fuzz_initialize()
{
    reliable_init();
    memset( &ctx, 0, sizeof( ctx ) );
    ctx.client = make_endpoint( 0, "client" );
    ctx.server = make_endpoint( 1, "server" );
}

void fuzz_shutdown()
{
    printf( "\nshutdown\n" );
    link_clear( &ctx.client_to_server );
    link_clear( &ctx.server_to_client );
    reliable_endpoint_destroy( ctx.client );
    reliable_endpoint_destroy( ctx.server );
    reliable_term();
}

static void send_random_packet( struct reliable_endpoint_t * endpoint )
{
    static uint8_t packet_data[MAX_PACKET_BYTES];
    int packet_bytes = random_int( 1, MAX_PACKET_BYTES );
    int i;
    for ( i = 0; i < packet_bytes; ++i )
        packet_data[i] = (uint8_t) rand();
    reliable_endpoint_send_packet( endpoint, packet_data, packet_bytes );
}

static void inject_random_packet( struct reliable_endpoint_t * endpoint )
{
    static uint8_t packet_data[MAX_PACKET_BYTES];
    int packet_bytes = random_int( 1, MAX_PACKET_BYTES );
    int i;
    for ( i = 0; i < packet_bytes; ++i )
        packet_data[i] = (uint8_t) rand();
    reliable_endpoint_receive_packet( endpoint, packet_data, packet_bytes );
}

void fuzz_iteration( double time )
{
    // real traffic in both directions (most large enough to fragment)
    send_random_packet( ctx.client );
    send_random_packet( ctx.server );

    // deliver queued traffic across the lossy / reordering / corrupting link
    link_flush( &ctx.client_to_server, ctx.server );
    link_flush( &ctx.server_to_client, ctx.client );

    // inject fully random packets straight into receive (the classic fuzz path)
    inject_random_packet( ctx.client );
    inject_random_packet( ctx.server );

    reliable_endpoint_update( ctx.client, time );
    reliable_endpoint_update( ctx.server, time );

    reliable_endpoint_clear_acks( ctx.client );
    reliable_endpoint_clear_acks( ctx.server );
}

int main( int argc, char ** argv )
{
    printf( "[fuzz]\n" );

    int num_iterations = -1;
    unsigned int seed = (unsigned int) time( NULL );

    if ( argc >= 2 )
        num_iterations = atoi( argv[1] );
    if ( argc >= 3 )
        seed = (unsigned int) strtoul( argv[2], NULL, 10 );

    printf( "seed = %u\n", seed );
    srand( seed );

    fuzz_initialize();

    signal( SIGINT, interrupt_handler );

    double delta_time = 0.1;

    if ( num_iterations > 0 )
    {
        int i;
        for ( i = 0; i < num_iterations; ++i )
        {
            if ( quit )
                break;
            if ( ( i % 1000 ) == 0 )
            {
                printf( "." );
                fflush( stdout );
            }
            fuzz_iteration( global_time );
            global_time += delta_time;
        }
    }
    else
    {
        int i = 0;
        while ( !quit )
        {
            if ( ( i++ % 1000 ) == 0 )
            {
                printf( "." );
                fflush( stdout );
            }
            fuzz_iteration( global_time );
            global_time += delta_time;
        }
    }

    fuzz_shutdown();

    return 0;
}
