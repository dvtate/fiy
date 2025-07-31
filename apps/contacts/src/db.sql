
CREATE TABLE Contacts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ownerUserName VARCHAR,
    fullName VARCHAR,
    user VARCHAR,
    fieldsJson VARCHAR
);

-- When a user sends another user a contact
-- CREATE TABLE PendingContacts (
--     sendingUser VARCHAR,
--     acceptUser VARCHAR,
--     fullName VARCHAR,
--     user VARCHAR,
--     fieldsJson VARCHAR
-- )