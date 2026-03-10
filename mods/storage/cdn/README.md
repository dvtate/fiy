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
