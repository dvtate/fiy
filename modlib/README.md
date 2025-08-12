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