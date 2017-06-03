[![Travis Build Status](https://travis-ci.org/networkprotocol/reliable.io.svg?branch=master)](https://travis-ci.org/networkprotocol/reliable.io)

# reliable.io

**reliable.io** is a simple reliability layer for UDP-based protocols.

It's designed for protocols that have a bidirectional flow of packets, for example real-time games like first person shooters.

It has the following features:

1. Identifies which packets were received by the other side (acks)

2. Packet fragmentation and re-assembly so you can send packets larger than MTU

# Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the game industry.

Glenn is currently writing an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

You can support Glenn's work writing articles and open source code via [Patreon](http://www.patreon.com/gafferongames).

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
 
And by individual supporters on [Patreon](http://www.patreon.com/gafferongames). Thank you. You make this possible!

# License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
