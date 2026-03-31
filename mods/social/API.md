# API Documentation
This api is available to both the frontend and peers.

- **All endpoints start with `/api/v1`.**
- Unless otherwise specified, results are in JSON.

## **GET** `/post/[domain/]<id>`
Returns JSON representation of a post
- if post is on another `domain` request will be proxied to that domain
- Codes:
  - 401:
    - If remote request and `domain` is not local
    - If private and requesting user is not party
  - 404:
    - Post doesn't exist/not found

## **GET** `/posts`
Search posts
- Uses:
  - Profile posts
  - Post Replies
  - 
- query params
  - `user`
  - `before_ts`
  - `after_ts`
  - `before_id`
  - `after_id`
  - `reply_to_post`
  - `sort_likes`
  - `sort_ts`

## **GET** `/likes`

## **POST** `/post`
Create a new post/reply

## **GET** `/thread/[domani/]<id>`
Load posts that this post is a reply to.
- query params
  - `n`: max number of posts to load

## **POST** `/like/[domain/]<id>`
Like a post
- Note: request can come from a peer user
