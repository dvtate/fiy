# Reverse Proxy Mod
Proxies requests to a target server with headers added containing user info.

Useful for throwing together non-federated apps quickly if user-identification is the only necessary feature.

## Module.json settings
Add a `mod_settings` field containing an object that has the following properties.

### `mod_settings` . `target` (required)
Where to proxy requests to
Path that the static files are stored in.
- example: `"http://localhost:8080"`

### Tips
- The `id` field should be globally unique, the easiest way to ensure this is to use reverse domain naming convention
  starting with the instance's domain, thus making giving it a local id.
- Also note that the `"access"` field can be used to restrict access to the mod.