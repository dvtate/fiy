# Internal File Storage
File CDN
Store and retrieve files for apps.

This enables apps to leverage CDNs and provide temporary unauthenticated download links.

## Configuration
The module.json file can have the following settings
```json
{
  "...":"...",
  "mod_settings": {
    "stores": [
      {
        "path": "https://cdn.example.com",
        "options": "bulk",
        "authentication": "...bearer token..." 
      }
    ]
  }
}
```

### Stores
- The stores list should be sorted in order of priority
- The `path` field should be a string representing the path to that store
  - If the path starts with `/`, the files will be stored in the local FS (not recommended)
  - Otherwise, the path is assumed to be a URL to a server running [the FIY CDN server](cdn/)
  - Maybe one day we can add AWS S3 or equivalents.
- `options`: string containing comma separated flags and options
  - `bulk`: store should be used for large/seldom-accessed files
  - `ro`: do not add new files to this store
- The `authentication` field should contain the auth token (for [the FIY CDN server](cdn/) this is a bearer token)

## API
Endpoints used to store data give a URL.

### **POST** `/store`
Store a short-term, copy of the data.

#### Query Parameters
- `public`: include this parameter to make a public link
- `cache-duration`
- `type`: storage type
  - `cache`: temporary copies of files to reduce load/authentication steps
  - `bulk`: file is either large or seldom-accessed
- `auth`:
  - ``
- `src`: URL the data came from or where to download it from if there is no file body sent
- `lifetime`: how long until this asset should be deleted from local storage (in seconds)
  - after the asset is deleted, requests will be redirected to `src`
- 
- `X-Storage-Type`: one of the following options
  - `cache`
  - `bulk`, 
- `X-Content-Source`: URL to download the file from, if unspecified, the file contents should be in the request body

#### Use cases
- unathuenticated downloads
  - url should only last for short duration
- cache files from other servers

## Features roadmap
### Stage 0
- Post file for storage
- Retrieve file

### Stage 1
- Mime-type
- Add lifetime, after which point the file is automatically deleted
- File name

### Stage 2
- Admin portal: shows breakdown of files
- User portal: shows all files they have access to

## Ideas
- Optimized virtual storage mapping
  - Provide a list of paths + capacity + recommended use (ie - cache/bulk storage)
  - I feel like this is overkill 99% of the time