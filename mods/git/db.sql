-- CREATE DATABASE fiygit;
-- USE fiygit;

-- PROBLEM: what if someone creates a user with org name?
-- SOLUTION: orgs are just users
CREATE TABLE Organizations (
    userName TEXT PRIMARY KEY
);

CREATE TABLE OrgMembers (
    orgName TEXT REFERENCES Organizations,
    userName TEXT NOT NULL,
    level INTEGER DEFAULT 0, -- privilege bitfield
    UNIQUE(orgName, userName)
);

-- remote repos are not stored in our database?
CREATE TABLE Repos (
    id INTEGER PRIMARY KEY,
    userName TEXT NOT NULL,
    name TEXT NOT NULL,

    -- Who can see this field?
    -- 0 = only me/people I share to
    -- 1 = users on my instance
    -- 2 = users on other instances
    -- 3 = public
    visibility INTEGER DEFAULT 0,

    UNIQUE(id, name)
);

CREATE TABLE RepoAccess (
    userName TEXT NOT NULL,     -- Users.name (can be on another domain)
    repoName TEXT NOT NULL,         -- user[@domain]/repo
    level INTEGER DEFAULT 0,    -- 0 for read-only, 1 if they can also push

    UNIQUE(userName, repoName)
);

CREATE TABLE Contributions (
    userName TEXT NOT NULL,
    repo TEXT NOT NULL,             -- user[@domain]/repo
    ts INTEGER NOT NULL,
    id TEXT NOT NULL
);

CREATE TABLE Collabs (
    authorUserName TEXT NOT NULL,
    ts INTEGER NOT NULL,
    repoId INTEGER NOT NULL,
    body TEXT NOT NULL
);


-- Queries planning
-- check if user can view a repo
-- check if user can commit to a repo
