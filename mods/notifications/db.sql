

CREATE TABLE Notifications (
    id INTEGER PRIMARY KEY,
    appid TEXT,
    title TEXT,

    --
    description TEXT,

    -- Where to send the user if they click on it
    link TEXT,

    -- Unix timestamp the message was created at
    ts INTEGER,

    -- What notification state
    -- 0 - not viewed
    -- 1 - viewed
    -- 2 - cleared
    status INTEGER DEFAULT 0

);