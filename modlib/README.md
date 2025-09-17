# Fediy Mod Development
Fediy mods are federated apps that run on top of Fediy.

## Module.json
This file contains relevant information about the mod. For example: this would work for the demo mod

```json
{
    "id": "demo",
    "version": "0.0",
    "name": "C++ Demo Mod",
    "description": "A simple Fediy mod written in C++ for demonstration purposes",
    "icon": "icon.png",
    "enabled": false,
    "connector": "shared_object",
    "connector_uri": "/opt/fediy/mods/demo/module.so",
    "path": "demo"
}
```

## TODO
- struct fiy_user_t ?
- hostinfo.request callback should maybe not take a pointer so that user doesn't worry about checking for null
- network based mod type
- network shared_object network wrapper
  - allows user to leverage multiple machines for their instance
