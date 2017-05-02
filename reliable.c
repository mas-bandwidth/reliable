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

#define RELIABLE_HEADER_BYTES 9

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

int reliable_sequence_greater_than( uint16_t s1, uint16_t s2 )
{
    return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
           ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

int reliable_sequence_less_than( uint16_t s1, uint16_t s2 )
{
    return reliable_sequence_greater_than( s2, s1 );
}

// ---------------------------------------------------------------

struct reliable_sequence_buffer_t
{
    uint16_t sequence;
    int num_entries;
    int entry_stride;
    uint32_t * entry_sequence;
    uint8_t * entry_data;
};

struct reliable_sequence_buffer_t * reliable_sequence_buffer_create( int num_entries, int entry_stride )
{
    assert( num_entries > 0 );
    assert( entry_stride > 0 );

    struct reliable_sequence_buffer_t * sequence_buffer = (struct reliable_sequence_buffer_t*) malloc( sizeof( struct reliable_sequence_buffer_t ) );

    sequence_buffer->sequence = 0;
    sequence_buffer->num_entries = num_entries;
    sequence_buffer->entry_stride = entry_stride;
    sequence_buffer->entry_sequence = (uint32_t*) malloc( num_entries * sizeof( uint32_t ) );
    sequence_buffer->entry_data = (uint8_t*) malloc( num_entries * entry_stride );

    memset( sequence_buffer->entry_sequence, 0xFF, sizeof( uint32_t) * sequence_buffer->num_entries );

    return sequence_buffer;
}

void reliable_sequence_buffer_destroy( struct reliable_sequence_buffer_t * sequence_buffer )
{
    assert( sequence_buffer );

    free( sequence_buffer->entry_sequence );
    free( sequence_buffer->entry_data );

    memset( sequence_buffer, 0, sizeof( struct reliable_sequence_buffer_t ) );

    free( sequence_buffer );
}

void reliable_sequence_buffer_reset( struct reliable_sequence_buffer_t * sequence_buffer )
{
    assert( sequence_buffer );
    sequence_buffer->sequence = 0;
    memset( sequence_buffer->entry_sequence, 0xFF, sizeof( uint32_t) * sequence_buffer->num_entries );
}

void reliable_sequence_buffer_remove_entries( struct reliable_sequence_buffer_t * sequence_buffer, int start_sequence, int finish_sequence )
{
    assert( sequence_buffer );
    if ( finish_sequence < start_sequence ) 
    {
        finish_sequence += 65535;
    }
    if ( finish_sequence - start_sequence < sequence_buffer->num_entries )
    {
        int sequence;
        for ( sequence = start_sequence; sequence <= finish_sequence; ++sequence )
        {
            sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] = 0xFFFFFFFF;
        }
    }
    else
    {
        for ( int i = 0; i < sequence_buffer->num_entries; ++i )
        {
            sequence_buffer->entry_sequence[i] = 0xFFFFFFFF;
        }
    }
}

void * reliable_sequence_buffer_insert( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence );
        sequence_buffer->sequence = sequence + 1;
    }
    else if ( reliable_sequence_less_than( sequence, sequence_buffer->sequence - sequence_buffer->num_entries ) )
    {
        return NULL;
    }
    int index = sequence % sequence_buffer->num_entries;
    sequence_buffer->entry_sequence[index] = sequence;
    return sequence_buffer->entry_data + index * sequence_buffer->entry_stride;
}

void reliable_sequence_buffer_remove( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    assert( sequence_buffer );
    sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] = 0xFFFFFFFF;
}

int reliable_sequence_buffer_available( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    assert( sequence_buffer );
    return sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] == 0xFFFFFFFF;
}

int reliable_sequence_buffer_exists( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    assert( sequence_buffer );
    return sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] == (uint32_t) sequence;
}

void * reliable_sequence_buffer_find( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    assert( sequence_buffer );
    int index = sequence % sequence_buffer->num_entries;
    return ( ( sequence_buffer->entry_sequence[index] == (uint32_t) sequence ) ) ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;
}

void * reliable_sequence_buffer_at_index( struct reliable_sequence_buffer_t * sequence_buffer, int index )
{
    assert( sequence_buffer );
    assert( index >= 0 );
    assert( index < sequence_buffer->num_entries );
    return sequence_buffer->entry_sequence[index] != 0xFFFFFFFF ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;
}

void reliable_sequence_buffer_generate_ack_bits( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t * ack, uint32_t * ack_bits )
{
    assert( sequence_buffer );
    assert( ack );
    assert( ack_bits );
    *ack = sequence_buffer->sequence - 1;
    *ack_bits = 0;
    uint32_t mask = 1;
    for ( int i = 0; i < 32; ++i )
    {
        uint16_t sequence = *ack - i;
        if ( reliable_sequence_buffer_exists( sequence_buffer, sequence ) )
            *ack_bits |= mask;
        mask <<= 1;
    }
}

// ---------------------------------------------------------------

void reliable_write_uint8( uint8_t ** p, uint8_t value )
{
    **p = value;
    ++(*p);
}

void reliable_write_uint16( uint8_t ** p, uint16_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = value >> 8;
    *p += 2;
}

void reliable_write_uint32( uint8_t ** p, uint32_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = value >> 24;
    *p += 4;
}

void reliable_write_uint64( uint8_t ** p, uint64_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = ( value >> 24 ) & 0xFF;
    (*p)[4] = ( value >> 32 ) & 0xFF;
    (*p)[5] = ( value >> 40 ) & 0xFF;
    (*p)[6] = ( value >> 48 ) & 0xFF;
    (*p)[7] = value >> 56;
    *p += 8;
}

void reliable_write_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        reliable_write_uint8( p, byte_array[i] );
    }
}

uint8_t reliable_read_uint8( uint8_t ** p )
{
    uint8_t value = **p;
    ++(*p);
    return value;
}

uint16_t reliable_read_uint16( uint8_t ** p )
{
    uint16_t value;
    value = (*p)[0];
    value |= ( ( (uint16_t)( (*p)[1] ) ) << 8 );
    *p += 2;
    return value;
}

uint32_t reliable_read_uint32( uint8_t ** p )
{
    uint32_t value;
    value  = (*p)[0];
    value |= ( ( (uint32_t)( (*p)[1] ) ) << 8 );
    value |= ( ( (uint32_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint32_t)( (*p)[3] ) ) << 24 );
    *p += 4;
    return value;
}

uint64_t reliable_read_uint64( uint8_t ** p )
{
    uint64_t value;
    value  = (*p)[0];
    value |= ( ( (uint64_t)( (*p)[1] ) ) << 8  );
    value |= ( ( (uint64_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint64_t)( (*p)[3] ) ) << 24 );
    value |= ( ( (uint64_t)( (*p)[4] ) ) << 32 );
    value |= ( ( (uint64_t)( (*p)[5] ) ) << 40 );
    value |= ( ( (uint64_t)( (*p)[6] ) ) << 48 );
    value |= ( ( (uint64_t)( (*p)[7] ) ) << 56 );
    *p += 8;
    return value;
}

void reliable_read_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        byte_array[i] = reliable_read_uint8( p );
    }
}

// ---------------------------------------------------------------

struct reliable_endpoint_t
{
    struct reliable_config_t config;
    int num_acks;
    uint16_t * acks;
    uint16_t sequence;
    struct reliable_sequence_buffer_t * sent_packets;
    struct reliable_sequence_buffer_t * received_packets;
    uint64_t counters[RELIABLE_ENDPOINT_NUM_COUNTERS];
};

struct reliable_sent_packet_data_t
{
    uint8_t acked;
};

struct reliable_received_packet_data_t
{
    int dummy;
};

struct reliable_endpoint_t * reliable_endpoint_create( struct reliable_config_t * config )
{
    assert( config );

    // todo: we probably want to validate the config here somehow

    struct reliable_endpoint_t * endpoint = (struct reliable_endpoint_t*) malloc( sizeof( struct reliable_endpoint_t ) );

    memset( endpoint, 0, sizeof( struct reliable_endpoint_t ) );

    endpoint->config = *config;
    endpoint->acks = (uint16_t*) malloc( config->ack_buffer_size * sizeof( uint16_t ) );
    endpoint->sent_packets = reliable_sequence_buffer_create( config->sent_packets_buffer_size, sizeof( struct reliable_sent_packet_data_t ) );
    endpoint->received_packets = reliable_sequence_buffer_create( config->received_packets_buffer_size, sizeof( struct reliable_received_packet_data_t ) );

    return endpoint;
}

void reliable_endpoint_destroy( struct reliable_endpoint_t * endpoint )
{
    assert( endpoint );
    assert( endpoint->acks );
    assert( endpoint->sent_packets );
    assert( endpoint->received_packets );

    free( endpoint->acks );
    free( endpoint->sent_packets );
    free( endpoint->received_packets );

    memset( endpoint, 0, sizeof( struct reliable_endpoint_t ) );

    free( endpoint );
}

uint16_t reliable_endpoint_next_packet_sequence( struct reliable_endpoint_t * endpoint )
{
    assert( endpoint );
    return endpoint->sent_packets->sequence;
}

void reliable_endpoint_send_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    assert( endpoint );
    assert( packet_data );
    assert( packet_bytes > 0 );

    uint16_t sequence = endpoint->sequence++;
    uint16_t ack;
    uint32_t ack_bits;

    reliable_sequence_buffer_generate_ack_bits( endpoint->sent_packets, &ack, &ack_bits );

    struct reliable_sent_packet_data_t * sent_packet_data = reliable_sequence_buffer_insert( endpoint->sent_packets, sequence );

    sent_packet_data->acked = 0;

    uint8_t * transmit_packet_data = malloc( packet_bytes + RELIABLE_HEADER_BYTES );

    uint8_t * p = transmit_packet_data;

    reliable_write_uint8( &p, 0 );
    reliable_write_uint16( &p, sequence );
    reliable_write_uint16( &p, ack );
    reliable_write_uint32( &p, ack_bits );

    assert( p - transmit_packet_data == RELIABLE_HEADER_BYTES );

    memcpy( transmit_packet_data + RELIABLE_HEADER_BYTES, packet_data, packet_bytes );

    endpoint->config.transmit_packet_function( endpoint->config.context, endpoint->config.index, transmit_packet_data, RELIABLE_HEADER_BYTES + packet_bytes );

    free( transmit_packet_data );
}

void reliable_endpoint_receive_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    assert( endpoint );
    assert( packet_data );
    assert( packet_bytes > 0 );

    (void) endpoint;
    (void) packet_data;
    (void) packet_bytes;

    if ( packet_bytes <= RELIABLE_HEADER_BYTES )
        return;

    uint8_t * p = packet_data;

    uint8_t prefix = reliable_read_uint8( &p );
    uint16_t sequence = reliable_read_uint16( &p );
    uint16_t ack = reliable_read_uint16( &p );
    uint32_t ack_bits = reliable_read_uint32( &p );

    (void) prefix;

    if ( endpoint->config.process_packet_function( endpoint->config.context, endpoint->config.index, packet_data + RELIABLE_HEADER_BYTES, packet_bytes - RELIABLE_HEADER_BYTES ) )
    {
        struct reliable_received_packet_data_t * received_packet_data = reliable_sequence_buffer_insert( endpoint->sent_packets, sequence );
        if ( received_packet_data )
        {
            // todo: fill received packet data (if any is needed)
        }
        else
        {
            // todo: inc stale packet counter
        }

        for ( int i = 0; i < 32; ++i )
        {
            if ( ack_bits & 1 )
            {                    
                const uint16_t sequence = ack - i;
                struct reliable_sent_packet_data_t * sent_packet_data = reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
                if ( sent_packet_data && !sent_packet_data->acked )
                {
                    if ( endpoint->num_acks < endpoint->config.ack_buffer_size )
                    {
                        endpoint->acks[endpoint->num_acks++] = sequence;
                        // todo: increment acked packet counter
                    }
                    else
                    {
                        // todo: increment dropped ack counter
                    }
                    sent_packet_data->acked = 1;
                }
            }
            ack_bits >>= 1;
        }
    }
}

void reliable_endpoint_free_packet( struct reliable_endpoint_t * endpoint, void * packet )
{
    assert( endpoint );
    assert( packet );

    (void) endpoint;

    free( packet );
}

uint16_t * reliable_endpoint_get_acks( struct reliable_endpoint_t * endpoint, int * num_acks )
{
    assert( endpoint );
    assert( num_acks );
    *num_acks = endpoint->num_acks;
    return endpoint->acks;
}

void reliable_endpoint_clear_acks( struct reliable_endpoint_t * endpoint )
{
    assert( endpoint );
    endpoint->num_acks = 0;
}

void reliable_endpoint_update( struct reliable_endpoint_t * endpoint )
{
    assert( endpoint );

    (void) endpoint;

    // todo: do we even need this? possibly for some jitter or QoS calculations or something
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

struct test_sequence_data_t
{
    uint16_t sequence;
};

#define TEST_SEQUENCE_BUFFER_SIZE 256

static void test_sequence_buffer()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, sizeof( struct test_sequence_data_t ) );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    int i;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, i ) == NULL );
    }

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE*4; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, i );
        check( entry );
        entry->sequence = i;
        check( sequence_buffer->sequence == i + 1 );
    }

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, i );
        check( entry == NULL );
    }    

    int index = TEST_SEQUENCE_BUFFER_SIZE * 4;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_find( sequence_buffer, index );
        check( entry );
        check( entry->sequence == (uint32_t) index );
        index--;
    }

    reliable_sequence_buffer_reset( sequence_buffer );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, i ) == NULL );
    }

    reliable_sequence_buffer_destroy( sequence_buffer );
}

void test_generate_ack_bits()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, sizeof( struct test_sequence_data_t ) );

    uint16_t ack = 0;
    uint32_t ack_bits = 0xFFFFFFFF;

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == 0xFFFF );
    check( ack_bits == 0 );

    int i;
    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, i );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == TEST_SEQUENCE_BUFFER_SIZE );
    check( ack_bits == 0xFFFFFFFF );

    reliable_sequence_buffer_reset( sequence_buffer );

    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( int i = 0; i < input_num_acks; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, input_acks[i] );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );

    check( ack == 11 );
    check( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );

    reliable_sequence_buffer_destroy( sequence_buffer );
}

struct test_context_t
{
    struct reliable_endpoint_t * sender;
    struct reliable_endpoint_t * receiver;
};

void test_transmit_packet_function( void * _context, int index, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

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

#define TEST_ACKS_NUM_ITERATIONS 256

void test_acks()
{
    struct test_context_t context;
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    sender_config.context = &context;
    sender_config.index = 0;
    sender_config.ack_buffer_size = 256;
    sender_config.sent_packets_buffer_size = 256;
    sender_config.received_packets_buffer_size = 256;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.index = 1;
    receiver_config.ack_buffer_size = 256;
    receiver_config.sent_packets_buffer_size = 256;
    receiver_config.received_packets_buffer_size = 256;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config );
    context.receiver = reliable_endpoint_create( &receiver_config );

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender );
        reliable_endpoint_update( context.receiver );
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == 1 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    uint16_t * receiver_acks = reliable_endpoint_get_acks( context.sender, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
            receiver_acked_packet[receiver_acks[i]] = 1;
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == 1 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

void test_acks_packet_loss()
{
    struct test_context_t context;
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    sender_config.context = &context;
    sender_config.index = 0;
    sender_config.ack_buffer_size = 256;
    sender_config.sent_packets_buffer_size = 256;
    sender_config.received_packets_buffer_size = 256;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.index = 1;
    receiver_config.ack_buffer_size = 256;
    receiver_config.sent_packets_buffer_size = 256;
    receiver_config.received_packets_buffer_size = 256;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config );
    context.receiver = reliable_endpoint_create( &receiver_config );

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender );
        reliable_endpoint_update( context.receiver );
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == 1 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    uint16_t * receiver_acks = reliable_endpoint_get_acks( context.sender, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
            receiver_acked_packet[receiver_acks[i]] = 1;
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == 1 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
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

    while ( 1 )
    {
        RUN_TEST( test_endian );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_generate_ack_bits );
        RUN_TEST( test_acks );
        RUN_TEST( test_acks_packet_loss );
    }

    printf( "\n*** ALL TESTS PASSED ***\n\n" );
}

#endif // #if RELIABLE_ENABLE_TESTS
