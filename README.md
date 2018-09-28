[![Travis Build Status](https://travis-ci.org/networkprotocol/reliable.io.svg?branch=master)](https://travis-ci.org/networkprotocol/reliable.io)

# Introduction

**reliable.io** is a reliability layer for UDP protocols where there is a continuous bidirectional flow of packets.

It has the following features: 

1. Identifies which packets were received by the other side (acks)
2. Packet fragmentation and re-assembly (send packets larger than MTU)
3. Provides estimates for round-trip time and packet loss

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the game industry.

Glenn wrote an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

Open source libraries by the same author include: [yojimbo](http://libyojimbo.com) and [netcode.io](http://netcode.io)

# Source Code

This repository holds the reference implementation of reliable.io in C.

Other reliable.io repositories include:

* [reliable.io Rust implementation](https://github.com/jaynus/reliable.io)

# Contributors

These people are awesome:

* [Walter Pearce](https://github.com/jaynus) - Rust Implementation

# Sponsors

**reliable.io** is generously sponsored by:

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
