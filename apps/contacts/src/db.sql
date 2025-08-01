
CREATE TABLE Contacts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ownerUserName TEXT NOT NULL,
    fullName TEXT,
    user TEXT,
    fieldsJson TEXT
);

CREATE TABLE ContactFields (
    contactId INTEGER REFERENCES Contacts,
    name TEXT,
    vcardName TEXT,
    value TEXT
);

SELECT Contacts.id, Contacts.fullName, Contacts.user FROM Contacts,

-- When a user sends another user a contact
-- CREATE TABLE PendingContacts (
--     sendingUser TEXT,
--     acceptUser TEXT,
--     fullName TEXT,
--     user TEXT,
--     fieldsJson TEXT
-- )