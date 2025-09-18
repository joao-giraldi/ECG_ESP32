const sel = document.getElementById("fileSel");
const fsIn = document.getElementById("fs");
const fmtSel = document.getElementById("fmt");
const gainIn = document.getElementById("gain");
const meta = document.getElementById("meta");
const stat = document.getElementById("stat");
const btn = document.getElementById("btnLoad");
const cvs = document.getElementById("chart");
const ctx = cvs.getContext("2d");
const xScroll = document.getElementById("xScroll");
const zoomCtrl = document.getElementById("zoomCtrl");

let W = 0,
  H = 0;
const DPR = window.devicePixelRatio || 1;

let fullData = null;
let fsCur = 100;
let yMinG = 0,
  yMaxG = 1;

let viewStart = 0;
let viewLen = 0;
let maxViewLen = 0;

const MIN_SAMPLES_ABS = 32;
const MIN_WINDOW_SECONDS = 3;
let minViewLen = 200;

let isDragging = false;
let dragStartX = 0;
let dragStartView = 0;

let rafId = 0;
function scheduleRender() {
  if (rafId) return;
  rafId = requestAnimationFrame(() => {
    rafId = 0;
    redraw();
  });
}

function syncCanvasSizeIfNeeded() {
  const r = cvs.getBoundingClientRect();
  const newW = Math.floor(r.width * DPR);
  const newH = Math.floor(r.height * DPR);
  if (newW !== W || newH !== H) {
    W = newW;
    H = newH;
    cvs.width = W;
    cvs.height = H;
    return true;
  }
  return false;
}
function handleResize() {
  if (syncCanvasSizeIfNeeded()) scheduleRender();
}
window.addEventListener("resize", handleResize);
handleResize();

function prettyBytes(b) {
  const u = ["B", "KB", "MB", "GB"];
  let i = 0;
  while (b >= 1024 && i < u.length - 1) {
    b /= 1024;
    i++;
  }
  return b.toFixed(i ? 1 : 0) + " " + u[i];
}

// === LISTAGEM DO SD COM TRATAMENTO DE ERRO ===
async function listFiles() {
  try {
    const r = await fetch("/api/files");
    if (!r.ok) throw new Error("HTTP " + r.status);
    const js = await r.json();
    console.log(js)

    sel.innerHTML = "";
    if (!Array.isArray(js) || js.length === 0) {
      const o = document.createElement("option");
      o.text = "(nenhum .bin encontrado)";
      o.value = "";
      sel.appendChild(o);
      btn.disabled = true;
      return [];
    }

    for (const f of js) {
      const o = document.createElement("option");
      o.value = f.name;
      o.text = `${f.name} (${prettyBytes(f.size)})`;
      sel.appendChild(o);
    }
    btn.disabled = false;
    return js;
  } catch (e) {
    sel.innerHTML = "";
    const o = document.createElement("option");
    o.text = "(falha ao listar)";
    o.value = "";
    sel.appendChild(o);
    btn.disabled = true;
    return [];
  }
}

function parseBuffer(buf) {
  const fmt = fmtSel.value;
  const g = parseFloat(gainIn.value) || 1;
  const dv = new DataView(buf);
  let N = buf.byteLength;
  let arr;

  if (fmt === "i16le") {
    N >>= 1;
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getInt16(i * 2, true) * g;
  } else if (fmt === "u16le") {
    N >>= 1;
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getUint16(i * 2, true) * g;
  } else if (fmt === "i8") {
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getInt8(i) * g;
  } else {
    // u8
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getUint8(i) * g;
  }
  return arr;
}

function decimateByStep(data, step) {
  if (step <= 1) return data;
  const out = new Float32Array(Math.ceil(data.length / step));
  let j = 0;
  for (let i = 0; i < data.length; i += step) out[j++] = data[i];
  return out;
}
function downsampleForWidth(dataWindow) {
  if (!dataWindow || dataWindow.length === 0) return dataWindow;
  const maxPts = Math.max(600, Math.floor(W / Math.max(1, DPR)));
  if (dataWindow.length <= maxPts) return dataWindow;
  const step = Math.ceil(dataWindow.length / maxPts);
  return decimateByStep(dataWindow, step);
}

function redraw() {
  syncCanvasSizeIfNeeded();
  ctx.clearRect(0, 0, W, H);

  ctx.fillStyle = "#0b0b0d";
  ctx.fillRect(0, 0, W, H);

  ctx.strokeStyle = "rgba(255,255,255,.05)";
  ctx.lineWidth = 1;
  const gx = 10,
    gy = 8;
  for (let i = 1; i < gx; i++) {
    const x = (i * W) / gx;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, H);
    ctx.stroke();
  }
  for (let i = 1; i < gy; i++) {
    const y = (i * H) / gy;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(W, y);
    ctx.stroke();
  }

  if (!fullData || fullData.length === 0) {
    ctx.fillStyle = "#9ca3af";
    ctx.font = `${14 * DPR}px system-ui`;
    ctx.fillText(
      "Selecione um arquivo e clique em Carregar",
      16 * DPR,
      28 * DPR
    );
    return;
  }

  const start = Math.max(0, Math.min(viewStart, fullData.length - 1));
  const end = Math.min(fullData.length, start + viewLen);
  const win = fullData.subarray(start, end);
  const data = downsampleForWidth(win);

  const pad = 8 * DPR;
  const yMin = yMinG,
    yMax = yMaxG;
  const kY = (H - 2 * pad) / (yMax - yMin || 1);

  ctx.strokeStyle = "#22d3ee";
  ctx.lineWidth = Math.max(1, 1 * DPR);
  ctx.beginPath();
  const N = data.length;
  for (let i = 0; i < N; i++) {
    const x = pad + (i * (W - 2 * pad)) / Math.max(1, N - 1);
    const y = H - pad - (data[i] - yMin) * kY;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  const secs = viewLen / (fsCur || 1);
  ctx.fillStyle = "#9ca3af";
  ctx.font = `${12 * DPR}px system-ui`;
  ctx.fillText(
    `${secs.toFixed(2)} s @ ${fsCur} Hz — ${viewLen} pts (janela)`,
    pad,
    H - 6 * DPR
  );
}

function lenToZoomValue(len) {
  const min = Math.max(1, minViewLen);
  const max = Math.max(min + 1, maxViewLen);
  const t = (Math.log(len) - Math.log(min)) / (Math.log(max) - Math.log(min));
  return Math.round(t * 1000);
}
function zoomValueToLen(v) {
  const min = Math.max(1, minViewLen);
  const max = Math.max(min + 1, maxViewLen);
  const t = Math.min(1, Math.max(0, v / 1000));
  return Math.round(
    Math.exp(Math.log(min) + t * (Math.log(max) - Math.log(min)))
  );
}

function setViewport(start, len) {
  if (!fullData) return;
  const newLen = Math.max(minViewLen, Math.min(len, maxViewLen));
  const newStart = Math.max(0, Math.min(start, fullData.length - newLen));

  if (newLen === viewLen && newStart === viewStart) return;

  viewLen = newLen;
  viewStart = newStart;

  if (xScroll) {
    const denom = fullData.length - viewLen;
    const pos = denom > 0 ? Math.round((viewStart / denom) * 1000) : 0;
    xScroll.value = String(pos);
  }
  if (zoomCtrl) {
    zoomCtrl.value = String(lenToZoomValue(viewLen));
  }

  scheduleRender();
}

function setZoomFromAnchor(newLen, anchorPx) {
  const frac = Math.max(
    0,
    Math.min(1, (anchorPx - 8 * DPR) / Math.max(1, W - 16 * DPR))
  );
  const anchorIdx = Math.floor(viewStart + frac * viewLen);
  const len = Math.max(minViewLen, Math.min(newLen, maxViewLen));
  const start = Math.round(anchorIdx - frac * len);
  setViewport(start, len);
}

cvs.addEventListener(
  "wheel",
  (e) => {
    if (!fullData) return;
    e.preventDefault();
    const base = 1.3;
    const speed = e.ctrlKey ? 1.8 : e.shiftKey ? 1.45 : base;
    const zoomOut = e.deltaY > 0;
    const factor = zoomOut ? speed : 1 / speed;
    setZoomFromAnchor(Math.round(viewLen * factor), e.offsetX * DPR);
  },
  { passive: false }
);

cvs.addEventListener("mousedown", (e) => {
  if (!fullData) return;
  isDragging = true;
  dragStartX = e.clientX;
  dragStartView = viewStart;
});
window.addEventListener("mouseup", () => {
  isDragging = false;
});
window.addEventListener("mousemove", (e) => {
  if (!isDragging || !fullData) return;
  const dxPx = (e.clientX - dragStartX) * DPR;
  const samplesPerPx = viewLen / Math.max(1, W - 16 * DPR);
  const delta = Math.round(dxPx * samplesPerPx);
  setViewport(dragStartView - delta, viewLen);
});

if (xScroll) {
  xScroll.addEventListener("input", (e) => {
    if (!fullData) return;
    const val = Number(e.target.value);
    const maxStart = fullData.length - viewLen;
    const start = Math.round((val / 1000) * maxStart);
    setViewport(start, viewLen);
  });
}

if (zoomCtrl) {
  zoomCtrl.addEventListener("input", (e) => {
    if (!fullData) return;
    const v = Number(e.target.value);
    const len = zoomValueToLen(v);
    setZoomFromAnchor(len, (W * DPR) / 2);
  });
}

window.addEventListener("keydown", (e) => {
  if (!fullData) return;
  if (e.key === "+" || e.key === "=") {
    setZoomFromAnchor(Math.round(viewLen / 1.3), (W * DPR) / 2);
  } else if (e.key === "-" || e.key === "_") {
    setZoomFromAnchor(Math.round(viewLen * 1.3), (W * DPR) / 2);
  }
});

async function handleBuffer(buf, nameLabel) {
  fsCur = parseFloat(fsIn.value) || 500;

  minViewLen = Math.max(
    MIN_SAMPLES_ABS,
    Math.round(fsCur * MIN_WINDOW_SECONDS)
  );

  const raw = parseBuffer(buf);

  let lo = Infinity,
    hi = -Infinity;
  for (let i = 0; i < raw.length; i++) {
    const v = raw[i];
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  yMinG = lo;
  yMaxG = hi;

  fullData = raw;
  maxViewLen = fullData.length;

  const desiredLen = Math.min(
    maxViewLen,
    Math.max(minViewLen, Math.round(fsCur * 10))
  );
  setViewport(0, desiredLen);

  if (zoomCtrl) zoomCtrl.value = String(lenToZoomValue(viewLen));

  meta.textContent = nameLabel;
  stat.textContent = `total pts ${fullData.length}`;
}

async function loadAndPlot() {
  const name = sel.value;
  if (!name || name.startsWith("(")) return;

  meta.textContent = "Baixando…";
  stat.textContent = "";

  const t0 = performance.now();
  const r = await fetch(`/api/file?name=${encodeURIComponent(name)}`);
  if (!r.ok) {
    meta.textContent = "Erro ao baixar arquivo";
    return;
  }
  const buf = await r.arrayBuffer();
  const t1 = performance.now();

  await handleBuffer(buf, `${name} — ${prettyBytes(buf.byteLength)}`);
  const t2 = performance.now();
  stat.textContent = `download ${(t1 - t0).toFixed(0)} ms · parse ${(
    t2 - t1
  ).toFixed(0)} ms · total pts ${fullData.length}`;
}
btn.addEventListener("click", loadAndPlot);

// --- (opcional) entrada local e drag&drop, se existir no HTML ---
const btnLocal = document.getElementById("btnLocal");
const localInput = document.getElementById("localFile");
btnLocal?.addEventListener("click", () => localInput?.click());
localInput?.addEventListener("change", async (e) => {
  const file = e.target.files?.[0];
  if (!file) return;
  meta.textContent = `${file.name} — (local)`;
  stat.textContent = "lendo arquivo…";
  const t0 = performance.now();
  const buf = await file.arrayBuffer();
  const t1 = performance.now();
  await handleBuffer(
    buf,
    `${file.name} — (local) ${prettyBytes(buf.byteLength)}`
  );
  const t2 = performance.now();
  stat.textContent = `local read ${(t1 - t0).toFixed(0)} ms · parse ${(
    t2 - t1
  ).toFixed(0)} ms · total pts ${fullData.length}`;
});
cvs.addEventListener("dragover", (e) => e.preventDefault());
cvs.addEventListener("drop", async (e) => {
  e.preventDefault();
  const f = e.dataTransfer?.files?.[0];
  if (!f) return;
  meta.textContent = `${f.name} — (local)`;
  const buf = await f.arrayBuffer();
  await handleBuffer(buf, `${f.name} — (local) ${prettyBytes(buf.byteLength)}`);
});

(async () => {
  try {
    await listFiles();
  } catch {}
  scheduleRender();
})();
