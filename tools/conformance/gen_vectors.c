/*
    Emits wire artifacts from the real library for tools/conformance/verify_standard.py.
    Two kinds:
      HDR  <sequence> <ack> <ack_bits> <bytes>   — reliable_write_packet_header output
      FRAG <sequence> <fragment_id> <num_fragments> <bytes> — a real fragment off the wire
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "reliable.h"

int reliable_write_packet_header( uint8_t *, uint16_t, uint16_t, uint32_t );

static void hex( uint8_t * b, int n ) { for ( int i = 0; i < n; i++ ) printf( "%02x", b[i] ); }

static int frag_count = 0;
static void on_transmit( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq;
    printf( "FRAG %d ", frag_count++ );
    hex( data, bytes );
    printf( "\n" );
}
static int on_process( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{ (void) ctx; (void) id; (void) seq; (void) data; (void) bytes; return 1; }

int main( void )
{
    reliable_init();

    /* packet headers across the interesting corners of the elision rules */
    uint16_t seqs[] = { 0, 1, 255, 256, 1000, 32768, 65535, 7 };
    uint16_t acks[] = { 0, 1, 254, 255, 256, 999, 65535, 32760 };
    uint32_t bits[] = { 0xFFFFFFFF, 0x00000000, 0xFFFFFF00, 0x00FFFFFF,
                        0xDEADBEEF, 0xFF00FF00, 0x000000FF, 0xFFFF00FF };
    uint8_t buf[64];
    for ( int a = 0; a < 8; a++ )
    for ( int b = 0; b < 8; b++ )
    for ( int c = 0; c < 8; c++ )
    {
        int n = reliable_write_packet_header( buf, seqs[a], acks[b], bits[c] );
        printf( "HDR %u %u %u ", seqs[a], acks[b], bits[c] );
        hex( buf, n );
        printf( "\n" );
    }

    /* real fragments: a packet well above the fragment threshold */
    struct reliable_config_t config;
    reliable_default_config( &config );
    config.fragment_above = 500;
    config.fragment_size = 500;
    config.max_fragments = 16;
    config.max_packet_size = 8 * 1024;
    config.transmit_packet_function = on_transmit;
    config.process_packet_function = on_process;
    struct reliable_endpoint_t * endpoint = reliable_endpoint_create( &config, 0.0 );
    if ( !endpoint ) { fprintf( stderr, "endpoint create failed\n" ); return 1; }
    uint8_t payload[2200];
    for ( int i = 0; i < (int) sizeof( payload ); i++ ) payload[i] = (uint8_t) ( i * 7 );
    reliable_endpoint_send_packet( endpoint, payload, sizeof( payload ) );
    printf( "FRAGINFO %d %d\n", (int) sizeof( payload ), config.fragment_size );
    reliable_endpoint_destroy( endpoint );
    reliable_term();
    return 0;
}
