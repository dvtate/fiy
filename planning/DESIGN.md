# Design

## Components
Eventually these will all get moved to separate repos
- Protocol Server:
    - User/admin portal: account creation and management, app list, server management?, etc.
    - Module/app management: done by editing config files 
    - Peer/Authenticaiton: 
- Shop:
    - How users are able to find and pay for mods
    - Would be cool if they had a
- Apps (mods):
    - fiymod.h: compile a .so file for your mod using interface provided

- Landing page/website

# Data Directory
```
    data_dir/
        auth/           -- keys for peer 2 peer communication
        config.ini      -- global admin settings
        db.db3          -- sqlite database for protocol server
        libfiymod.so    -- module library
        mods/           -- installed apps (mods)
            example.app.id/     -- mod folder
                module.json     -- mod configuration     
                module.so       -- mod shared object (assuming shared object mod)
        pages/          -- user portal pages 
        server          -- protocol server
        apps/
```

# Security
- specific user for app but run modules as a different user
