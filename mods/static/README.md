# HTTP File Server
Serve static files.

## Module.json settings
Add a `mod_settings` field containing an object that has the following properties.
### `mod_settings` . `root` (required)
Path that the static files are stored in.
- example: `"/var/www/public"`

### `mod_settings` . `index` (optional)
Array of file names to check for if a folder is requested.
- example: `["index.xhtml","index.html","index.htm"]`

### Tips
Also note that the `"access"` field can be used to restrict access to the mod.

## Problem: multi-path
Users will likely want to have multiple statically hosted paths (eg. /, /blog, /wiki, etc.)
- Where do we put the settings for this?
  - Probably module.json
- How do we know what path user wants since that's usually removed from the request path?
  - Uh...
- Solution A: Duplicate the mod
  - For each statically hosted path, have a separate mod.
  - Each mod needs:
    - distinct module.json, and copy/symlink to module.so and icons 
    - distinct app id (does it?)
    - distinct path
  - Downsides:
    - More admin configuration and not very intuitive

- Solution B: ??? Special module.json setting?
  - Add `paths` field with type string[]
  - Router will forward requests with that path to the mod
  - Problems:
    - The path would have to be included in the uri in order for app to be able to use it
    - This is different logic to how paths and app IDs work now
- For now going with Solution A

## module.json Settings
In the module.json file, the following fields are relevant:
