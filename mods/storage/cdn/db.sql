

CREATE TABLE IF NOT EXISTS Assets (
    id INTEGER PRIMARY KEY,
    bucket INTEGER NOT NULL, -- path is data_dir/bucket/id
    mimeType TEXT DEFAULT NULL,
    cacheControl TEXT DEFAULT NULL,
    createTs INTEGER DEFAULT 0,         -- Unix timestamp asset was uploaded
    deleteTs INTEGER DEFAULT -1         -- Unix timestamp to delete the file, or -1 if the file is permanent
);

CREATE TABLE IF NOT EXISTS Tokens (
    token INTEGER UNIQUE,
    assetId INTEGER REFERENCES Assets,
    expireTs INTEGER DEFAULT -1,
);

-- queries

DELETE FROM Tokens WHERE (expireTs < ? AND expireTs != -1) OR assetId IN (SELECT id FROM Assets WHERE deleteTs < ? AND deleteTs != -1);
SELECT bucket, id FROM Assets WHERE deleteTs < ? AND deleteTs != -1;
DELETE FROM Assets WHERE deleteTs < ? AND deleteTs != -1;