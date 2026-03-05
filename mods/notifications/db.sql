
--
CREATE TABLE Notifications (
    -- unique id
    id INTEGER PRIMARY KEY,

    -- Unix timestamp the notif was created at
    ts INTEGER NOT NULL,

    -- module that created the notification
    modId TEXT DEFAULT NULL,

    -- relevant instance
    instance TEXT DEFAULT NULL,

    -- User this notification is for
    user TEXT NOT NULL,

    -- short notification text
    title TEXT NOT NULL,

    -- optional expanded notification body
    description TEXT DEFAULT NULL,

    -- Where to send the user if they click on it
    link TEXT,

    -- Link to relevant image for the notification
    icon TEXT,

    -- Unix timestamp the notif was viewed at
    viewTs INTEGER DEFAULT 0,
);

-- Future feature that lets notifications add actions
--  eg. mute, mark as read, block, follow back, etc.
CREATE TABLE NotificationActions (
    actionId INTEGER PRIMARY KEY,
    notificationId INTEGER REFERENCES Notifications,

    label TEXT NOT NULL,
    icon TEXT NOT NULL,
    action TEXT NOT NULL
);