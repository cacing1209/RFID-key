const express = require("express");
const app = express();

const PORT = 3000;
const API_KEY = "lockerqyubitL0002L0004L0008L000264L000128";

// ── Middleware ──────────────────────────────────────────
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Simple logger
app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.path}`);
  next();
});

// ── Auth Middleware ─────────────────────────────────────
function checkApiKey(req, res, next) {
  const key = req.headers["x-api-key"];
  if (!key || key !== API_KEY) {
    return res.status(401).json({ error: "Unauthorized", code: 401 });
  }
  next();
}

// ── In-memory log storage ───────────────────────────────
const eventLogs = [];

// ── POST /event-log ─────────────────────────────────────
// Dipanggil oleh Arduino send_eventLog()
// Body: { "t": "2026-02-28 08:14:19", "no": 3, "id": 28758077 }
app.post("/event-log", checkApiKey, (req, res) => {
  const { t, no, id } = req.body;

  if (t === undefined || no === undefined || id === undefined) {
    return res.status(400).json({ error: "Missing fields: t, no, id", code: 400 });
  }

  // Format uid_decimal jadi 10 digit leading zero (sama seperti Arduino sprintf %010lu)
  const card_uid = String(id).padStart(10, "0");

  const log = {
    id: eventLogs.length + 1,
    timestamp: t,
    locker: no,
    card_uid: card_uid,
    received: new Date().toISOString(),
  };

  eventLogs.push(log);

  console.log(`  ✓ Event log → locker #${no} | card ${card_uid} @ ${t}`);
  res.status(200).json({ status: "ok", id: log.id });
});

// ── GET /event-log ──────────────────────────────────────
// Lihat semua log yang masuk (untuk debug)
app.get("/event-log", (req, res) => {
  const limit = parseInt(req.query.limit) || 50;
  const locker = req.query.locker !== undefined ? parseInt(req.query.locker) : null;

  let result = [...eventLogs].reverse(); // terbaru dulu

  if (locker !== null) {
    result = result.filter(l => l.locker === locker);
  }

  result = result.slice(0, limit);

  res.json({
    total: eventLogs.length,
    count: result.length,
    logs: result,
  });
});

// ── GET /event-log/:id ──────────────────────────────────
app.get("/event-log/:id", (req, res) => {
  const log = eventLogs.find(l => l.id === parseInt(req.params.id));
  if (!log) return res.status(404).json({ error: "Not found", code: 404 });
  res.json(log);
});

// ── DELETE /event-log ───────────────────────────────────
// Clear semua log
app.delete("/event-log", checkApiKey, (req, res) => {
  const count = eventLogs.length;
  eventLogs.length = 0;
  res.json({ status: "cleared", deleted: count });
});

// ── GET /health ─────────────────────────────────────────
app.get("/health", (req, res) => {
  res.json({
    status: "ok",
    uptime: Math.floor(process.uptime()),
    logs: eventLogs.length,
    time: new Date().toISOString(),
  });
});

// ── 404 fallback ────────────────────────────────────────
app.use((req, res) => {
  res.status(404).json({ error: "Not found", code: 404 });
});

// ── Start ────────────────────────────────────────────────
app.listen(PORT, () => {
  console.log(`
╔══════════════════════════════════════════╗
║       Arduino Locker Event Log Server    ║
╚══════════════════════════════════════════╝
  Port    : ${PORT}
  API Key : ${API_KEY}

  Endpoints:
  POST   /event-log        ← dari Arduino
  GET    /event-log        ← lihat semua log
  GET    /event-log?locker=0  ← filter per locker
  GET    /event-log/:id    ← detail 1 log
  DELETE /event-log        ← clear semua log
  GET    /health           ← cek server
`);
});