# Drunkcan - a multiprotocol CAN device manager

Drunkcan is a CAN (Controller area network) manager that aims to read multiple
protocols and make the devices in the network into Unix socket resources.

## Installation
To install run the following commands
```
git clone https://github.com/TileHalo/drunkcan
cd drunkcan
make
make install # run as root
```

## Update
To update please pull the repository. Go into the directory and run the
commands

```
git pull
make
make install
```

## Usage
To use just run `drunkcan` in terminal. For documentation please either run
`drunkcan -h` or run `man drunkcan`.
