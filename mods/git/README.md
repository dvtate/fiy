# GitIY - Git It Yourself
Federated Git Hosting and Collaboration.

## Features Roadmap
With this roadmap, and the pages below, it would probably take about a man-year to accomplish everything.

### Stage 0
- Create repos
- Clone + push to repo via terminal
- This should be easy to accomplish via git-http-backend CGI script
  - Arch: /usr/lib/git-core/git-http-backend

### Stage 1
- Repo summary
- Repo settings
- View user profile
  - list repos
- Show files in browser

### Stage 2
- Private repos, manage access to them
- Forks
- Pull requests
- Issues
- Repo likes / bookmarks
- Commit on website

### Stage 3
- Organizations
- CI/CD
- Drag and drop files for commit
- Git+SSH

## Database
For now, it's a single database for simplicity, but in the future it might make sense to split things out.
For example, the organizations tables don't need to be connected to the repos tables. 

## Pages
- **GET** `/` (auth): activity overview page
- **GET** `/` (guest): About Git-IY

- **GET** `/repo/create` (auth): create a new repo
- **POST** `/repo/create` (auth):

- `/api` (APIs for federated instances):  
  - **GET** `/api/<user>`: basic user info
  - **GET** `/api/<user>/repo/<repo>`: basic repo info
  - ...

- **GET** /settings

- **GET** `/<user>` or `/@<user>`: user overview page
  - `@` prefix in case username is 'api', 'settings', 'repo', etc. 
  - ?tab=<tab> :
    - connections: user followers+following
    - activity: user activity overview
    - ...
- **GET** `/<user>/<repo>`: repo page
- **GET** `/<user>/<repo>.git`: forward to the 

- Organization page
  - Organization members
  - Settings

- Repo page
  - Create new repo
  - Collaborate tab:
    - View tickets for issues, pull requests, etc.
    - Make new ticket
  - Project browser
  - Code viewer/editor
  - Make Commit
  - Settings

- User page
  - Connections (followers, following)
  - Contributions
  - Likes/bookmarks
  - Overview
  - Repos
  - Settings

## Reference material
- https://git-scm.com/docs/http-protocol