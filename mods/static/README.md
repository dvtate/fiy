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
- The `id` field should be globally unique, the easiest way to ensure this is to use reverse domain naming convention 
  starting with the instance's domain, thus making giving it a local id.
- Also note that the `"access"` field can be used to restrict access to the mod.

## Problem: multi-path
Users will likely want to have multiple statically hosted paths (eg. /, /blog, /wiki, etc.)
- Solution: Duplicate the mod
  - For each statically hosted path, have a separate mod.
  - Each mod needs:
    - distinct module.json, and copy/symlink to module.so and icons 
    - distinct app id (does it?)
    - distinct path
  - Downsides:
    - More admin configuration and not very intuitive
