# Internal File Storage
Store and retrieve files for apps.

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