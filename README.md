[![Build status](https://github.com/mas-bandwidth/reliable/workflows/CI/badge.svg)](https://github.com/mas-bandwidth/reliable/actions?query=workflow%3ACI)

# Introduction

**reliable** is a simple packet acknowledgement system for UDP-based protocols.

It has the following features:

1. Acknowledgement when packets are received
2. Packet fragmentation and reassembly
3. RTT and packet loss estimates

reliable is stable and production ready.

# Usage

Reliable is designed to operate with your own network socket library.

If you don't have this already, try [netcode](https://github.com/mas-bandwidth/netcode), it was designed to work well with reliable.

First, create an endpoint on each side of the connection:

```c
struct reliable_config_t config;

reliable_default_config( &config );

config.max_packet_size = 32 * 1024;                     // maximum packet size that may be sent in bytes
config.fragment_above = 1200;                           // fragment and reassemble packets above this size
config.max_fragments = 32;                              // maximum number of fragments per-packet
config.fragment_size = 1024;                            // the size of each fragment sent
config.transmit_packet_function = transmit_packet;      // set the callback function to transmit packets
config.process_packet_function = process_packet;        // set the callback function to process packets

reliable_endpoint_t * endpoint = reliable_endpoint_create( &config, time );
if ( endpoint == NULL )
{
  printf( "error: could not create endpoint\n" );
  exit(1);
}
```

For example, in a client/server setup you would have one endpoint on each client, and n endpoints on the server, one for each client slot. In a fully connected peer-to-peer mesh with n peers, you would have n-1 endpoints on each peer.

You can name your endpoints by setting _name_ in the config and use any combination of _context_ and _id_ so you know how to send and receive packets for that endpoint.

For example, an endpoint belonging to a client would send packets to the server IP address, and a client endpoint on the server would know to send packets to the address of that specific client via _id_.

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
uint8_t packet[8];
memset( packet, 0, sizeof( packet ) );
reliable_endpoint_send_packet( endpoint, packet, sizeof( packet ) );
```

And the process packet callback will be called for each packet received by the endpoint.

You can get acks for packets sent through an endpoint like this:

```c
int num_acks;
uint16_t * acks = reliable_endpoint_get_acks( endpoint, &num_acks );
for ( int i = 0; i < num_acks; i++ )
{
    printf( "acked packet %d\n", acks[j] );
}
```

When you are finished with an endpoint, destroy it:

```c
reliable_endpoint_destroy( endpoint );
```

# Author

The author of this library is Glenn Fiedler.

Open source libraries by the same author include: [netcode](https://github.com/mas-bandwidth/netcode), [serialize](https://github.com/mas-bandwidth/serialize),  and [yojimbo](https://github.com/mas-bandwidth/yojimbo)

If you find this software useful, [please consider sponsoring it](https://github.com/sponsors/mas-bandwidth).

# Source Code

This repository holds the implementation of reliable in C.

Other reliable implementations include:

* [reliable Rust implementation](https://github.com/jaynus/reliable.io)

# Contributors

These people are awesome:

* [Walter Pearce](https://github.com/jaynus) - Rust Implementation

# License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
