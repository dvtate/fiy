CREATE DATABASE gitfed;
USE gitfed;

CREATE TABLE Users (
    `username` VARCHAR(128) PRIMARY KEY,
    `password` CHAR(128) NOT NULL,
);

CREATE TABLE UserEmails (
    `username` VARCHAR(128) REFERENCES Users,
    `email` VARCHAR(128) NOT NULL
);

-- remote repos are not stored in our databese?
CREATE TABLE Repo (
    `username` VARCHAR(128) REFERENCES Users,
);

CREATE TABLE RepoLikes (

);