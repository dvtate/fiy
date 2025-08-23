--CREATE DATABASE fediy;

CREATE TABLE Peers (
    -- domain used to connect to peer
    domain          VARCHAR(255) PRIMARY KEY,

    -- when we connected with the peer
    connectTs       BIGINT NOT NULL,

    -- token we give to identify ourselves to the peer
    giveToken       CHAR(24) NOT NULL,

    -- token we accept as identification for peer
    takeToken       CHAR(24) NOT NULL,

    -- use to generate signature for the peer
    symKey          BLOB,

    -- cached public key
    pubkey          TEXT,

    -- when does this peer's authentication expire
    -- (probably would be more secure if it didn't expire)
    tokenExpireTs   BIGINT
);

CREATE TABLE Users (
    username       VARCHAR(32) PRIMARY KEY,
    isAdmin        INTEGER NOT NULL,
    name           VARCHAR(128) NOT NULL DEFAULT "", -- Moved to contacts app
    hashedPassword CHAR(128) NOT NULL,
    email          VARCHAR(255) NOT NULL DEFAULT "",
    locale         VARCHAR(12)   NOT NULL DEFAULT "en",
    joinTs         INTEGER UNSIGNED NOT NULL
);
