# Repos
These classes store, varying amounts of data about repositories.

## BasicRepo
This class only stores what we can determine from the path of a repository.

# planning
- basic repo: just used to parse the name of the repo and get basic info
  - can be converted to other repos as needed
  - also other repo types should be cached

- local repos
  - Db and Git components
- remote repos
  - Api and Git components

## Classes

- Basic Repo
  - Useful for basic operations
- RemoteRepo
  - For now should probably just proxy requests to remote host
  - Eventually:
    - Git repo cloned locally when requests made but evicted after set period of time
    - Until repo cloned or if data doesn't come from git repo, requests are forwarded to remote host
- LocalRepo 
  - Requests handled using sqlite and libgit2

- BasicRepo             -- just owner+name
- GitRepo               -- Git helper
- DbRepo: BasicRepo     -- Handles DB
- LocalRepo