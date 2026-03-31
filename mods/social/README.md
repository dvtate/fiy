# Social-IY
Like Twitter but with minimal features and designed to be used as a chat app.

- Posts are threads, with infinite replies allowed, thus they also act like chatrooms
- Users can pin threads to their homepage
  - this gives an alternative to the default scrolling view.
- Posts can be either public or private
- 

## Private Thread
- Content should probably be protected by the frontend (ie - e2e)
- Mentioning a user at the start of the post adds them to the conversation
  - they can see the message and replies
  - this allows other members of the private 
  - and if the thread continues in the replies they can also see future messages


## IDEA: Alternative implementation - threads as chats
- Threads: Post locations
- Global thread: default
- Users can create new private threads: private chats
- reply to post in thread:
  - either continue current thread
  - start reply thread
- Users can start new threads in the reply to a thread
- Users can add/remove users in the thread replies
- Threads are stored in database in a way that's amenable to searching
- Users can pin threads
- Flaws:
  - replying to posts in threads:
    - should it create a new thread or show up in the current thread?
    - maybe: forum style:
      - if no reply: posted after
      - otherwise: new post
    - maybe: 4chan style:
      - every reply has a (rely to) link
