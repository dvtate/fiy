-- basically Twitter but with half the features
-- and designed to be used more like a chat app

-- REMEMBER: Data ownership
-- Objects "owned" by local users are stored on the local server
-- data associated with the owned objects must also be stored
-- in order to enable functionality.
--
-- We could alternatively create local copies of everything but
-- for efficiency, etc. that's not ideal.
--
-- For example, in this case, users own their posts. So posts
-- are stored on their local server's database. 
-- remote users are notified of its existence and can interact
-- but there has to be a local copy of those interactions


-- This only stores local posts
-- Remote posts should be fetched from the relevant server
CREATE TABLE Posts (
    id INTEGER PRIMARY KEY,     -- locally they can be referenced by the id, otherwise domain+id
    user TEXT NOT NULL,
    createTs INTEGER NOT NULL,
    content TEXT NOT NULL,
    visibility INTEGER,          -- who can see this post
    -- 0    - public (default)
    -- 1    - followers only
    -- 2    - private thread (only those mentioned at the start)

    -- If the post is a reply
    replyToPostId INTEGER DEFAULT NULL,
    replyToPostDomain INTEGER DEFAULT NULL
);

CREATE TABLE PostMentions (
    postId INTEGER REFERENCES Posts,
    user TEXT NOT NULL,
    UNIQUE(postId, user)
);

CREATE TABLE Likes (
    postId INTEGER REFERENCES Posts, -- we only track likes on our own posts
    domain TEXT DEFAULT NULL,
    user TEXT NOT NULL,
    ts INTEGER NOT NULL
);

CREATE TABLE Following (
    followedUser TEXT NOT NULL,
    followerUser TEXT NOT NULL,
    followTs INTEGER NOT NULL
);