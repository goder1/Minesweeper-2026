const screens = document.querySelectorAll('.screen');

function showScreen(name) {
  screens.forEach((el) => el.classList.toggle('active', el.id === `screen-${name}`));

  if (name !== 'play-game') {
    setBombOverlay(false);
    stopTimer();
    paused = false;
    pauseOverlay.hidden = true;
  }
  if (name === 'menu') {
    updateContinueButton();
  }
  if (name === 'leaderboard') {
    loadLeaderboard();
  }
  if (name === 'settings') {
    renderSettings();
  }
  if (name === 'login' || name === 'register') {
    const form = document.getElementById(`${name}-form`);
    const messageEl = document.getElementById(`${name}-message`);
    form.reset();
    setFormMessage(messageEl, '', '');
  }
}

document.querySelectorAll('[data-screen]').forEach((el) => {
  el.addEventListener('click', () => showScreen(el.dataset.screen));
});

const avatarBtn = document.getElementById('avatar-btn');
const avatarInitial = document.getElementById('avatar-initial');
const accountMenu = document.getElementById('account-menu');

const AVATARS = ['🙂', '🐱', '🤖', '🚀', '💣', '🎯', '🦊', '🐧', '🌟', '🔥'];

function getToken() {
  return localStorage.getItem('ms_token');
}

function getNickname() {
  return localStorage.getItem('ms_nickname');
}

function getAvatar() {
  return localStorage.getItem('ms_avatar');
}

function setSession(token, nickname, avatar) {
  localStorage.setItem('ms_token', token);
  localStorage.setItem('ms_nickname', nickname);
  if (avatar) localStorage.setItem('ms_avatar', avatar);
  else localStorage.removeItem('ms_avatar');
  renderAccountInfo();
}

function clearSession() {
  localStorage.removeItem('ms_token');
  localStorage.removeItem('ms_nickname');
  localStorage.removeItem('ms_avatar');
  renderAccountInfo();
}

async function refreshProfile() {
  if (!getToken()) return;
  try {
    const res = await fetch('/api/users/me', {
      headers: { Authorization: `Bearer ${getToken()}` },
    });
    if (!res.ok) return;
    const data = await res.json().catch(() => ({}));
    if (data.avatar) {
      localStorage.setItem('ms_avatar', data.avatar);
      renderAccountInfo();
    }
    if (data.theme) applyTheme(data.theme);
    if (data.controls) applyControlMode(data.controls);
  } catch (err) {

  }
}

function setAccountMenuOpen(open) {
  accountMenu.hidden = !open;
  avatarBtn.setAttribute('aria-expanded', String(open));
}

function renderAccountInfo() {
  const nickname = getNickname();
  const avatar = getAvatar();
  avatarInitial.textContent = avatar || (nickname ? nickname.charAt(0).toUpperCase() : 'G');
  accountMenu.innerHTML = '';
  setAccountMenuOpen(false);

  if (nickname) {
    const name = document.createElement('div');
    name.className = 'menu-name';
    name.textContent = nickname;

    const logout = document.createElement('button');
    logout.textContent = 'Log out';
    logout.addEventListener('click', async () => {
      setAccountMenuOpen(false);
      await leaveActiveGame(true);
      clearSession();
      showScreen('menu');
    });

    accountMenu.append(name, logout);
  } else {
    const login = document.createElement('button');
    login.textContent = 'Log in';
    login.addEventListener('click', () => {
      setAccountMenuOpen(false);
      showScreen('login');
    });

    const register = document.createElement('button');
    register.textContent = 'Register';
    register.addEventListener('click', () => {
      setAccountMenuOpen(false);
      showScreen('register');
    });

    accountMenu.append(login, register);
  }
}

avatarBtn.addEventListener('click', (e) => {
  e.stopPropagation();
  setAccountMenuOpen(accountMenu.hidden);
});

document.addEventListener('click', (e) => {
  if (!accountMenu.hidden && !accountMenu.contains(e.target) && e.target !== avatarBtn) {
    setAccountMenuOpen(false);
  }
});

function escapeHtml(value) {
  const div = document.createElement('div');
  div.textContent = value;
  return div.innerHTML;
}

function formatAchievedAt(isoString) {
  const date = new Date(isoString);
  if (Number.isNaN(date.getTime())) return isoString;
  const pad = (n) => String(n).padStart(2, '0');
  return `${pad(date.getHours())}:${pad(date.getMinutes())} `
       + `${pad(date.getDate())}.${pad(date.getMonth() + 1)}.${date.getFullYear()}`;
}

function setFormMessage(el, message, kind) {
  el.textContent = message;
  el.className = `form-message ${kind || ''}`.trim();
}

document.getElementById('login-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const messageEl = document.getElementById('login-message');
  const form = e.target;
  const submitBtn = form.querySelector('button[type="submit"]');
  const nickname = form.nickname.value.trim();
  const password = form.password.value;

  submitBtn.disabled = true;
  try {
    const res = await fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ nickname, password }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || 'login failed');

    await leaveActiveGame(false);
    setSession(data.token, nickname);
    refreshProfile();
    await tryResumeSavedGame();
    setFormMessage(messageEl, `Welcome back, ${nickname}!`, 'success');
    form.reset();
    showScreen('menu');
  } catch (err) {
    setFormMessage(messageEl, err.message, 'error');
  } finally {
    submitBtn.disabled = false;
  }
});

document.getElementById('register-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const messageEl = document.getElementById('register-message');
  const form = e.target;
  const submitBtn = form.querySelector('button[type="submit"]');
  const nickname = form.nickname.value.trim();
  const password = form.password.value;

  submitBtn.disabled = true;
  try {
    const res = await fetch('/api/auth/register', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ nickname, password }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || 'registration failed');

    await leaveActiveGame(false);
    setSession(data.token, nickname);
    refreshProfile();
    setFormMessage(messageEl, `Account created — welcome, ${nickname}!`, 'success');
    form.reset();
    showScreen('menu');
  } catch (err) {
    setFormMessage(messageEl, err.message, 'error');
  } finally {
    submitBtn.disabled = false;
  }
});

renderAccountInfo();
refreshProfile();

const lbDifficulty = document.getElementById('lb-difficulty');
const lbBody = document.getElementById('lb-body');
const lbMessage = document.getElementById('lb-message');
const lbPagination = document.getElementById('lb-pagination');
const lbPrevBtn = document.getElementById('lb-prev');
const lbNextBtn = document.getElementById('lb-next');
const lbPageLabel = document.getElementById('lb-page-label');

const LB_PAGE_SIZE = 10;
let lbRows = [];
let lbPage = 0;

function renderLeaderboardPage() {
  lbBody.innerHTML = '';

  const pageCount = Math.max(1, Math.ceil(lbRows.length / LB_PAGE_SIZE));
  lbPage = Math.min(lbPage, pageCount - 1);
  const start = lbPage * LB_PAGE_SIZE;
  const pageRows = lbRows.slice(start, start + LB_PAGE_SIZE);

  pageRows.forEach((row, i) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${start + i + 1}</td>
      <td>${row.avatar || ''} ${escapeHtml(row.nickname || row.user_id)}</td>
      <td>${row.time_seconds}s</td>
      <td>${row.mistakes}</td>
      <td>${formatAchievedAt(row.achieved_at)}</td>`;
    lbBody.appendChild(tr);
  });

  lbPagination.hidden = lbRows.length <= LB_PAGE_SIZE;
  lbPageLabel.textContent = `Page ${lbPage + 1} of ${pageCount}`;
  lbPrevBtn.disabled = lbPage === 0;
  lbNextBtn.disabled = lbPage >= pageCount - 1;
}

lbPrevBtn.addEventListener('click', () => {
  if (lbPage > 0) {
    lbPage -= 1;
    renderLeaderboardPage();
  }
});

lbNextBtn.addEventListener('click', () => {
  if (lbPage < Math.ceil(lbRows.length / LB_PAGE_SIZE) - 1) {
    lbPage += 1;
    renderLeaderboardPage();
  }
});

async function loadLeaderboard() {
  lbBody.innerHTML = '';
  lbPagination.hidden = true;
  lbPage = 0;
  setFormMessage(lbMessage, 'Loading…');
  try {
    const res = await fetch(`/api/leaderboard?difficulty=${encodeURIComponent(lbDifficulty.value)}&limit=100`);
    const data = await res.json().catch(() => ([]));
    if (!res.ok) throw new Error((data && data.error) || 'failed to load leaderboard');

    lbRows = Array.isArray(data) ? data : [];
    if (lbRows.length === 0) {
      setFormMessage(lbMessage, 'No records yet for this difficulty.');
      return;
    }
    setFormMessage(lbMessage, '');
    renderLeaderboardPage();
  } catch (err) {
    setFormMessage(lbMessage, err.message, 'error');
  }
}

lbDifficulty.addEventListener('change', loadLeaderboard);

const themeButtons = document.querySelectorAll('.theme-btn');
const settingsAccount = document.getElementById('settings-account');
const settingsGuestNote = document.getElementById('settings-guest-note');
const nicknameForm = document.getElementById('nickname-form');
const nicknameMessage = document.getElementById('nickname-message');
const avatarPicker = document.getElementById('avatar-picker');
const avatarMessage = document.getElementById('avatar-message');

function getTheme() {
  return localStorage.getItem('ms_theme') || 'light';
}

function applyTheme(theme) {
  document.documentElement.dataset.theme = theme;
  localStorage.setItem('ms_theme', theme);
  themeButtons.forEach((btn) => btn.classList.toggle('active', btn.dataset.theme === theme));
}

themeButtons.forEach((btn) => {
  btn.addEventListener('click', async () => {
    const theme = btn.dataset.theme;
    applyTheme(theme);
    if (getToken()) {
      try {
        await patchProfile({ theme });
      } catch (err) {
      }
    }
  });
});

applyTheme(getTheme());

const controlsButtons = document.querySelectorAll('.controls-btn');

function getControlMode() {
  return localStorage.getItem('ms_controls') || 'standard';
}

function applyControlMode(mode) {
  localStorage.setItem('ms_controls', mode);
  controlsButtons.forEach((btn) => btn.classList.toggle('active', btn.dataset.mode === mode));
}

controlsButtons.forEach((btn) => {
  btn.addEventListener('click', async () => {
    const mode = btn.dataset.mode;
    applyControlMode(mode);
    if (getToken()) {
      try {
        await patchProfile({ controls: mode });
      } catch (err) {
      }
    }
  });
});

applyControlMode(getControlMode());

async function patchProfile(body) {
  const res = await fetch('/api/users/me', {
    method: 'PATCH',
    headers: {
      'Content-Type': 'application/json',
      Authorization: `Bearer ${getToken()}`,
    },
    body: JSON.stringify(body),
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.error || 'update failed');
  return data;
}

function renderAvatarPicker() {
  avatarPicker.innerHTML = '';
  const current = getAvatar();
  AVATARS.forEach((avatar) => {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'avatar-option';
    btn.textContent = avatar;
    btn.classList.toggle('selected', avatar === current);
    btn.addEventListener('click', async () => {
      try {
        const data = await patchProfile({ avatar });
        setSession(data.token, data.nickname, data.avatar);
        renderAvatarPicker();
        setFormMessage(avatarMessage, 'Avatar updated.', 'success');
      } catch (err) {
        setFormMessage(avatarMessage, err.message, 'error');
      }
    });
    avatarPicker.appendChild(btn);
  });
}

function renderSettings() {
  const loggedIn = Boolean(getToken());
  settingsAccount.hidden = !loggedIn;
  settingsGuestNote.hidden = loggedIn;
  setFormMessage(nicknameMessage, '', '');
  setFormMessage(avatarMessage, '', '');
  if (loggedIn) {
    renderAvatarPicker();
  }
}

nicknameForm.addEventListener('submit', async (e) => {
  e.preventDefault();
  const nickname = nicknameForm.nickname.value.trim();
  if (!nickname) return;
  try {
    const data = await patchProfile({ nickname });
    setSession(data.token, data.nickname, data.avatar);
    setFormMessage(nicknameMessage, `Nickname updated to ${data.nickname}.`, 'success');
    nicknameForm.reset();
  } catch (err) {
    setFormMessage(nicknameMessage, err.message, 'error');
  }
});

const DIFFICULTIES = [
  { value: 'beginner', label: 'Beginner', description: '9×9 board, 14 mines' },
  { value: 'intermediate', label: 'Intermediate', description: '16×16 board, 45 mines' },
  { value: 'expert', label: 'Expert', description: '30×30 board, 155 mines' },
];

const diffLabel = document.getElementById('diff-label');
const diffDescription = document.getElementById('diff-description');
const diffPrevButton = document.getElementById('diff-prev');
const diffNextButton = document.getElementById('diff-next');
const undoButton = document.getElementById('undo-btn');
const bombOverlay = document.getElementById('bomb-overlay');
const statusEl = document.getElementById('status');
const boardEl = document.getElementById('board');
const mistakeCounterEl = document.getElementById('mistake-counter');
const timerEl = document.getElementById('timer');
const pauseBtn = document.getElementById('pause-btn');
const pauseOverlay = document.getElementById('pause-overlay');
const resumeBtn = document.getElementById('resume-btn');
const quitBtn = document.getElementById('quit-btn');
const winMenuBtn = document.getElementById('win-menu-btn');
const continueBtn = document.getElementById('continue-btn');

let sessionId = null;
let difficultyIndex = 0;
let paused = false;
let gameOver = false;
let savedElapsedSeconds = null;

function setDifficulty(index) {
  difficultyIndex = (index + DIFFICULTIES.length) % DIFFICULTIES.length;
  diffLabel.textContent = DIFFICULTIES[difficultyIndex].label;
  diffDescription.textContent = DIFFICULTIES[difficultyIndex].description;
}

function beginGame() {
  showScreen('play-game');
  startGame();
}

function setBombOverlay(visible) {
  bombOverlay.classList.toggle('visible', visible);
  undoButton.hidden = !visible;
}

let elapsedSeconds = 0;
let timerHandle = null;

function formatTime(totalSeconds) {
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
}

function renderTimer() {
  timerEl.textContent = formatTime(elapsedSeconds);
}

function startTimer() {
  stopTimer();
  elapsedSeconds = 0;
  renderTimer();
  timerHandle = setInterval(() => {
    elapsedSeconds += 1;
    renderTimer();
  }, 1000);
}

function stopTimer() {
  if (timerHandle !== null) {
    clearInterval(timerHandle);
    timerHandle = null;
  }
}

function resumeTimer() {
  stopTimer();
  renderTimer();
  timerHandle = setInterval(() => {
    elapsedSeconds += 1;
    renderTimer();
  }, 1000);
}

function openPause() {
  if (paused) return;
  paused = true;
  pauseOverlay.hidden = false;
  stopTimer();
}

function closePause() {
  if (!paused) return;
  paused = false;
  pauseOverlay.hidden = true;
  if (!gameOver) {
    resumeTimer();
  }
}

function updateContinueButton() {
  continueBtn.hidden = !sessionId || gameOver;
}

async function leaveActiveGame(shouldSave) {
  if (!sessionId || gameOver) return;
  if (shouldSave) {
    try {
      await api('/api/game/save', { method: 'POST', body: { elapsed_secs: elapsedSeconds } });
    } catch (err) {
    }
  }
  sessionId = null;
  gameOver = false;
  savedElapsedSeconds = null;
  paused = false;
  pauseOverlay.hidden = true;
  stopTimer();
}

async function tryResumeSavedGame() {
  if (!getToken() || sessionId) return;
  try {
    const data = await api('/api/game/resume', { method: 'POST' });
    sessionId = data.session_id;
    gameOver = false;
    savedElapsedSeconds = typeof data.elapsed_secs === 'number' ? data.elapsed_secs : null;
    updateContinueButton();
  } catch (err) {
  }
}

function isPlayingScreen() {
  return document.getElementById('screen-play-game').classList.contains('active');
}

pauseBtn.addEventListener('click', openPause);
resumeBtn.addEventListener('click', closePause);
quitBtn.addEventListener('click', () => {
  paused = false;
  pauseOverlay.hidden = true;
  stopTimer();
  showScreen('menu');
});

document.addEventListener('keydown', (e) => {
  if (e.key !== 'Escape' || !isPlayingScreen()) return;
  if (paused) closePause();
  else openPause();
});

async function api(path, { method = 'GET', body } = {}) {
  const headers = { 'Content-Type': 'application/json' };
  if (sessionId) headers['X-Session-Id'] = sessionId;
  if (getToken()) headers['Authorization'] = `Bearer ${getToken()}`;

  const res = await fetch(path, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    throw new Error(data.error || `request failed: ${res.status}`);
  }
  return data;
}

async function startGame() {
  try {
    const data = await api('/api/game/start', {
      method: 'POST',
      body: { difficulty: DIFFICULTIES[difficultyIndex].value },
    });
    sessionId = data.session_id;
    gameOver = false;
    winMenuBtn.hidden = true;
    startTimer();
    renderBoard(data.board);
    updateContinueButton();
  } catch (err) {
    showError(err);
  }
}

async function continueGame() {
  if (!sessionId || gameOver) return;
  try {
    const board = await api('/api/game/state');
    winMenuBtn.hidden = true;
    showScreen('play-game');
    renderBoard(board);
    if (savedElapsedSeconds !== null) {
      elapsedSeconds = savedElapsedSeconds;
      savedElapsedSeconds = null;
      renderTimer();
    }
    resumeTimer();
  } catch (err) {
    sessionId = null;
    updateContinueButton();
    showError(err);
  }
}

async function reveal(x, y) {
  try {
    renderBoard(await api('/api/game/move', { method: 'POST', body: { x, y } }));
  } catch (err) {
    showError(err);
  }
}

async function toggleFlag(x, y) {
  try {
    renderBoard(await api('/api/game/flag', { method: 'POST', body: { x, y } }));
  } catch (err) {
    showError(err);
  }
}

async function revert() {
  try {
    renderBoard(await api('/api/game/revert', { method: 'POST' }));
  } catch (err) {
    showError(err);
  }
}

function showError(err) {
  statusEl.textContent = err.message;
  statusEl.className = 'status lost';
}

function renderBoard(board) {
  boardEl.style.gridTemplateColumns = `repeat(${board.width}, var(--cell-size))`;
  boardEl.innerHTML = '';

  board.cells.forEach((row, y) => {
    row.forEach((cell, x) => {
      const div = document.createElement('div');
      div.className = 'cell';

      if (cell.state === 'revealed') {
        div.classList.add('revealed');
        if (cell.adjacent_mines > 0) {
          div.classList.add(`n${cell.adjacent_mines}`);
          div.textContent = cell.adjacent_mines;
        }
      } else if (cell.state === 'flagged') {
        div.classList.add('flagged');
      } else if (cell.state === 'mine') {
        div.classList.add('mine');
      }

      div.addEventListener('click', () => {
        if (cell.state === 'revealed') {
          reveal(x, y);
        } else if (getControlMode() === 'swapped') {
          toggleFlag(x, y);
        } else {
          reveal(x, y);
        }
      });
      div.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        if (cell.state === 'revealed') {
          return;
        }
        if (getControlMode() === 'swapped') reveal(x, y);
        else toggleFlag(x, y);
      });

      boardEl.appendChild(div);
    });
  });

  updateStatus(board);
}

function updateStatus(board) {
  setBombOverlay(board.pending_revert);
  mistakeCounterEl.textContent = `Mistakes: ${board.mistake_count}`;

  if (board.status === 'won') {
    statusEl.textContent = 'You won!';
    statusEl.className = 'status won';
    gameOver = true;
    winMenuBtn.hidden = false;
    stopTimer();
  } else if (board.pending_revert) {
    statusEl.textContent = 'Boom! Hit Undo to continue.';
    statusEl.className = 'status lost';
  } else {
    statusEl.textContent = '';
    statusEl.className = 'status';
  }
}

winMenuBtn.addEventListener('click', () => {
  paused = false;
  pauseOverlay.hidden = true;
  stopTimer();
  sessionId = null;
  showScreen('menu');
});

continueBtn.addEventListener('click', continueGame);
undoButton.addEventListener('click', revert);
diffPrevButton.addEventListener('click', () => setDifficulty(difficultyIndex - 1));
diffNextButton.addEventListener('click', () => setDifficulty(difficultyIndex + 1));
diffLabel.addEventListener('click', beginGame);
diffLabel.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' || e.key === ' ') {
    e.preventDefault();
    beginGame();
  }
});

setDifficulty(0);
showScreen('menu');
tryResumeSavedGame();
