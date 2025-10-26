
The setup, build and testing was done on Baadal VM Ubuntu 22.04.5 LTS, using instructions provided on Piazza(https://docs.google.com/document/d/1IsxoB60id2kLFSC7nqzzn4osT8CQEEkz0WIMJzRLYMg/edit?tab=t.0)

## Activate venv for access to Ryu and Mininet packages

```
$  source ~/py_3.9.6/bin/activate
```

## Run Hub controller

```
$ ryu-manager --observe-links --verbose p1_hub.py
```

## Run topology file

```
$ sudo python3.10 p1_topo.py

```

## Run Learning Switch controller

```
$ ryu-manager --observe-links --verbose p1_learning.py
```

## Run topology file

```
$ sudo python3.10 p1_topo.py

```


*Note* : Run a new Controller with every seaprate topology file

## Run SPR controller with topology files in different window
```
$ sudo mn -c
$ ryu-manager --observe-links --verbose p3_spr.py
$ sudo python3.10 test_topologies/circular_topology.py

$ sudo mn -c
$ ryu-manager --observe-links --verbose p3_spr.py
$ sudo python3.10 test_topologies/tree_topology.py

$ sudo mn -c
$ ryu-manager --observe-links --verbose p3_spr.py
$ sudo python3.10 test_topologies/mesh_topology.py

```


## Run CA SPR controller with topology files in different window
```
$ sudo mn -c
$ ryu-manager --observe-links --verbose p4_ca_spr.py
$ sudo python3.10 test_topologies/circular_topology.py

$ sudo mn -c
$ ryu-manager --observe-links --verbose p4_ca_spr.py
$ sudo python3.10 test_topologies/tree_topology.py

$ sudo mn -c
$ ryu-manager --observe-links --verbose p4_ca_spr.py
$ sudo python3.10 test_topologies/mesh_topology.py

```

*Note* For each topology run, a topology.txt is generated(and overwritten) with info about Hosts, Switches and Links


