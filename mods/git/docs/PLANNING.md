# Planning & Design

## Challenges

### Frontend
- Issues and PRs are both "contributions"
- Maybe allow Issues to become PRs?

### fiy-user vs email
- Problem: Can't use emails like normal git because gotta be able to tie the emails back to host instance.
- Solution?: Contributors.txt - file that contains all the fediy - email mappings

## Filesystem
This section describes how files should be stored within the mod's data dir

```
- mod_dir/
  - ...
```

### Repos
Repos are stored 
```
  - local_repos/
    - <user>/
      - <repo>/
```

### Database
- it might make sense to have a separate database for cached contributions

## API
### Pages
HTML
#### *GET* `/<user>`
#### *GET* `/<user>/settings`
#### *POST* `/<user>/settings`
#### *GET* `/new`
#### *GET* `/<user>/<repo>`
#### *GET* `/<user>/<repo>/.git`

### API
JSON
#### *GET* `/api/users/<user>`
#### *GET* `/api/repos/<user>/<repo>`
