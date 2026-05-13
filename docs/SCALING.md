# Scaling
This document outlines some options for scaling a FIY instance that's expected to take on many users.

## Vertical Scaling
Vertical scaling FIY is very straightforward:
- The main server should automatically leverage improvements to network and RAM.
- If you add more CPU cores, increase 'concurrency' in the server config.ini file.
- If you want to leverage additional disks you can add them to the storage mod and/or use symlinks.

The simplicity of having a single service running on a single machine is good for both ease of use and performance.
For nearly all instances, vertical scaling should be plenty.

## Horizontal Scaling
Horizontal scaling lets you spread server load across multiple machines which could be required if you've already maxxed
out your server but also helps if you'd like to leverage a bunch of weaker machines. This section outlines some techniques
you can leverage to maximize horizontal scaling.

These techniques can also be applied for other benefits. For example, storing files on a separate server that's not 
directly connected to the internet is generally good for security. Unfortunately, communications between machines is not
free and thus horizontal scaling means increased overall resource consumption. Stability can go both ways. Sure, more
machines means more points of failure, but at the same time that could mean more improved resilience as FIY could still
continue to operate with limited functionality.

### Limitations of the Main Server
It is common to design backend services to be sharded in order to spread the load across many machines. 
Unfortunately, as of 2026.5, the main server is not designed to be sharded and thus is limited to running on a single machine.

This is largely a limitation of it using SQLite as a database engine which gives improved performance on a single machine
doesn't facilitate sharing a single SQLite database across multiple machines. If this were to change, caching mechanisms
would also have to change. This is definitely something that should fixed in the future, in which case this guide would
need significant changes.

Fortunately, the main server more or less just proxies requests to mods which do most of the actual computations. Thus,
we can gain horizontal scaling by outsourcing things to mods and associated services.

### Network-Connected Mods: Outsourcing Compute, Storage, and RAM to Mods
Most mods default to the "shared_object" connector which shares resources between the host and the mod, effectively
incorporating it into the main server for optimal performance.

However, the "net" connector allows you to deploy the mod to another machine. The main server will send requests to the
mod over the network, the mod processes them on the remote machine and the main server sends the response back to the
user. This allows you to offload much of the compute, storage and memory requirements of that mod to another machine.

For example, the git mod has to do lots of heavy computations and filesystem operations. If you're expecting it to get
a lot of use, it probably would make sense to switch it to a net mod and dedicate a machine to handle it.

Depending on the mod, it could even be possible to shard it across multiple machines, giving even more horizontal
scaling. This does not solve the network bandwidth bottleneck, and if the mod servers aren't on the same network,
it could even make that problem worse. 

#### Pros
- Improved resilience:
  - Isolated failure: if a net mod crashes it won't take the main server (and other mods) down with it.
  - Isolated responsiveness: if a net mod is under heavy load, the main server (and other mods) are less impacted.
- Potentially improved security
- Reduced compute, storage, and RAM consumption on main server machine

#### Cons
- Increased overall resource consumption:
  - each machine has an OS, runtime, etc.
  - network communication isn't free
- Maintenance burden: more machines to keep updated
- Complexity and moving parts introduce points of failure and make things harder to debug
- Doesn't solve bandwidth bottleneck
    - Note: if bandwidth is an issue you may want to keep mod servers local and/or use a separate network interface

### Accessory Services : Outsourcing Bandwidth
Net mods can be used to eliminate compute, ram and storage bottlenecks, but what if you're hosting a FIY instance behind
30 mbps consumer internet connection? In that case, because all mod requests have to pass through the main server,
you can only handle 30 mbps worth of requests. This is a huge problem if you have mods that serve large files like
images or videos.

#### Built-in 'storage' mod
Getting around this requires designing mods to outsource bandwidth either by leveraging separate services or by calling
mods that do this. One built-in mod that can do this is the "storage" mod. This mod is primarily used by other mods to
store and serve the large files that use the most bandwidth. So it's important that mod developers know about it and
that server admins know how to configure it correctly.

When given a file to store, the storage mod chooses from a list of configured "stores" and responds with a URL that can
be used to access the file which gets included in user responses. These stores can URLs to instances of a "FIY CDN"
service, which should be hosted on a separate machine with its own internet connection. The mod can then send that URL
to users where they can download the file with no impact on the main server's bandwidth.

The 'storage' mod can also be configured to use local directories as stores but this would route requests through the
main server which isn't desirable in a bandwidth-constrained environment however if your CDN server has limited storage
you can configure the storage mod to put seldomly used files on the local filesystem. The 'storage' mod documentation
is a recommended read for both mod developers and high-traffic instance admins.

### Caching
If your mod serves static or seldomly changed files, you should definitely add the "Cache-Control" HTTP header to your
responses.

### Reverse Proxying
Because the main server doesn't support HTTP out of the box, you are expected to reverse-proxy requests to it with a
server like nginx or apache. There are a few techniques you can apply with these as well that could improve scaling.

#### Optimize HTTP Server Settings

- Enabling modern HTTP protocols can reduce bandwidth
  - nginx: `http2 on;`
- Enabling compression algorithms (eg gzip) can reduce response sizes. You could even leverage different compression
    algorithms for different mime_types
  - [gzip nginx](https://nginx.org/en/docs/http/ngx_http_gzip_module.html):
    - Example:
      ```
      gzip on;
      gzip_vary on;
      gzip_comp_level 6; # 1-9 bigger values = more compression but more compute
      gzip_min_length 256;
      gzip_proxied expired no-cache no-store private no_last_modified no_etag auth;
      gzip_types *;
      ```
- Don't set `Server` header. This is mostly a security thing but technically also bandwidth.
  - nginx: `server_tokens off;`

#### Reverse Proxy on Separate Machine
If compute/ram is the bottleneck, and you see nginx consuming a lot of resources, moving your nginx reverse proxy to a
separate machine could free up resources for the FIY server.

#### Caching CDN
Public caching CDNs like Cloudflare or Fastly aren't for everyone, but one major benefit is that they reduce bandwidth
by, among other things, caching responses. For mods with lots of static assets, this can dramatically reduce bandwidth
(ie - >50%) and, in some cases, improve latency. As of writing, Cloudflare has a generous free-tier, so if privacy and 
control aren't concerns, it's a great option.

If privacy and control are concerns, there are self-hostable alternatives or you can always make your own CDN (glhf).

### P2P Protocols
This mostly applies to mod developers, but WebRTC exists and results in users communicating directly instead of having
proxy everything through the main server. For server admins, mods that use WebRTC may require additional configuration.
