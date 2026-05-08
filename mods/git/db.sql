-- It might make sense to split this into multiple databases

-----------------------------
-- Users
-----------------------------

-- PROBLEM: what if someone creates a user with org name?
-- SOLUTION: orgs are just users
CREATE TABLE Organizations (
    userName TEXT PRIMARY KEY
);

-- Organization membership
CREATE TABLE OrgMembers (
    orgName TEXT REFERENCES Organizations,
    userName TEXT NOT NULL,
    role TEXT NOT NULL DEFAULT 'member', -- privileges
        -- member --
        -- admin --
        -- owner --
    UNIQUE(orgName, userName)
);

-- User settings (can also apply to orgs)
-- CREATE TABLE UserSettings (
--     userName TEXT NOT NULL,      -- relevant user's setting
--     setting TEXT NOT NULL,       -- example: default branch name
--     value TEXT NOT NULL,         -- example: master
--     UNIQUE(userName, setting)
-- );

-----------------------------
-- Repos
-----------------------------
-- TODO need to better accommodate renaming repos

-- Instance Local Repos
-- Note: remote repos are not stored in our database!
CREATE TABLE Repos (
    id INTEGER PRIMARY KEY,
    userName TEXT NOT NULL, -- local user owner
    repoName TEXT NOT NULL,
    description TEXT DEFAULT NULL,

    -- Who can see this repo?
    -- 0 = only me/people I share to
    -- 1 = users on my instance
    -- 2 = users on other instances
    -- 3 = public
    visibility INTEGER DEFAULT 0,

    createTs INTEGER NOT NULL,
    lastUpdateTs INTEGER NOT NULL,

    UNIQUE(userName, repoName)
);

CREATE TABLE RepoForks (
    fromRepoPath TEXT NOT NULL, -- could be on another instance
    toRepoPath TEXT NOT NULL UNIQUE, -- a fork can only have one parent
    user TEXT NOT NULL, -- user that created the fork
    createTs INTEGER NOT NULL -- When was the fork created
);

-- Note: this could be a remote repo!
CREATE TABLE RepoAccess (
    repoPath TEXT NOT NULL,     -- could be on another instance
    userName TEXT NOT NULL,     -- Users.name (can be on another domain)
    level INTEGER DEFAULT 1,
        -- 0 access revoked (ie - blocked)
        -- 1 for read-only
        -- 2 read+write
        -- 3 admin

    PRIMARY KEY(repoPath, userName)
);

-----------------------------
-- Tickets
-- This system encapsulates GH issues + PRs
-----------------------------

--
CREATE TABLE RepoTickets (
    repoId INTEGER REFERENCES Repos,
    ticketId INTEGER NOT NULL,
    userName TEXT NOT NULL,
    title TEXT NOT NULL,
    description NOT NULL DEFAULT '',
    createTs INTEGER NOT NULL,
    status INT DEFAULT 0, -- enum values open, closed, merged, rejected, etc.

    PRIMARY KEY(repoId, ticketId)
);
-- CREATE TABLE RepoTicketEdits ( ts, oldDescription ); -- title edits also get added to comments

-- a PR is a specific type of ticket
CREATE TABLE RepoPullRequest (
    repoId INTEGER REFERENCES Repos,
    ticketId INTEGER NOT NULL,
    fromRepoPath TEXT NOT NULL, -- could be on a different instance
    fromBranch TEXT NOT NULL,
    toBranch TEXT NOT NULL,

    PRIMARY KEY(repoId, ticketId)
);

CREATE TABLE RepoTicketComments (
    id INTEGER PRIMARY KEY,
    repoId INTEGER REFERENCES Repos,
    ticketId INTEGER NOT NULL,
    createTs INTEGER NOT NULL
);
-- CREATE TABLE RepoTicketCommentEdits

-- email -> fiy user map
CREATE TABLE UserEmails (
    email TEXT UNIQUE NOT NULL,
    user TEXT
);

CREATE TABLE RepoLikes (
    user TEXT, -- user that created the like
    repoPath TEXT,
    ts INTEGER NOT NULL,
    UNIQUE(user, user, repoPath)
);