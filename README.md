[![Travis Build Status](https://travis-ci.org/networkprotocol/reliable.io.svg?branch=master)](https://travis-ci.org/networkprotocol/reliable.io)

# Introduction

**reliable.io** is a packet acknowlegement system for UDP protocols.

It has the following features: 

1. Identifies which packets are received by the other side
2. Packet fragmentation and reassembly
3. RTT and packet loss estimates

reliable.io is stable and well tested having been used in AAA game projects for over 2 years now.

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler).

Glenn wrote an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

Open source libraries by the same author include: [netcode.io](http://netcode.io) and [yojimbo](http://libyojimbo.com)

# Source Code

This repository holds the implementation of reliable.io in C.

Other reliable.io repositories include:

* [reliable.io Rust implementation](https://github.com/jaynus/reliable.io)

# Contributors

These people are awesome:

* [Walter Pearce](https://github.com/jaynus) - Rust Implementation

# Sponsors

**reliable.io** was generously sponsored by:

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
