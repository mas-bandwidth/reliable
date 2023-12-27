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
#include <string.h>
#include <stdlib.h>

struct connection_t
{
    struct reliable_endpoint_t * endpoint;
};

static struct connection_t client;
static struct connection_t server;

static void transmit_packet( void * context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) index;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    // simulate 90% packet loss

    if ( ( rand() % 10 ) != 0 )
    {
        return;
    }

    // send the packet directly to the other endpoint

    if ( context == &client )
    {
        reliable_endpoint_receive_packet( client.endpoint, packet_data, packet_bytes );
    }
    else
    {
        reliable_endpoint_receive_packet( server.endpoint, packet_data, packet_bytes );
    }
}

static int process_packet( void * context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) index;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    // you would read the packet here and process its contents, return 0 if the packet should not be acked

    return 1;
}

#include <stdio.h>
#include <assert.h>

int main( int argc, char ** argv )
{
    (void) argc;
    (void) argv;

    printf( "\nreliable example\n\n" );

    double time = 0.0;

    // configure the endpoints

    struct reliable_config_t config;
    
    reliable_default_config( &config );

    config.max_packet_size = 32 * 1024;
    config.fragment_above = 1200;
    config.max_fragments = 32;
    config.fragment_size = 1024;
    config.transmit_packet_function = transmit_packet;
    config.process_packet_function = process_packet;

    // create client connection

    config.context = &client;
    reliable_copy_string( config.name, "client", sizeof( config.name ) );
    client.endpoint = reliable_endpoint_create( &config, time );
    if ( client.endpoint == NULL )
    {
        printf( "error: could not create client endpoint\n" );
        exit(1);
    }

    // create server connection

    config.context = &server;
    reliable_copy_string( config.name, "server", sizeof( config.name ) );
    server.endpoint = reliable_endpoint_create( &config, time );
    if ( server.endpoint == NULL )
    {
        printf( "error: could not create server endpoint\n" );
        exit(1);
    }

    // send packets and print out acks

    uint8_t packet[8];
    memset( packet, 0, sizeof( packet ) );

    for ( int i = 0; i < 1000; i++ )
    {
        uint16_t client_packet_sequence = reliable_endpoint_next_packet_sequence( client.endpoint );

        reliable_endpoint_send_packet( client.endpoint, packet, sizeof( packet ) );
        reliable_endpoint_send_packet( server.endpoint, packet, sizeof( packet ) );

        reliable_endpoint_update( client.endpoint, time );
        reliable_endpoint_update( server.endpoint, time );

        printf( "%d: client sent packet %d\n", i, client_packet_sequence );

        int num_acks;
        uint16_t * acks = reliable_endpoint_get_acks( client.endpoint, &num_acks );
        for ( int j = 0; j < num_acks; j++ )
        {
            printf( " --> server acked packet %d\n", acks[j] );
        }

        reliable_endpoint_clear_acks( client.endpoint );
        reliable_endpoint_clear_acks( server.endpoint );        

        time += 0.01;
    }

    // clean up

    reliable_endpoint_destroy( client.endpoint );
    reliable_endpoint_destroy( server.endpoint );

    printf( "\nSuccess\n\n" );

    return 0;
}
