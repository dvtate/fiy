# API Documentation

## **GET** `/`

## **GET** `/all`
Responds with a multi-vcard containing all the local user's contacts.
- 401: if requesting user is not a local user

## **GET** `/id/{contact id}`
Returns vcard for given contact id

## **POST** `/id/{contact id}`
Update vcard contact

## **GET** `/profile/{user}`
Constructs a profile vCard for given user.

Start by requesting user's profile from their host server.

If request is from a local user, add contact info for that user on top.

### Results
vCard user profile
- 200: success
- 404: no profile for user

## **GET** `/profile_json/{user}`
Similar to `/profile/{user}` but gives limited JSON instead of vCard.
- **TODO**
### Results
Only public profile properties will be included.
```json
{
  "user": "{user}",
  "displayName": "Dustin Van Tate Testa",
  "fn": "Dustin Van Tate Testa",
  "nickname": "ridder",
  "websites": ["https://dvtt.net"],
  "socials": ["https://youtube.com/@dvtt"],
  "email": "email@example.com",
  "bio": "Content of NOTE vCard field"
}
```

## **GET** `/profile?users=csv,users,list`
Same as `/profile/{user}` but combines requests

## **GET** `/pfp/{local user}`
Sends the user's profile picture as a png

## **POST** `/save/{contact id}`
Edit/create a contact (incl profiles)
- The body is expected to be a vcard

## **GET** `/search?q={query}&loc={locality}`
Get a list of contacts matching search query.
- **TODO**
- user must be logged in
- loc: locality: default=2
    *  0 - invalid
    *  1 - contacts for fediy users on this instance
    *  2 - contacts for fediy users on this or other instances
    *  3 - contacts for anyone even if not federated
### result
If loc is 1 or 2 -- list of users
```json
['user@example.com', 'localuser']
```
If loc=3 -- list of contact IDs
```json
[1,2,3]
```