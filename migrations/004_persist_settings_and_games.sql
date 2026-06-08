ALTER TABLE games ADD COLUMN last_updated TIMESTAMPTZ NOT NULL DEFAULT NOW();

ALTER TABLE users
    ADD COLUMN theme         VARCHAR(8) NOT NULL DEFAULT 'light'    CHECK (theme IN ('light', 'dark')),
    ADD COLUMN controls_mode VARCHAR(8) NOT NULL DEFAULT 'standard' CHECK (controls_mode IN ('standard', 'swapped'));
