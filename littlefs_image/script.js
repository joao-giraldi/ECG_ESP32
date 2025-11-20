/* =========================================================================
 * CONFIG FIXA
 * ========================================================================= */
const FS_CONST = 100;
const FMT_CONST = "i16le";
const GAIN_CONST = 1.0;

/* =========================================================================
 * DOM
 * ========================================================================= */
const sel = document.getElementById("fileSel");
const btn = document.getElementById("btnLoad");
const btnDownload = document.getElementById("btnDownload");
const meta = document.getElementById("meta");
const stat = document.getElementById("stat");
const cvs = document.getElementById("chart");
const ctx = cvs.getContext("2d");
const xScroll = document.getElementById("xScroll");
const zoomCtrl = document.getElementById("zoomCtrl");

const infoDur = document.getElementById("infoDur");
const infoBpm = document.getElementById("infoBpm");
const fsInput = document.getElementById("fs");

/* =========================================================================
 * STATE
 * ========================================================================= */
const DPR = window.devicePixelRatio || 1;
let W = 0,
  H = 0;

let fullData = null;
let fsCur = FS_CONST;

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

let currentLoadedName = null;

/* =========================================================================
 * UTILS
 * ========================================================================= */
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
window.addEventListener("resize", () => {
  if (syncCanvasSizeIfNeeded()) scheduleRender();
});
syncCanvasSizeIfNeeded();

function prettyBytes(b) {
  const u = ["B", "KB", "MB", "GB"];
  let i = 0;
  while (b >= 1024 && i < u.length - 1) {
    b /= 1024;
    i++;
  }
  return b.toFixed(i ? 1 : 0) + " " + u[i];
}

/* ====== DOWNLOAD ====== */
// Agora gera CSV (um valor por linha)
function signalToCsv(
  values,
  { decimals = 0, newline = "\n", asIntegers = true } = {}
) {
  const n = values.length;
  const out = new Array(n);
  if (asIntegers) {
    for (let i = 0; i < n; i++) out[i] = Math.round(values[i]).toString();
  } else {
    const fmt = new Intl.NumberFormat("en-US", {
      maximumFractionDigits: decimals,
      minimumFractionDigits: 0,
    });
    for (let i = 0; i < n; i++) out[i] = fmt.format(values[i]);
  }
  return out.join(newline) + newline;
}
function downloadCsvFile(text, filename) {
  const blob = new Blob([text], { type: "text/csv;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.rel = "noopener";
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}
function makeSiblingCsv(name) {
  if (!name) return "sinal.csv";
  const i = name.lastIndexOf(".");
  const base = i > 0 ? name.slice(0, i) : name;
  return `${base}.csv`;
}

/* =========================================================================
 * ARQUIVOS DO ESP
 * ========================================================================= */
async function listFiles() {
  try {
    const r = await fetch("/api/files");
    if (!r.ok) throw new Error("HTTP " + r.status);
    const js = await r.json();

    sel.innerHTML = "";
    if (!Array.isArray(js) || js.length === 0) {
      const o = document.createElement("option");
      o.text = "(nenhum .bin encontrado)";
      o.value = "";
      sel.appendChild(o);
      btn.disabled = true;
      btnDownload.disabled = true;
      return [];
    }

    for (const f of js) {
      const o = document.createElement("option");
      o.value = f.name;
      o.text = `${f.name} (${prettyBytes(f.size)})`;
      sel.appendChild(o);
    }
    btn.disabled = false;
    btnDownload.disabled = !fullData;
    return js;
  } catch {
    sel.innerHTML = "";
    const o = document.createElement("option");
    o.text = "(falha ao listar)";
    o.value = "";
    sel.appendChild(o);
    btn.disabled = true;
    btnDownload.disabled = true;
    return [];
  }
}

/* =========================================================================
 * PARSE DO BIN
 * ========================================================================= */
function parseBuffer(buf) {
  const dv = new DataView(buf);
  let N = buf.byteLength;
  let arr;

  if (FMT_CONST === "i16le") {
    N >>= 1;
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getInt16(i * 2, true) * GAIN_CONST;
  } else if (FMT_CONST === "u16le") {
    N >>= 1;
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getUint16(i * 2, true) * GAIN_CONST;
  } else if (FMT_CONST === "i8") {
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getInt8(i) * GAIN_CONST;
  } else {
    arr = new Float32Array(N);
    for (let i = 0; i < N; i++) arr[i] = dv.getUint8(i) * GAIN_CONST;
  }
  return arr;
}

/* =========================================================================
 * DOWNSAMPLE P/ DESENHO
 * ========================================================================= */
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

/* =========================================================================
 * DESENHO
 * ========================================================================= */
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

/* =========================================================================
 * ZOOM/PAN
 * ========================================================================= */
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
  if (zoomCtrl) zoomCtrl.value = String(lenToZoomValue(viewLen));

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

xScroll?.addEventListener("input", (e) => {
  if (!fullData) return;
  const val = Number(e.target.value);
  const maxStart = fullData.length - viewLen;
  const start = Math.round((val / 1000) * maxStart);
  setViewport(start, viewLen);
});
zoomCtrl?.addEventListener("input", (e) => {
  if (!fullData) return;
  const v = Number(e.target.value);
  const len = zoomValueToLen(v);
  setZoomFromAnchor(len, (W * DPR) / 2);
});
window.addEventListener("keydown", (e) => {
  if (!fullData) return;
  if (e.key === "+" || e.key === "=")
    setZoomFromAnchor(Math.round(viewLen / 1.3), (W * DPR) / 2);
  else if (e.key === "-" || e.key === "_")
    setZoomFromAnchor(Math.round(viewLen * 1.3), (W * DPR) / 2);
});

/* =========================================================================
 * BPM (simplificado)
 * ========================================================================= */
function movingAverage(arr, win) {
  const n = arr.length,
    w = Math.max(1, win);
  const out = new Float32Array(n);
  let acc = 0;
  for (let i = 0; i < n; i++) {
    acc += arr[i];
    if (i >= w) acc -= arr[i - w];
    out[i] = acc / Math.min(i + 1, w);
  }
  return out;
}
function bandpassMAvg(x, fs, lowMs = 80, highMs = 800) {
  const long = Math.max(1, Math.floor((highMs / 1000) * fs));
  const short = Math.max(1, Math.floor((lowMs / 1000) * fs));
  const base = movingAverage(x, long);
  const hp = new Float32Array(x.length);
  for (let i = 0; i < x.length; i++) hp[i] = x[i] - base[i];
  return movingAverage(hp, short);
}
function detectRPeaks(samples, fs, { thrQuantile = 0.9, refrMs = 300 } = {}) {
  let mean = 0,
    varAcc = 0;
  for (let i = 0; i < samples.length; i++) mean += samples[i];
  mean /= samples.length || 1;
  for (let i = 0; i < samples.length; i++) varAcc += (samples[i] - mean) ** 2;
  const std = Math.sqrt(varAcc / (samples.length || 1)) || 1;

  const x = new Float32Array(samples.length);
  for (let i = 0; i < samples.length; i++) x[i] = (samples[i] - mean) / std;

  const xf = bandpassMAvg(x, fs, 80, 800);
  const z = new Float32Array(xf.length);
  for (let i = 0; i < xf.length; i++) z[i] = xf[i] * xf[i];

  const sorted = Array.from(z).sort((a, b) => a - b);
  const k = Math.floor(thrQuantile * (sorted.length - 1));
  const thr = sorted[Math.max(0, Math.min(sorted.length - 1, k))];

  const rawPeaks = [];
  for (let i = 1; i < z.length - 1; i++) {
    if (z[i] > thr && z[i] > z[i - 1] && z[i] > z[i + 1]) rawPeaks.push(i);
  }

  const refr = Math.floor((refrMs / 1000) * fs);
  const peaks = [];
  for (const p of rawPeaks) {
    if (peaks.length === 0 || p - peaks[peaks.length - 1] >= refr)
      peaks.push(p);
  }
  return { peaks, z, thr };
}
function computeBpmFromPeaks(peaks, fs) {
  if (!peaks || peaks.length < 2)
    return { bpm: NaN, rrMean: NaN, beats: peaks.length };
  const rr = [];
  for (let i = 1; i < peaks.length; i++)
    rr.push((peaks[i] - peaks[i - 1]) / fs);
  const rrFiltered = rr.filter((r) => r >= 60 / 220 && r <= 60 / 30);
  if (!rrFiltered.length) return { bpm: NaN, rrMean: NaN, beats: peaks.length };
  const rrMean = rrFiltered.reduce((a, b) => a + b, 0) / rrFiltered.length;
  return { bpm: 60 / rrMean, rrMean, beats: peaks.length };
}
function updateMetricsUI() {
  if (!fullData) return;
  const totalSamples = fullData.length;
  const durationSec = totalSamples / (fsCur || 1); // <-- duração em segundos

  const { peaks } = detectRPeaks(fullData, fsCur, {
    thrQuantile: 0.9,
    refrMs: 300,
  });
  const { bpm, beats } = computeBpmFromPeaks(peaks, fsCur);

  // Agora exibindo em segundos
  if (infoDur) infoDur.textContent = `${durationSec.toFixed(2)} s`;
  if (infoBpm)
    infoBpm.textContent = isFinite(bpm) ? `${bpm.toFixed(0)} BPM` : "n/d";
}

/* =========================================================================
 * PIPELINE DE CARREGAMENTO
 * ========================================================================= */
async function handleBuffer(buf, nameLabel) {
  fsCur = Math.max(1, Number(fsInput?.value) || FS_CONST);
  minViewLen = Math.max(
    MIN_SAMPLES_ABS,
    Math.round(fsCur * MIN_WINDOW_SECONDS)
  );

  const raw = parseBuffer(buf);

  let trimmed = raw;
  const ZERO_THRESHOLD = 5;
  const MIN_RUN_SEC = 0.25;
  const MIN_RUN_SAMPLES = Math.round((fsCur || 100) * MIN_RUN_SEC);
  let zeroRun = 0;
  let lastValidIdx = raw.length - 1;

  for (let i = raw.length - 1; i >= 0; i--) {
    const v = raw[i];
    if (Math.abs(v) <= ZERO_THRESHOLD) zeroRun++;
    else {
      if (zeroRun >= MIN_RUN_SAMPLES) {
        lastValidIdx = i;
        break;
      }
      zeroRun = 0;
    }
  }
  if (lastValidIdx < raw.length - 1)
    trimmed = raw.subarray(0, lastValidIdx + 1);

  fullData = trimmed;
  maxViewLen = fullData.length;

  let lo = Infinity,
    hi = -Infinity;
  for (let i = 0; i < fullData.length; i++) {
    const v = fullData[i];
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  yMinG = lo;
  yMaxG = hi;

  const desiredLen = Math.min(
    maxViewLen,
    Math.max(minViewLen, Math.round(fsCur * 10))
  );
  setViewport(0, desiredLen);

  updateMetricsUI();
  meta.textContent = nameLabel;
  stat.textContent = `total pts ${fullData.length}`;

  if (btnDownload) btnDownload.disabled = !fullData || fullData.length === 0;
}

fsInput?.addEventListener("change", () => {
  const v = Math.max(1, Number(fsInput.value) || FS_CONST);
  fsInput.value = String(v);
  if (!fullData) return;

  fsCur = v;
  minViewLen = Math.max(
    MIN_SAMPLES_ABS,
    Math.round(fsCur * MIN_WINDOW_SECONDS)
  );
  if (viewLen < minViewLen) setViewport(viewStart, minViewLen);

  updateMetricsUI();
  scheduleRender();
  stat.textContent = `total pts ${fullData.length} · Fs = ${fsCur} Hz`;
});

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

  currentLoadedName = name;
}
btn?.addEventListener("click", loadAndPlot);

/* =========================================================================
 * DOWNLOAD: sempre o arquivo inteiro
 * ========================================================================= */
btnDownload?.addEventListener("click", () => {
  if (!fullData || !fullData.length) return;

  const csv = signalToCsv(fullData, {
    asIntegers: true,
    decimals: 0,
    newline: "\n",
  });

  const baseName = currentLoadedName || sel?.value || "sinal";
  const filename = makeSiblingCsv(baseName);
  downloadCsvFile(csv, filename);
});

/* =========================================================================
 * DRAG & DROP LOCAL
 * ========================================================================= */
let dragCounter = 0;
cvs.addEventListener("dragenter", (e) => {
  e.preventDefault();
  dragCounter++;
  cvs.classList.add("drop-ready");
});
cvs.addEventListener("dragleave", (e) => {
  e.preventDefault();
  dragCounter = Math.max(0, dragCounter - 1);
  if (dragCounter === 0) cvs.classList.remove("drop-ready");
});
cvs.addEventListener("dragover", (e) => e.preventDefault());
cvs.addEventListener("drop", async (e) => {
  e.preventDefault();
  dragCounter = 0;
  cvs.classList.remove("drop-ready");
  const f = e.dataTransfer?.files?.[0];
  if (!f) return;
  if (!f.name.toLowerCase().endsWith(".bin")) {
    meta.textContent = "Arquivo inválido: selecione um .bin";
    return;
  }
  meta.textContent = `${f.name} — (local)`;
  const buf = await f.arrayBuffer();
  await handleBuffer(buf, `${f.name} — (local) ${prettyBytes(buf.byteLength)}`);

  currentLoadedName = f.name;
});

/* =========================================================================
 * INIT
 * ========================================================================= */
(async () => {
  try {
    await listFiles();
  } catch {}
  if (btnDownload) btnDownload.disabled = true;
  scheduleRender();
})();
