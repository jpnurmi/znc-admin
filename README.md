An _experimental_ admin module for ZNC
======================================

The admin module for ZNC aims to offer an intuitive and easy
to remember IRC interface to ZNC administration.

### Notes
- The module is experimental. It may or may not work as expected.
- The module currently requires the admin branch of [jpnurmi/znc]
  (https://github.com/jpnurmi/znc/commits/admin).

### Usage

To access global settings (admins only), open a query with ```*admin```.

- global settings: ```/msg *admin help```

To access settings of the current user or network, open a query with
```**user``` or ```**network```, respectively.

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

PS. The prefix for user, network, and channel queries is configurable.
By default, a doubled ZNC status prefix is used.

Got questions? Contact jpnurmi@gmail.com, or *jpnurmi* @ *#znc* on Freenode.
