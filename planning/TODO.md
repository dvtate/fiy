# TODO

## CLI
Convert protocol/main.cpp into a CLI with more functionality (or make a bash wrapper).

### Commands
These are just some ideas
- Run server
  - specify config
- Set up mods
- Check mods and configs
- Admin web interface
- Admin CLI

## Easy Changes
- fix include order 
- replace std::unordered_map with boost::unordered_flat_map where possible
- Replace SQLiteCpp
  - the library is badly designed

## Portal+Protocol Server Features
- Use local copies of CSS+JS Libraries
- [minify](https://www.npmjs.com/package/minify) html+css+js?
- encrypt peer handshake
- DLL Mods: shared request task queue + thread pool
  - if task queue gets big, warn user

## Shared library mod to server mod wrapper
Currently developing all mods as .so's as this would give best performance on low-traffic systems.
This wrapper would enable some degree of horizontal scaling without having to rewrite the mods. 

## Logging system
- Log files
- Module log files
- Maybe write logs in a different format?
- Maybe good for new users


## Much later
- How to handle websockets/webrtc/ssh?
  - Websockets should be easy
  - IDK anything about WebRTC but probably hard
  - SSH basically impossible
  - Mods could start their own servers, but admin would have to set up

- i18n:
  - backend: Boost.Locale ? GNU gettext ?
  - frontend: javascript ?
  - Problem: locale specific caches?
  
- Disable federation? Mod specific?
  - lots of settings
