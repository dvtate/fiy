-- CREATE DATABASE fiygit;
-- USE fiygit;

-- Local users
CREATE TABLE Users (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE,

    -- PROBLEM: what if someone creates a user with org name?
    -- SOLUTION: orgs are just users
    isOrganization INTEGER DEFAULT 0
);

-- remote repos are not stored in our database?
CREATE TABLE Repos (
    id INTEGER PRIMARY KEY
    owner INTEGER REFERENCES Users,
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
    repo TEXT NOT NULL,         -- user[@domain]/repo
    canWrite INTEGER DEFAULT 0, -- 0 for read-only, 1 if they can also push

    UNIQUE(userName, repoName)
);

CREATE TABLE Contributions (
    user INTEGER REFERENCES Users,
    repo TEXT NOT NULL,             -- user[@domain]/repo
    ts INTEGER NOT NULL,

);

-- Queries planning
-- check if user can view a repo

-- check if user can commit to a repo
