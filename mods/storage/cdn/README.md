# FIY Storage CDN Server
This server is designed to be hosted on a separate server from the main FIY instance, thus outsourcing much of the
bandwidth requirements.

## Configuration
- `Max storage`

## Design
Every asset is assigned a unique ID.
Access to assets is controlled via access tokens.
These often correspond to users.
When an asset has no associated access tokens, it is deleted.

## API
- `/api`:
  - all endpoints must be authenticated with bearer token
  - **POST** `/api/asset`
    - add a file to the CDN
    - option: `url`: url to get the data from
    - option: `eol`: when to delete the asset (or -1 / "" if permanent)
  - Create token:
    - **POST** `/api/token/:asset_id`: create a new token for a particular asset id
    - **POST** `/api/token/:asset_id/:token`: grant existing token access to asset
    - option: `expire`: number of seconds until this token is invalidated (or -1 for permanent access token)
  - Delete a token
  - 
- `/:auth_token/:asset_id`
  - Get requested file


## New API ?
- `/api`:
  - all endpoints must be authenticated with bearer token
  - **POST** `/api/token`
    - create a new access token
  - **DELETE** `/api/token`
    - delete an access token (ie - revoke access)
  - **POST** `/api/asset`
    - create a new asset that will be deleted in 10 mins if no access token is assigned
  - **POST** `/api/token/:asset_id`
    - create a new token with access to asset
  - **POST** `/api/token/:asset_id/:token`
    - grant existing token access to asset

## New DB ?
```sqlite
CREATE TABLE Assets (
    
);
CREATE TABLE Tokens (
    
);
```