An _experimental_ settings module for ZNC
=========================================

The settings module for ZNC aims to offer an intuitive and easy
to remember IRC interface to ZNC settings.

### Notes
- The module is experimental. It may or may not work as expected.
- The module requires the latest development version (1.7.x) of ZNC.

### Background

The module is inspired by, but not a direct replacement for the
built-in controlpanel module. The settings module does not offer
commands for adding or removing users and networks and so forth,
but focuses on the set of global, user, network and channel specific
variables available in the ZNC configuration file. The available
commands are ```LIST```, ```GET```, ```SET``` and ```RESET```.

### Usage

To access settings of the current user or network, open a query
with ```**user``` or ```**network```, respectively.

- user settings: ```/msg **user help```
- network settings: ```/msg **network help```

To access settings of a different user (admins only) or a specific
network, open a query with ```**target```, where target is the name
of the user or network. The same applies to channel specific
settings.

- user settings: ```/msg **somebody help```
- network settings: ```/msg **freenode help```
- channel settings: ```/msg **#znc help```

It is also possible to access the network settings of a different
user (admins only), or the channel settings of a different network.
Combine a user, network and channel name separated by a forward
slash ('/') character.

Advanced examples:
- network settings of another user: ```/msg **somebody/freenode help```
- channel settings of another network: ```/msg **freenode/#znc help```
- channel settings of another network of another user: ```/msg **somebody/freenode/#znc help```

PS. The prefix for settings queries is configurable. By default,
a doubled ZNC status prefix is used.

Got questions? Contact jpnurmi@gmail.com, or *jpnurmi* @ *#znc* on Freenode.
