# Planning

## Big Features/Selling points
- Data ownership
- Collaboration (ie - git)
- Single Sign On (SSO)
- Sever app management

## Components
Eventually these will all get moved to separate repos
- Protocol Server:
    - User/admin portal: account creation and management, app list, server management?, etc.
    - Module/app management:
    - Peer/Authenticaiton:
- Shop:
    - How users are able to find and pay for mods
    - Would be cool if they had a
- Apps:
    - fediymod.h: compile a .so file for your mod using interface provided
    -
    - Simple chat app/social network/blog?
    -
- Landing page
- Supporting sites:
    - Git adopt
    - Modnet to handle platform abuse

MVP:
- Protocol
    - [x] local user authentication
    - [ ] peer authorization
        - [ ] p2p auth
        - [ ] p2p user auth
    - [x] module loading
        - [x] .so
        - [ ] http (needs security)
        - [ ] unix sockets
- Git module
    - [ ] hosted repos
    - [ ] limited/no gui

# Data Directory
```
    data_dir/
        db.db3      - sqlite database
        pubkey      -
        config.ini  - global admin settings

        apps/
            example.app.id/
                config.json
                index.so
```



# Security
- specific user for app but run modules as a different user
