[![Build status](https://github.com/networkprotocol/reliable/workflows/CI/badge.svg)](https://github.com/networkprotocol/reliable/actions?query=workflow%3ACI)

# Introduction

**reliable** is a simple packet acknowledgement system for UDP-based protocols.

It has the following features:

1. Acknowledgement when packets are received
2. Packet fragmentation and reassembly
3. RTT and packet loss estimates

reliable is stable and production ready.

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler).

Open source libraries by the same author include: [netcode](http://netcode.io) and [yojimbo](http://libyojimbo.com)

Glenn is now the founder and CEO of Network Next. Network Next is a new internet where networks compete on performance and price to carry your traffic. Check it out at https://networknext.com

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
    * [The Network Protocol Company](http://www.thenetworkprotocolcompany.com)
    
* **Bronze Sponsors**
    * [Kite & Lightning](http://kiteandlightning.la/)
    * [Data Realms](http://datarealms.com)
 
And by individual supporters on Patreon. Thank you. You made this possible!

# License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
