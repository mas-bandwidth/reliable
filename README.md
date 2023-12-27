[![Build status](https://github.com/mas-bandwidth/reliable/workflows/CI/badge.svg)](https://github.com/mas-bandwidth/reliable/actions?query=workflow%3ACI)

# Introduction

**reliable** is a simple packet acknowledgement system for UDP-based protocols.

It has the following features:

1. Acknowledgement when packets are received
2. Packet fragmentation and reassembly
3. RTT and packet loss estimates

reliable is stable and production ready.

# Usage

Reliable is designed to operate with your own network library and your own sockets.

To use reliable, create an endpoint:

```c
double time = 0.0;                                      // time in seconds

struct reliable_config_t config;
reliable_default_config( &config );
config.max_packet_size = 32 * 1024;                     // maximum packet size that may be sent in bytes
config.fragment_above = 1200;                           // fragment and reassemble packets above this size in bytes
config.max_fragments = 40;                              // maximum number of fragments a packet may be broken up into
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

# Author

The author of this library is Glenn Fiedler.

Open source libraries by the same author include: [netcode](https://github.com/mas-bandwidth/netcode) and [yojimbo](https://github.com/mas-bandwidth/yojimbo)

# Source Code

This repository holds the reference implementation of reliable in C.

Other reliable implementations include:

* [reliable Rust implementation](https://github.com/jaynus/reliable.io)

# Contributors

These people are awesome:

* [Walter Pearce](https://github.com/jaynus) - Rust Implementation

# Sponsors

**reliable** was generously sponsored by:

* **Gold Sponsors**
    * [Remedy Entertainment](http://www.remedygames.com/)
    * [Cloud Imperium Games](https://cloudimperiumgames.com)
    
* **Silver Sponsors**
    * [Moon Studios](http://www.oriblindforest.com/#!moon-3/)
    * [Mas Bandwidth](https://www.mas-bandwidth.com)
    * The Network Protocol Company
    
* **Bronze Sponsors**
    * Kite & Lightning
    * [Data Realms](http://datarealms.com)
 
And by individual supporters on Patreon. Thank you. You made this possible!

# License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
