CREATE TABLE EXTENSION_POOLS(
NAME VARCHAR(80) NOT NULL,
ORG_ID INTEGER NOT NULL,
ID INTEGER NOT NULL CONSTRAINT PK_EXTENSION_POOLS1 PRIMARY KEY);

CREATE TABLE EXTENSIONS(
EXTENSION_NUMBER VARCHAR(30) NOT NULL CONSTRAINT PK_EXTENSIONS1 PRIMARY KEY,
EXT_POOL_ID INTEGER NOT NULL,
STATUS VARCHAR(1) NOT NULL);

ALTER TABLE EXTENSION_POOLS
ADD CONSTRAINT FK_EXTENSION_POOLS_1 
FOREIGN KEY (ORG_ID) REFERENCES ORGANIZATIONS (ID);

ALTER TABLE EXTENSIONS
ADD CONSTRAINT FK_EXTENSIONS_1 
FOREIGN KEY (EXT_POOL_ID) REFERENCES EXTENSION_POOLS (ID);

CREATE SEQUENCE EXTENSION_POOLS_SEQ;

CREATE UNIQUE INDEX IDX_EXTENSION_POOLS_1 ON EXTENSION_POOLS (ORG_ID,NAME);