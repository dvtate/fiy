# API Documentation

## GET `/card/{user}`
Constructs a profile vCard for given user.

Start by requesting user's profile from their host server.

If request is from a local user, add contact info for that user on top.

### Results
vCard user profile

#### Status
- 200: success
- 404: no profile for user

## GET `/cards?users=csv,users,list`
Same as `/profile/{user}` but combines requests


## GET `/pfp/{local user}`
Sends the user's profile picture as a png

## GET `/json/{contact id}`
Get JSON for a contact

## GET `/vcard/{contact id}`
Get vCard for a contact

## POST `/save/{contact id}`
Edit/create a contact (incl profiles)
- The body is expected to be valid JSON