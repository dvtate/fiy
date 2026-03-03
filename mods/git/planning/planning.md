# Planning

## Users
Options:
- commit trailer:
```
  git config trailer.fiyuser.key fiyuser
  git config trailer.fiyuser.ifexists addIfDifferent
  git config trailer.fiyuser.command "echo somevalue"
```
- git notes
  - https://tylercipriani.com/blog/2022/11/19/git-notes-gits-coolest-most-unloved-feature/
- mailmap

## Remote Repos
### Local Copies/Mirrors
Pros:
- We want to reduce traffic to servers hosting popular repos
- Proxying requests slows things down
- Nice to have local copies for archival reason

Cons:
- More disk space
- Need to keep in sync with upstream
- Implementation complexity

Decision:
For now we should just have a local read-replica, to enable cloning.
Non-read-only operations get proxied to the host instance.
