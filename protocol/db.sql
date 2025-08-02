--CREATE DATABASE fediy;

CREATE TABLE Peers (
    domain          VARCHAR(64) PRIMARY KEY,
    connectTs       BIGINT NOT NULL,
    giveToken       CHAR(24) NOT NULL,
    takeToken       CHAR(24) NOT NULL,
    symKey          BLOB,
    pubkey          TEXT,
    tokenExpireTs   BIGINT
);

CREATE TABLE Users (
    username       VARCHAR(32) PRIMARY KEY,
    isAdmin        INTEGER NOT NULL,
    name           VARCHAR(128) NOT NULL DEFAULT "",
    hashedPassword CHAR(128) NOT NULL,
    email          VARCHAR(255) NOT NULL DEFAULT "",
    locale         VARCHAR(12)   NOT NULL DEFAULT "en",
    joinTs         INTEGER UNSIGNED NOT NULL
);

-- CREATE TABLE UserTokens (
--     token    VARCHAR(32) PRIMARY KEY,
--     username VARCHAR(32) REFERENCES Users
-- );
