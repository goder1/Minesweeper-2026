CREATE EXTENSION IF NOT EXISTS "pgcrypto";

CREATE TABLE users (
    id            UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    nickname      VARCHAR(32)  NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE TABLE records (
    user_id       UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    difficulty    VARCHAR(16) NOT NULL CHECK (difficulty IN ('beginner', 'intermediate', 'expert', 'custom')),
    time_seconds  INTEGER     NOT NULL CHECK (time_seconds > 0),
    mistake_count INTEGER     NOT NULL DEFAULT 0 CHECK (mistake_count >= 0),
    achieved_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, difficulty)
);

-- state JSONB layout:
--   {
--     "mines":    [4, 17, 33, ...],   -- flat indices: y*width + x
--     "revealed": [0,  1,  2, ...],   -- non-mine cells the player has opened
--     "flagged":  [8, 22]             -- cells the player has flagged
--   }
CREATE TABLE games (
    id            UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id       UUID        NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
    difficulty    VARCHAR(16) NOT NULL CHECK (difficulty IN ('beginner', 'intermediate', 'expert', 'custom')),
    width         SMALLINT    NOT NULL CHECK (width > 0),
    height        SMALLINT    NOT NULL CHECK (height > 0),
    mine_cnt      SMALLINT    NOT NULL CHECK (mine_cnt > 0),
    state         JSONB       NOT NULL,
    elapsed_secs  INTEGER     NOT NULL DEFAULT 0 CHECK (elapsed_secs >= 0),
    mistake_count INTEGER     NOT NULL DEFAULT 0 CHECK (mistake_count >= 0)
);
