ALTER TABLE users ALTER COLUMN avatar SET DEFAULT '🙂';
UPDATE users SET avatar = '🙂' WHERE avatar !~ '^[🙂🐱🤖🚀💣🎯🦊🐧🌟🔥]$';
