
-- Design:
-- Multiple contacts (Cards) can share the same vcard field data (Properties)
-- Cards use field data via [Profile]CardProperties.

-- two types of contacts
--  1. user profiles created for themselves
--      fields have visibility levels for sharing
--  2. contacts created for other users
--      all fields visible for themselves

-- User contacts
CREATE TABLE Cards (
    -- unique id for the contact
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- FIY user who owns this contact
    owner TEXT NOT NULL,

    -- FIY user that this contact corresponds to
    user TEXT DEFAULT NULL,

    updateTs INTEGER
);

-- vCard field data
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

-- Public card fields
CREATE TABLE CardProperties (
    cardId INTEGER REFERENCES Cards,
    propertyId INTEGER REFERENCES Properties,
    UNIQUE(cardId, propertyId)
);

-- Fields with restricted visibility (for use in profile cards)
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
