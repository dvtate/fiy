# Fed-IY -- Federate It Yourself
A platform that makes it easy to make and host federated apps.

## Building
### Dependencies
- boost
- nlohmann json
- sqlite3

### Submodules
If not cloned already, fill submodules
`$ git submodule update --init --recursive`

### Build
Standard cmake project
```console
$ mkdir build
$ cd build
$ cmake ..
$ make -j`nproc`
```

## Install
Run the installation script

```
chmod +x install.sh
./install.sh
```

Set up the configuration
```
sudo cp config.ini /opt/fediy/config.ini
```

Typical options included in the config file:
```
hostname=localhost.localdomain
data_dir=/opt/fediy
salt=your_random_salt_here_change_this
ssl=false
port=8848
concurrency=4
```

### Run Server
```
./build/protocol_server
```
