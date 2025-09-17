
-- two types of contacts
--  1. user profiles created for themselves
--      these have some fields public and others private
--  2. contacts created for other users
--      all fields visible for themselves

CREATE TABLE Cards (
    -- unique id for the contact
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- fediy user who owns this contact
    owner TEXT NOT NULL,

    -- fediy user that this contact corresponds to
    user TEXT DEFAULT NULL,

    updateTs INTEGER
);

-- card properties
CREATE TABLE Properties (
    -- Relevant contact
--     cardId INTEGER REFERENCES Cards,
    id INTEGER PRIMARY KEY,

    -- Field type
    name TEXT,

    params TEXT,

    -- Field value
    value TEXT
);

CREATE TABLE CardProperties (
    cardId INTEGER REFERENCES Cards,
    propertyId INTEGER REFERENCES Properties,
    UNIQUE(cardId, propertyId)
);

CREATE TABLE ProfileCardProperties (
    cardId INTEGER REFERENCES Cards,
    propertyId INTEGER REFERENCES Properties,

    -- Who can see this field?
    -- 0 = only me/people I share to
    -- 1 = users on my instance
    -- 2 = users on other instances
    -- 3 = public
    visibility INTEGER DEFAULT 0
);

-- entry required for both sending and receiving instances
CREATE TABLE SharedCards (
    cardId INTEGER,
    sendingUser TEXT, -- could be remote user
    acceptUser TEXT
);

-- erase db
-- delete from Cards; delete from Properties; delete from CardProperties; delete from ProfileCardProperties;
