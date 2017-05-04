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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <signal.h>
#include <inttypes.h>

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
    struct reliable_endpoint_t * sender;
    struct reliable_endpoint_t * receiver;
};

struct test_context_t context;

void test_transmit_packet_function( void * _context, int index, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

    if ( random_int(0,100) < 5 )
        return;

    if ( index == 0 )
    {
        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
    }
    else if ( index == 1 )
    {
        reliable_endpoint_receive_packet( context->sender, packet_data, packet_bytes );
    }
}

int test_process_packet_function( void * _context, int index, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

    (void) context;
    (void) index;
    (void) packet_data;
    (void) packet_bytes;

    return 1;
}

void soak_initialize()
{
    printf( "initializing\n" );

    reliable_init();

    memset( &context, 0, sizeof( context ) );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.context = &context;
    sender_config.index = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.index = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config );
    context.receiver = reliable_endpoint_create( &receiver_config );
}

void soak_shutdown()
{
    printf( "shutdown\n" );

    reliable_endpoint_destroy( context.sender );

    reliable_endpoint_destroy( context.receiver );

    reliable_term();
}

void soak_iteration( double time )
{
    (void) time;

    uint8_t packet[16*1024];
    memset( packet, 0, sizeof( packet ) );

    uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );

    printf( "packet sent  %d\n", sequence );

    reliable_endpoint_send_packet( context.sender, packet, random_int( 1, sizeof( packet ) ) );
    reliable_endpoint_send_packet( context.receiver, packet, random_int( 1, sizeof( packet ) ) );

    reliable_endpoint_update( context.sender );
    reliable_endpoint_update( context.receiver );

    int sender_num_acks;
    uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    int i;
    for ( i = 0; i < sender_num_acks; ++i )
    {
        printf( "packet acked %d\n", sender_acks[i] );
    }
    reliable_endpoint_clear_acks( context.sender );
}

int main( int argc, char ** argv )
{
    int num_iterations = -1;

    if ( argc == 2 )
        num_iterations = atoi( argv[1] );

    printf( "[soak]\nnum_iterations = %d\n", num_iterations );

    soak_initialize();

    printf( "starting\n" );

    signal( SIGINT, interrupt_handler );

    double time = 0.0;
    double delta_time = 0.1;

    if ( num_iterations > 0 )
    {
        int i;
        for ( i = 0; i < num_iterations; ++i )
        {
            if ( quit )
                break;

            soak_iteration( time );

            time += delta_time;
        }
    }
    else
    {
        while ( !quit )
        {
            soak_iteration( time );

            time += delta_time;
        }
    }

    soak_shutdown();
    
    return 0;
}
