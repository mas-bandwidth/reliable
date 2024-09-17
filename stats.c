/*
    reliable

    Copyright © 2017 - 2024, Mas Bandwidth LLC

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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <inttypes.h>

#define MAX_PACKET_BYTES 290

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

int random_int( int a, int b )
{
    assert( a < b );
    int result = a + rand() % ( b - a + 1 );
    assert( result >= a );
    assert( result <= b );
    return result;
}

struct test_context_t
{
    struct reliable_endpoint_t * client;
    struct reliable_endpoint_t * server;
};

double global_time = 100.0;

struct test_context_t global_context;

void test_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) sequence;

    struct test_context_t * context = (struct test_context_t*) _context;

    if ( ( sequence % 5 ) == 0 )
        return;

    if ( id == 0 )
    {
        reliable_endpoint_receive_packet( context->server, packet_data, packet_bytes );
    }
    else if ( id == 1 )
    {
        reliable_endpoint_receive_packet( context->client, packet_data, packet_bytes );
    }
}

int generate_packet_data( uint16_t sequence, uint8_t * packet_data )
{
    int packet_bytes = MAX_PACKET_BYTES;
    assert( packet_bytes >= 2 );
    assert( packet_bytes <= MAX_PACKET_BYTES );
    packet_data[0] = (uint8_t) ( sequence & 0xFF );
    packet_data[1] = (uint8_t) ( (sequence>>8) & 0xFF );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( ( (int)i + sequence ) % 256 );
    }
    return packet_bytes;
}

static void check_handler( char * condition, 
                           char * function,
                           char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifdef RELIABLE_DEBUG
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

void check_packet_data( uint8_t * packet_data, int packet_bytes )
{
    assert( packet_bytes == MAX_PACKET_BYTES );
    uint16_t sequence = 0;
    sequence |= (uint16_t) packet_data[0];
    sequence |= ( (uint16_t) packet_data[1] ) << 8;
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( ( (int)i + sequence ) % 256 ) );
    }
}

int test_process_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    assert( packet_data );
    assert( packet_bytes > 0 );
    assert( packet_bytes <= MAX_PACKET_BYTES );

    (void) context;
    (void) id;
    (void) sequence;

    check_packet_data( packet_data, packet_bytes );

    return 1;
}

void stats_initialize()
{
    printf( "initializing\n" );

    reliable_init();

    memset( &global_context, 0, sizeof( global_context ) );
    
    struct reliable_config_t client_config;
    struct reliable_config_t server_config;

    reliable_default_config( &client_config );
    reliable_default_config( &server_config );

    client_config.fragment_above = MAX_PACKET_BYTES;
    server_config.fragment_above = MAX_PACKET_BYTES;

    reliable_copy_string( client_config.name, "client", sizeof( client_config.name ) );
    client_config.context = &global_context;
    client_config.id = 0;
    client_config.transmit_packet_function = &test_transmit_packet_function;
    client_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( server_config.name, "server", sizeof( server_config.name ) );
    server_config.context = &global_context;
    server_config.id = 1;
    server_config.transmit_packet_function = &test_transmit_packet_function;
    server_config.process_packet_function = &test_process_packet_function;

    global_context.client = reliable_endpoint_create( &client_config, global_time );
    global_context.server = reliable_endpoint_create( &server_config, global_time );
}

void stats_shutdown()
{
    printf( "shutdown\n" );

    reliable_endpoint_destroy( global_context.client );
    reliable_endpoint_destroy( global_context.server );

    reliable_term();
}

void stats_iteration( double time )
{
    uint8_t packet_data[MAX_PACKET_BYTES];
    memset( packet_data, 0, MAX_PACKET_BYTES );
    int packet_bytes;
    uint16_t sequence;

    sequence = reliable_endpoint_next_packet_sequence( global_context.client );
    packet_bytes = generate_packet_data( sequence, packet_data );
    reliable_endpoint_send_packet( global_context.client, packet_data, packet_bytes );

    sequence = reliable_endpoint_next_packet_sequence( global_context.server );
    packet_bytes = generate_packet_data( sequence, packet_data );
    reliable_endpoint_send_packet( global_context.server, packet_data, packet_bytes );

    reliable_endpoint_update( global_context.client, time );
    reliable_endpoint_update( global_context.server, time );

    reliable_endpoint_clear_acks( global_context.client );
    reliable_endpoint_clear_acks( global_context.server );

    RELIABLE_CONST uint64_t * counters = reliable_endpoint_counters( global_context.client );

    float sent_bandwidth_kbps, received_bandwidth_kbps, acked_bandwidth_kbps;
    reliable_endpoint_bandwidth( global_context.client, &sent_bandwidth_kbps, &received_bandwidth_kbps, &acked_bandwidth_kbps );

    printf( "%" PRIi64 " sent | %" PRIi64 " received | %" PRIi64 " acked | rtt = %dms | jitter = %dms | packet loss = %d%% | sent = %dkbps | recv = %dkbps | acked = %dkbps\n", 
        counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT],
        counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED],
        counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED],
        (int) reliable_endpoint_rtt_min( global_context.client ),
        (int) reliable_endpoint_jitter( global_context.client ),
        (int) ( reliable_endpoint_packet_loss( global_context.client ) + 0.5f ),
        (int) sent_bandwidth_kbps,
        (int) received_bandwidth_kbps,
        (int) acked_bandwidth_kbps );
}

int main( int argc, char ** argv )
{
    int num_iterations = -1;

    if ( argc == 2 )
        num_iterations = atoi( argv[1] );

    stats_initialize();

    signal( SIGINT, interrupt_handler );

    double delta_time = 0.01;

    if ( num_iterations > 0 )
    {
        int i;
        for ( i = 0; i < num_iterations; ++i )
        {
            if ( quit )
                break;

            stats_iteration( global_time );

            global_time += delta_time;
        }
    }
    else
    {
        while ( !quit )
        {
            stats_iteration( global_time );

            global_time += delta_time;
        }
    }

    stats_shutdown();
    
    return 0;
}
