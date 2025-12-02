

CREATE TABLE Notifications (
    id INTEGER PRIMARY KEY,
    modid TEXT,
    title TEXT,

    --
    description TEXT,

    -- Where to send the user if they click on it
    link TEXT,

    -- Unix timestamp the notif was created at
    ts INTEGER,

    -- Unix timestamp the notif was viewed at
    viewTs INTEGER,

    -- What notification state
    -- 0 - not viewed
    -- 1 - viewed
    -- 2 - archived
    status INTEGER DEFAULT 0
);