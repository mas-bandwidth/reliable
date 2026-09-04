/*
    Emits wire artifacts from the real library for tools/conformance/verify_standard.py.
    Two kinds:
      HDR  <sequence> <ack> <ack_bits> <bytes>   — reliable_write_packet_header output
      FRAG <sequence> <fragment_id> <num_fragments> <bytes> — a real fragment off the wire
      ACKCASE/ACKRECV/ACKHDR/ACKED: a stateful acknowledgment vector, a known set of
        sequences delivered to one endpoint, the header the endpoint then generates, and
        the sequences the far end reports acknowledged after consuming that header
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

/* ---------------------------------------------------------------------------
   Stateful acknowledgment vectors.

   Endpoint A sends a run of packets; only the sends named in deliver_index reach
   endpoint B. B then sends one packet, whose header carries B's acknowledgment
   state for exactly that set. A consumes the header and reports its acks.
   --------------------------------------------------------------------------- */

static struct reliable_endpoint_t * ack_receiver = NULL;
static int ack_deliver_now = 0;
static uint8_t ack_reply[256];
static int ack_reply_bytes = 0;

static void ack_transmit_a( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq;
    if ( ack_deliver_now )
        reliable_endpoint_receive_packet( ack_receiver, data, bytes );
}

static void ack_transmit_b( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq;
    if ( bytes <= (int) sizeof( ack_reply ) )
    {
        memcpy( ack_reply, data, bytes );
        ack_reply_bytes = bytes;
    }
}

static int contains( const int * set, int n, int value )
{
    for ( int i = 0; i < n; i++ )
        if ( set[i] == value ) return 1;
    return 0;
}

static int emit_ack_vector( const char * name, int num_sends, const int * deliver_index, int num_deliver )
{
    struct reliable_config_t config_a, config_b;

    reliable_default_config( &config_a );
    config_a.transmit_packet_function = ack_transmit_a;
    config_a.process_packet_function = on_process;

    reliable_default_config( &config_b );
    config_b.transmit_packet_function = ack_transmit_b;
    config_b.process_packet_function = on_process;

    struct reliable_endpoint_t * a = reliable_endpoint_create( &config_a, 0.0 );
    struct reliable_endpoint_t * b = reliable_endpoint_create( &config_b, 0.0 );
    if ( !a || !b ) { fprintf( stderr, "endpoint create failed\n" ); return 0; }

    ack_receiver = b;
    ack_reply_bytes = 0;

    uint8_t payload[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    printf( "ACKCASE %s\n", name );
    printf( "ACKRECV" );

    for ( int i = 0; i < num_sends; i++ )
    {
        uint16_t sequence = reliable_endpoint_next_packet_sequence( a );
        ack_deliver_now = contains( deliver_index, num_deliver, i );
        if ( ack_deliver_now )
            printf( " %d", (int) sequence );
        reliable_endpoint_send_packet( a, payload, sizeof( payload ) );
    }
    ack_deliver_now = 0;
    printf( "\n" );

    reliable_endpoint_send_packet( b, payload, sizeof( payload ) );
    printf( "ACKHDR " );
    hex( ack_reply, ack_reply_bytes );
    printf( "\n" );

    reliable_endpoint_receive_packet( a, ack_reply, ack_reply_bytes );

    int num_acks = 0;
    RELIABLE_CONST uint16_t * acks = reliable_endpoint_get_acks( a, &num_acks );
    printf( "ACKED" );
    for ( int i = 0; i < num_acks; i++ )
        printf( " %d", (int) acks[i] );
    printf( "\n" );

    reliable_endpoint_destroy( a );
    reliable_endpoint_destroy( b );
    ack_receiver = NULL;
    return 1;
}

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

    /* stateful acknowledgment vectors */

    /* a run with no wrap: the delivered set straddles the 32-wide window edge, so
       sequences 8 and below fall outside it and must not be represented */
    static const int plain[] = { 8, 9, 20, 40 };
    if ( !emit_ack_vector( "plain", 41, plain, 4 ) ) return 1;

    /* a run that crosses 65535 to 0: delivered sequences 65530, 65531, 65533, 65535,
       0, 1 and 4, so the represented window spans the wrap */
    static const int wrap[] = { 65530, 65531, 65533, 65535, 65536, 65537, 65540 };
    if ( !emit_ack_vector( "wrap", 65541, wrap, 7 ) ) return 1;

    reliable_term();
    return 0;
}
