# Mod Net Wrapper (MNW)
Normally mods are loaded as shared modules directly by the protocol server.
This program allows shared library modules to be instead used as network modules.
It loads the module and then proxies network requests from the protocol server.
This can be useful for horizontal scaling, allowing compute/storage/etc. heavy mods
to be run on separate machines.

## Configuration
### MNW Config
- fediy server address
- listening port number

### FeDIY Config
- MNW address + portnumber
