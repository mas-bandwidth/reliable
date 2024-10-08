[![Build status](https://github.com/mas-bandwidth/reliable/workflows/CI/badge.svg)](https://github.com/mas-bandwidth/reliable/actions?query=workflow%3ACI)

# Introduction

**reliable** is a simple packet acknowledgement system for UDP-based protocols.

It's useful in situations where you need to know which UDP packets you sent were received by the other side.

![image](https://github.com/mas-bandwidth/reliable/assets/696656/c58edcde-8bff-4683-8bc9-36c562ee7570)

It has the following features:

1. Acknowledgement when packets are received
2. Packet fragmentation and reassembly
3. RTT, jitter and packet loss estimates

reliable is stable and production ready.

# Usage

Reliable is designed to operate with your own network socket library.

If you don't have one already, try [netcode](https://github.com/mas-bandwidth/netcode), it's designed to work well with reliable.

First, create an endpoint on each side of the connection:

```c
struct reliable_config_t config;

reliable_default_config( &config );

config.max_packet_size = 32 * 1024;
config.fragment_above = 1200;
config.max_fragments = 32;
config.fragment_size = 1024;
config.transmit_packet_function = transmit_packet;
config.process_packet_function = process_packet;

reliable_endpoint_t * endpoint = reliable_endpoint_create( &config, time );
if ( endpoint == NULL )
{
  printf( "error: could not create endpoint\n" );
  exit(1);
}
```

For example, in a client/server setup you would have one endpoint on each client, and n endpoints on the server, one for each client slot.

Next, create a function to transmit packets:

```c
static void transmit_packet( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    // send packet using your own udp socket
}
```

And a function to process received packets:

```c
static int process_packet( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    // read the packet here and process its contents, return 0 if the packet should not be acked
    return 1;
}
```

For each packet you receive from your udp socket, call this on the endpoint that should receive it:

```c
reliable_endpoint_receive_packet( endpoint, packet_data, packet_bytes );
```

Now you can send packets through the endpoint:

```c
reliable_endpoint_send_packet( endpoint, packet_data, packet_bytes );
```

And get acks like this:

```c
int num_acks;
uint16_t * acks = reliable_endpoint_get_acks( endpoint, &num_acks );
for ( int i = 0; i < num_acks; i++ )
{
    printf( "acked packet %d\n", acks[j] );
}
```

Once you process all acks, clear them:

```c
reliable_endpoint_clear_acks( endpoint );
```

Before you send a packet, you can ask reliable what sequence number the sent packet will have:

```c
uint16_t sequence = reliable_endpoint_next_packet_sequence( endpoint );
```

This way you can map acked sequence numbers to the contents of packets you sent, for example, resending unacked messages until a packet that included that message was acked.

Make sure to update each endpoint once per-frame. This keeps track of network stats like latency, jitter, packet loss and bandwidth:

```c
reliable_endpoint_update( endpoint, time );
```

You can then grab stats from the endpoint:

```c
    printf( rtt = %.1fms | jitter = %.1fms | packet loss = %.1f%%\n", 
        reliable_endpoint_rtt_min( endpoint ),
        reliable_endpoint_jitter_avg_vs_min_rtt( endpoint ),
        reliable_endpoint_packet_loss( endpoint ) ),
```

When you are finished with an endpoint, destroy it:

```c
reliable_endpoint_destroy( endpoint );
```

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glenn-fiedler-11b735302/).

Open source libraries by the same author include: [netcode](https://github.com/mas-bandwidth/netcode), [serialize](https://github.com/mas-bandwidth/serialize),  and [yojimbo](https://github.com/mas-bandwidth/yojimbo)

If you find this software useful, [please consider sponsoring it](https://github.com/sponsors/mas-bandwidth). Thanks!

# License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
