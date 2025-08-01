
-- two types of contacts
--  1. user profiles created for themselves
--      these have some fields public and others private
--  2. contacts created for other users
--      all fields visible for themselves


CREATE TABLE Contacts (
    -- unique id for the contact
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- fediy user who owns this contact
    owner TEXT NOT NULL,

    -- display name for the contact
    name TEXT NOT NULL,

    -- fediy user that this contact corresponds to
    user TEXT
);

CREATE TABLE ContactFields (
    -- Relevant contact
    contactId INTEGER REFERENCES Contacts,

    -- Field type
    name TEXT,
    vcardName TEXT,

    -- Field value
    value TEXT
);

CREATE TABLE ProfileContactFields (
    contactId INTEGER

    -- Field type
    name TEXT,
    vcardName TEXT,

    -- Field value
    value TEXT

    -- Who can see this field?
    -- 0 = only me
    -- 1 = users on my instance
    -- 2 = users on other instances
    -- 3 = public
    visibility INTEGER DEFAULT 0
);

-- entry required for both sending and receiving instances
CREATE TABLE SharedContacts (
    contactId INTEGER,
    sendingUser TEXT,
    acceptUser TEXT
);

-- query planning

-- delete a contact
DELETE FROM ProfileContactFields WHERE contactId = ?;
DELETE FROM SharedContacts WHERE contactId = ?;
DELETE FROM Contacts WHERE id = ?;

--