# API Documentation

## *GET* `/`

## *GET* `/all`
Responds with a multi-vcard containing all the local user's contacts.
- 401: if requesting user is not a local user

## *GET* `/id/{contact id}`
Returns vcard for given contact id

## *GET* `/profile/{user}`
Constructs a profile vCard for given user.

Start by requesting user's profile from their host server.

If request is from a local user, add contact info for that user on top.

### Results
vCard user profile
- 200: success
- 404: no profile for user

## *GET* `/profile?users=csv,users,list`
Same as `/profile/{user}` but combines requests

## *GET* `/pfp/{local user}`
Sends the user's profile picture as a png

## *POST* `/save/{contact id}`
Edit/create a contact (incl profiles)
- The body is expected to be a vcard