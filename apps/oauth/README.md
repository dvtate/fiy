# OAuth App
This app would be able to provide two services:

## Identity Verification
Apps make you log in to the oauth app and the app sends the user's username to the provided callback to verify the user's identity.

## Make requests on user's behalf
The oauth request could also have a list of requests to make on behalf of the user. When the user logs in and authorizes these request, they are made on behalf of the user+instance via the oauth app. And the results are sent back via the provided callback.


## API
- callback query parameter