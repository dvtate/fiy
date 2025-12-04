# TODO

## Shared library mod to server mod wrapper
Currently developing all mods as .so's as this would give best performance on low-traffic systems.

## Handshake protocol still not done
- should encrypt body

## Logging system
- Log files
- Module log files
- Maybe write logs in a different format?

## HTTP Keep-Alive
It would probably be a good idea to recycle Sessions.
I believe that the Session class design should enable this.
Does nginx do this?

## Minify HTML+CSS+JS
Maybe good to [minify](https://www.npmjs.com/package/minify) these.

## More than just SQLite
SQLite prevents horizontal scaling.

## Much later
- How to handle websockets/webrtc/ssh?
  - I guess mods have to start their own servers (not ideal)
  - Or advanced configuration from admins
- i18n: Boost.Locale ? GNU gettext
- Option to disable federation? Mod specific?
- Interact with other mods on the same system?
  - ie - query contacts app for user suggestions
