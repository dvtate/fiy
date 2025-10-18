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

## Minify HTML+CSS+JS
Maybe good to [minify](https://www.npmjs.com/package/minify) these.

## No DB for protocol server?
Replace DB with RAM cache + files.
Protocol relevant info for users should be <1kb/userm.
So less than 1GB of ram for 1M users is fine. 
```
// approx 1kb/user
// so 100mb cache for 500k users is no big deal
struct LocalUser {
  char username[31];
  bool admin : 1;
  uint8_t password[128];
  char contact_email[255]; // maybe could move somewhere else?
  char locale[12];        // not needed for MVP
  std::time join_ts;
}
```
  - make a vector of these structs
  - raw data could be mmap()'d to/from a file
  - additional data would be handled by apps
    - For example, their display name would be handled by the contacts app

## Much later
- How to handle websockets/webrtc?
  - I guess mods have to start their own servers (bad)
- i18n: Boost.Locale ? GNU gettext
- Unfederated instance
- Interact with other mods on the same system?
  - ie - query contacts app for user suggestions
