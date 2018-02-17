[![Travis Build Status](https://travis-ci.org/networkprotocol/reliable.io.svg?branch=master)](https://travis-ci.org/networkprotocol/reliable.io)

# Introduction

**reliable.io** is a simple reliability layer for UDP-based protocols.

It's designed for situations where there is a bidirectional flow of packets, for example, real-time action games like first person shooters.

It has the following features: 

1. Identifies which packets were received by the other side (acks)

2. Packet fragmentation and re-assembly (so you can send packets larger than MTU)

3. Estimates for round-trip time and packet loss

# Usage

**reliable.io** is a low-level library that is designed to be used with your existing network layer. 

It has no networking support, and no opinion on what topology you should use. Its only requirement is that you have one reliable.io endpoint on each side of a connection.

If you would like a secure client/server communications layer to use with reliable.io, I recommend [netcode.io](http://www.netcode.io)

If you want something higher level that implements reliable-ordered messages on top of reliable.io, try [yojimbo](http://libyojimbo.com).

There is also a port of reliable.io to JavaScript here: https://github.com/mreinstein/reliable.io.js

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the game industry.

Glenn wrote an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

Open source libraries by the same author include: [yojimbo](http://libyojimbo.com) and [netcode.io](http://netcode.io)

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
