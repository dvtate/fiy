# Contacts app
Manage contacts for federated users. 

## Implementation details
- All cards are converted into vcard 4.0 format

## Features Planning
### DONE
- [x] Add contact
- [x] Edit contact
- [x] Edit personal contact
- [x] Delete contact
- [x] List contacts
- [x] Search contacts
- [x] Web interface + API
- [x] User public profiles
- [x] Let them determine what info to share publicly
- [x] Download vCard file for contact
- [x] /pfp/* endpoint
- [x] /profile/* endpoint
- [x] Upload contact(s)

### TODO
- [ ] UI Improvements
  - Distinction between normal contacts and profiles
  - photos and names should be at the top
  - less useful properties go at the end (eg. update ts, contact id, etc.)
- [ ] CardDAV: https://datatracker.ietf.org/doc/rfc6352/
- [ ] Share contacts and profiles with other users
- [ ] View (but not edit) contacts of other users
- [ ] Accept/merge received contacts
- [ ] i18n
- [ ] optimizations and caching? this mod is gonna get used a lot
  - might even make sense to use something other than vcard internally