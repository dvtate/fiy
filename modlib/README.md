# FIY Mod Development
FIY mods are federated apps that run on top of FIY.

## Module.json
This section describes the module.json file which contains relevant information about the mod.

For example: this would work for the demo mod
```json
{
    "id": "demo",
    "version": "0.0",
    "name": "C++ Demo Mod",
    "description": "A simple FIY mod written in C++ for demonstration purposes",
    "icon": "icon.png",
    "enabled": false,
    "connector": "shared_object",
    "connector_uri": "/opt/fiy/mods/demo/module.so",
    "path": "demo",
    "access": "global"
}
```

### `id`
A globally unique name given to the mod to facilitate federation. Ideally this should be in reverse domain name notation in order to eliminate namespace collisions. Some built-in and example mods may not follow this notation. 

Examples:
- net.dvtt.chat
- com.example.app

### `version`
Version of the module. Currently, this only matters to inform admins that updates may be available.

In the future the first number would indicate breaking changes and the second number indicates patches, minor feature additions, etc.

The mod can also provide this value.

### `name`
User understandable name for the mod.

### `description`
User understandable description for the mod.

### `icon`
Path to an icon that represents the mod. The path should be served by the mod.

ie - /icon.png should be accessible as <instance>/<mod>/icon.png

### `enabled`
Should this mod be enabled or not?

### `connector`
How does the FIY protocol server communicate with this mod?
- For now, the options are `"shared_object"`, `"http"` or `"https"`

### `connector_uri`
Where does the FIY protocol server communicate with this mod?
- `/opt/fiy/mods/demo/module.so` -- shared_object
- `cloud.example.com:3000` -- http/https

### `path`
Default path for this mod. For example, "chat" would make the mod accessible via
- chat.example.com and example.com/chat

### `access`
Restrict access to this mod.
#### Options
- `"public"` (default): Grants access to anyone, even anonymous users
- `"federated"` : Grants access to any authenticated user, even those on other instances
- `"local"` : Grants access to users of this instance
- `"user1,user2,user3"`: grants access to a specific list of users
  - note: if username is public/federated/local, add a trailing comma `"public,"`

### `data_dir`
Place for the mod to store relevant data.


## TODO
- struct fiy_user_t ?
- hostinfo.request callback should maybe not take a pointer so that user doesn't worry about checking for null
- network based mod type
- network shared_object network wrapper
  - allows user to leverage multiple machines for their instance
