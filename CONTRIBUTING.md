# New Contributors Guide
## Project Guide
This project is organized into 3 categories of components broken down by architecture
- Protocol Server `/protocol_server/`
  - Runs the web server that handles all user requests and communication between instances
  - Loads and routes requests to apps (aka mods)
  - Due to the many responsibilities of this service, performance and security are very important
- Mod SDK - `/modlib/`
  - Libraries and tools used to build and run mods.
  - Breaking changes are a big deal here and should be avoided when possible.
- Built-in Mods - `/mods/`
  - There are several built-in mods that other mod developers rely on, notably:
    - contacts:
      - users shouldn't have to create new profiles and contacts for each app
      - mods can make requests to the contacts app to get user profile pictures, contact info, etc.
    - notifications (todo):
      - write notifications that the user can see when they log in (or if they have an app)
    - http reverse proxy (todo):
      - proxy requests, including the user info to a destination server
      - useful as a shortcut for developing apps

## What we want
- Bug/security fixes
- Optimizations
- Improved documentation, comments, etc.
- Features

## What we don't want
- Features that could

## Style
### JS/TS
You should try to fix all linter errors before merging code.
```json
{
  "root": true,
  "parser": "@typescript-eslint/parser",
  "plugins": [
    "@typescript-eslint"
  ],
  "extends": [
    "eslint:recommended",
    "plugin:@typescript-eslint/eslint-recommended",
    "plugin:@typescript-eslint/recommended"
  ],
  "rules": {
      "@typescript-eslint/no-explicit-any": "off",
      "@typescript-eslint/ban-ts-comment": "off"
  }
}
```

### C/C++
C++ style is harder to pin down but arguably more important.
Apologies in advanced if you PR gets delayed due to style opinions.
