# OAuth App
This app would provide two services:

- Phase 1: Identity Verification
  - External apps make you log in to the oauth app and the app sends the user's username to the provided callback to verify the user's identity.
- Phase 2: Authorization
  - Allows an app to make requests on behalf of the user. See phase 2 further down in this document.

## Phase 1
### **GET** `/identify?callback={callback uri}`
Sends a username+password login page.

Page has a form which sends username and password via post.

### **POST** `/identify?callback={callback uri}`
- Receives username+password in formbody.
- uses `host_info.local_login(username, password)` to authenticate user
- if successful, redirect them to the callback uri (HTTP status 303)
  - with added query parameters `?user={username}&token={token}`
  - where token is a random string that can be used with the /exchange endpoint

### **GET** `/exchange?token={token}`
- Gives the user name associated with the given token
- If invalid token gives 401 - authenticated

### Implementation ideas
#### Tokens
- You can store tokens in a unordered_map to make it fast to look them up
  - `std::unordered_map<std::string, std::string> g_tokens_lookup;`
  - unordered_map allows multiple readers but only one writer
  - For thread safety you can use `util/RWMutex.hpp`
- Alternatively could use an array and+remove entropy to the tokens based on the numbers
  - same issues with threading afaik tho

#### Reference
- the mailmod is a good example of a working mod that you can use as a reference

## Phase 2
We eventually we will also want to allow apps using oauth to make requests on behalf of the user.

This could be accomplished by adding a new endpoint
- **GET** `/authorize?callback={callback uri}&reason={reason message}&scope={relevant app ids}`
  - Where the reason message is a justification for why the app needs authorization to work on behalf of the user
  - This would work similar to the /identify endpoint but the token would have additional information associated with it
  - the `/exchange` endpoint would provide them an additional token use to authorize the following endpoint
  - The page shows:
    - "By logging in you are granting access to use the following apps on your behalf: {scope}"
    - "For the following reason: {reason}"

- **POST** `/api`
  - This endpoint allows a remote app to act on behalf of the user
  - Token from `/exchange` used in `Authorization` HTTP header
  - Custom headers:
    - Fiy-OA-App: id for relevant app to request to on behalf of the user
    - Fiy-OA-Path: path to forward to the relevant app request
    - Fiy-OA-Method: Method to make app request
    - Fiy-OA-Headers: Additional headers to include in request
  - Those headers and the body are used to call `host_info.request` on behalf of the relevant user.
  - The response is proxied back.
