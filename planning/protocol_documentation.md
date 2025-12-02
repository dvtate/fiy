# Protocol

## Terminology
- Instance: server running server for this protocol
- Peers: two instances that have established a secure channel for communication

## P2P Communication

### Handshake
Maybe instead we should just use TLS?

Two instances connect when an app makes a remote request to that server on behalf of a local user.

The process goes as follows:
- Instance A (the initiator) gets the public key from Instance B.
- Instance A uses instance B's public key to send an encrypted message containing the following:
  - Instance A's domain
    - Used so that instance B can validate signature
  - a secret that neither server will ever share but can be used to verify identity
    - Used for p2p comms: verifies identity
  - a bearer token that instance A will use to recognize instance B.
    - Used for p2p comms: authenticates
  - a digital signature of this message
    - Prevents DoS
- Instance B decrypts the message and verifies the digital signature using Instance A's public key
- Instance B responds to instance A's message with an encrypted response, that contains:
  - a bearer token that it will use to recognize instance A
  - a secret that should be appended to the secret that has already been provided by instance A
  - a digital signature of this response

### p2p app requests
After the handshake, p2p app requests include the following HTTP headers to mitigate mitm attack vectors:
- Fiy-Peer: bearer token the instance will recognize us by
- Fiy-User: local user (if any) that the request is for
- Fiy-Path: path to forward to the app
- Fiy-Now: current epoch millisecond
  - adds randomness and prevents reuse of messages
  - peers' clocks must be within 1 hour of eachother
- Authentication: contains a hash signature with the following components:
  - above headers
  - the relevant appid
  - the p2p secret from the handshake

## Protocol Server API
- *<>* /mods/<mod-id> -- other instance mod request
- *POST* /internal/mods/request/<mod-id> -- local net mod make request to other local mod
- *POST* /internal/mods/log -- write a log
- *GET* /internal/mods/user_info/<user> -- get some info on the user
- *GET* /internal/mods/login -- authenticate a user
