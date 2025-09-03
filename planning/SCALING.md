# Scaling
## Vertical Scaling
Should almost always be enough

## Horizontal Scaling
For instances with lots of users and/or massive bandwidth requirements, it's common to run multiple shards across various machines.

### Challenges
#### Mods
DLL mods can be replaced with server mods, perhaps even via a dedicated program proxy server.

This would allow multiple protocol server shards to use the same mod and even allow mods to be sharded as well.

Mods would have to be designed with this in mind.

#### SQLite
- SQLite databases need to be on the same filesystem so this would need to be replaced
- Probably would make sense to use a sql sever or a microservice 
