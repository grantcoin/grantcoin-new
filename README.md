Codecoin Catbox
================================

http://kittyco.in

Copyright (c) 2009-2013 Bitcoin Developers
Copyright (c) 2011-2013 Litecoin Developers
Copyright (c) 2013-2014 Catcoin Developers
Copyright (c) 2015 Grantcoin Developers
Copyright (c) 2015 Troy Benjegerdes <hozer@hozed.org>

### What is Codecoin

Codecoin is a project to build multiple C++ 'Catoshi' clients from the
same source tree. Default build is Grantcoin. On linux, do this:

    cd src
    make                         # builds grantcoin
    make COIN_BRAND=solarcoin    # builds solarcoin
    make COIN_BRAND=uro          # builds uro

### What is Grantcoin?
[Grantcoin](http://www.grantcoin.org/) (abbreviated GRT) is the first [cryptocurrency](https://en.wikipedia.org/wiki/Cryptocurrency) distributed primarily as Proof-of-Participation grants according to a transparent business plan by a legally incorporated nonprofit organization. Grantcoin is based on [Peercoin](http://peercoin.net/) (PPCoin), the first digital currency using Proof-of-Stake consensus as a security model, which is more energy efficient than [Bitcoin](http://en.wikipedia.org/wiki/Bitcoin)'s Proof-of-Work mining system. Grantcoin will begin with several months of PoW until the currency is widely circulated, with further evolution determined by the foundation and community.

### What is kittycoin?

Kittycoin used to ba a Catcoin-compatible cryptographic currency with
scrypt N=1024 proof of work, with (mostly) the same blockchain parameters
as Bitcoin:
 - 50 coins per block
 - 10 minutes block targets
 - Subsidy halves in 210k blocks
 - 21 million total coins
 - 1 block difficulty adjustment, with a PID ajustment.

For more information, as well as an immediately useable, binary version of
the Catcoin client sofware, see http://www.catcoins.org

###Why the Catbox?

Because, frankly, right now this code stinks. It's a big pile of crap, and
I'm taking a bunch of other crapcoin and other stinky bits like p2pool and
counterparty and mixing it all up here.

But there's a point here. If your shit stinks, you're composting it wrong.
I grow commodities, and if you're a farmer, all the micronutrients and
fertilizer in a biomass-based catbox start looking like the most valuable 
thing in the world.

And this:

Seymour Cray was a friend of my dad's. I asked him what it was like to know
the genius who had built the world's first supercomputer company. My dad said,
‘Well, actually, son, he wasn't so much smarter than me. He just made mistakes
a hundred times faster.’

~~ Townes Duncan

What do we do with Catcoin better than any other coin? 
	make
			mistakes
					faster
And then land on our feet.

### License

Kittycoin is released under the terms of the Affero GNU General Public License
version 3 (AGPLv3). See COPYRING for more information, or the GNU website at
http://www.gnu.org/licenses/agpl-3.0.html

If you want to give me crap about the license, flame away, or pay me enough 
to buy a farm and I'll license it however the hell you want. Otherwise some
get-rich-quick copycrap startup will accidentally include my AGPLv3 code, and
then I will 0wn them. The revolution will not be televised, it will be quietly
settled out-of-court.

### Development process

The development process is Troy commits stuff when he feels like it. He's not
a computer execution unit following a process, he's a guy who happens to like
cats, and think cryptographic currencies are a better way to make money.

He also likes rough consensus, running code, and good tests. So if you have all
of those, the probability that he'll feel like integrating your change approaches
unity over a month or three.

If you have only rough consensus, or only running code, the others can also be
achieved with sufficient money for a publicly announced contract and request
for proposals. However be warned that it's dangerous to think you can buy a
person who writes code that makes money.

[a href=<the original bitcoin/litecoin/catcoin dev process boilerplate>]

Testing
-------

Testing is good. Do it. It makes aforementioned consensus easier.

### Building
on debian 8.x:

    sudo apt-get install mercurial build-essential libboost-dev libdb++-dev \
		libssl-dev libminiupnpc-dev libboost-filesystem-dev \
		libboost-system-dev libboost-program-options-dev \
		libboost-thread-dev libboost-test-dev libboost-test1.55-dev 
    cd src; make

### Automated Testing

Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code.

Unit tests for the core code are in `src/test/`. To compile and run them:

    cd src; make test

Unit tests for the GUI code are in `src/qt/test/`. To compile and run them:

    qmake BITCOIN_QT_TEST=1 -o Makefile.test bitcoin-qt.pro
    make -f Makefile.test
    ./catcoin-qt_test

