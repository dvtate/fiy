
-- this file is currently just for planning

CREATE TABLE UserKeys (
    name TEXT PRIMARY KEY,
    key TEXT
);

CREATE TABLE Chats (
    id INTEGER PRIMARY KEY,
    createTs INTEGER
);

CREATE TABLE ChatMembers (
    chatId INTEGER REFERENCES Chats,
    user TEXT NOT NULL,
    joinTs INTEGER
);

CREATE TABLE Messages (
    chatId INTEGER REFERENCES Chats,
    user TEXT, -- NULL == system message
    ts INTEGER NOT NULL
);