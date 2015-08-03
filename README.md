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
but focuses on the set of user, network and channel specific
variables available in the ZNC configuration file. The available
commands are ```LIST```, ```GET```, and ```SET```.

### Usage

In order to adjust user, network and channel specific settings,
open a query with ```**target```, where target is a user, network,
or channel name.

Examples:
- user settings: ```/msg **jpnurmi help```
- network settings: ```/msg **freenode help```
- channel settings: ```/msg **#znc help```

To access network settings of a different user (admins only),
or channel settings of a different network, the target can be
a combination of a user, network, and channel name separated
by a forward slash ('/') character.

Advanced examples:
- network settings of another user: ```/msg **user/network help```
- channel settings of another network: ```/msg **network/#chan help```
- channel settings of another network of another user: ```/msg **user/network/#chan help```

PS. The prefix for settings queries is configurable. By default,
a doubled ZNC status prefix is used.

Got questions? Contact jpnurmi@gmail.com, or *jpnurmi* @ *#znc* on Freenode.
