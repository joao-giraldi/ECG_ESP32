#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>   // strcasecmp
#include <stdio.h>
#include <errno.h>

static const char *TAG = "http";

// LÊ DE /spiffs/teste (SPIFFS)
#define BASE_DIR "/sdcard/leitura"

static bool contains_dotdot(const char *s){ return strstr(s, "..") != NULL; }

// ---- UI: HTML + CSS + JS (mobile) ----
static const char INDEX_HTML[] =
"<!doctype html><html lang='pt-br'><head>"
"<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover' />"
"<title>ECG Viewer</title>"
"<style>"
":root{--bg:#0b0b0d;--fg:#f4f4f5;--muted:#9ca3af;--acc:#22d3ee}"
"*{box-sizing:border-box}html,body{height:100%}body{margin:0;background:var(--bg);color:var(--fg);font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial}"
".wrap{max-width:900px;margin:0 auto;padding:16px;display:grid;gap:12px}"
".card{background:#141418;border:1px solid #1f1f24;border-radius:16px;box-shadow:0 8px 20px rgba(0,0,0,.3)}"
".head{padding:14px 16px;border-bottom:1px solid #1f1f24;display:flex;gap:10px;align-items:center;flex-wrap:wrap}"
".title{font-weight:700;font-size:18px;margin-right:auto}"
".pill{font-size:12px;color:var(--muted)}"
".controls{padding:12px 16px;display:grid;gap:10px;grid-template-columns:1fr 1fr;align-items:center}"
".controls label{font-size:12px;color:var(--muted);display:block;margin-bottom:4px}"
".controls input,.controls select{width:100%;padding:10px 12px;background:#0f0f13;color:var(--fg);border:1px solid #24242b;border-radius:10px;outline:none}"
".controls .row{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
".btn{appearance:none;border:1px solid #2b2b33;background:#111116;color:var(--fg);padding:10px 14px;border-radius:10px}"
".btn:active{transform:scale(.98)}"
".canvasBox{padding:8px 8px 12px}"
"#chart{display:block;width:100%;height:42vh;background:#0e0e12;border:1px solid #1f1f24;border-radius:12px}"
".footer{padding:10px 16px;border-top:1px solid #1f1f24;display:flex;gap:12px;flex-wrap:wrap;font-size:12px;color:var(--muted)}"
"@media (min-width:640px){.controls{grid-template-columns:2fr 1fr 1fr 1fr}.controls .row{grid-template-columns:1fr 1fr 1fr}}"
"</style>"
"</head><body>"
"<div class='wrap'>"
"  <div class='card'>"
"    <div class='head'><div class='title'>ECG — Visualizador</div><div class='pill'>Acesse <b>192.168.4.1</b> conectado ao Wi-Fi do ESP</div></div>"
"    <div class='controls'>"
"      <div><label>Arquivo</label><select id='fileSel'></select></div>"
"      <div class='row'>"
"        <div><label>Fs (Hz)</label><input id='fs' type='number' inputmode='numeric' value='500' min='1'/></div>"
"        <div><label>Formato</label>"
"          <select id='fmt'>"
"            <option value='i16le' selected>Int16 LE</option>"
"            <option value='u16le'>Uint16 LE</option>"
"            <option value='i8'>Int8</option>"
"            <option value='u8'>Uint8</option>"
"          </select></div>"
"        <div><label>Ganho (LSB→mV)</label><input id='gain' type='number' step='0.01' value='1.0'/></div>"
"      </div>"
"      <button id='btnLoad' class='btn'>Carregar & Desenhar</button>"
"    </div>"
"    <div class='canvasBox'><canvas id='chart'></canvas></div>"
"    <div class='footer'><span id='meta'>–</span><span id='stat'></span></div>"
"  </div>"
"</div>"
"<script>"
"const sel=document.getElementById('fileSel');const fsIn=document.getElementById('fs');const fmtSel=document.getElementById('fmt');const gainIn=document.getElementById('gain');const meta=document.getElementById('meta');const stat=document.getElementById('stat');const btn=document.getElementById('btnLoad');const cvs=document.getElementById('chart');const ctx=cvs.getContext('2d');let W=0,H=0;const DPR=window.devicePixelRatio||1;"
"function resize(){const r=cvs.getBoundingClientRect();W=Math.floor(r.width*DPR);H=Math.floor(r.height*DPR);cvs.width=W;cvs.height=H;}"
"window.addEventListener('resize',()=>{resize();draw([],{fs:1,min:0,max:0});});"
"async function listFiles(){const r=await fetch('/api/files');const js=await r.json();sel.innerHTML='';js.forEach(f=>{const o=document.createElement('option');o.value=f.name;o.text=`${f.name} (${prettyBytes(f.size)})`;sel.appendChild(o);});if(js.length===0){const o=document.createElement('option');o.text='(nenhum .bin encontrado)';sel.appendChild(o);}return js;}"
"function prettyBytes(b){const u=['B','KB','MB','GB'];let i=0;while(b>=1024&&i<u.length-1){b/=1024;i++}return b.toFixed((i?1:0))+' '+u[i];}"
"function parseBuffer(buf){const fmt=fmtSel.value;const g=parseFloat(gainIn.value)||1;const dv=new DataView(buf);let N=buf.byteLength;let arr;"
"if(fmt==='i16le'){N>>=1;arr=new Float32Array(N);for(let i=0;i<N;i++){arr[i]=dv.getInt16(i*2,true)*g;}}"
"else if(fmt==='u16le'){N>>=1;arr=new Float32Array(N);for(let i=0;i<N;i++){arr[i]=dv.getUint16(i*2,true)*g;}}"
"else if(fmt==='i8'){arr=new Float32Array(N);for(let i=0;i<N;i++){arr[i]=(dv.getInt8(i))*g;}}"
"else {arr=new Float32Array(N);for(let i=0;i<N;i++){arr[i]=(dv.getUint8(i))*g;}}"
"return arr;}"
"function decimate(data,maxPts=6000){if(data.length<=maxPts) return data;const step=Math.ceil(data.length/maxPts);const out=new Float32Array(Math.ceil(data.length/step));let j=0;for(let i=0;i<data.length;i+=step) out[j++]=data[i];return out;}"
"function draw(data,{fs,min,max}){resize();ctx.clearRect(0,0,W,H);ctx.fillStyle='#0b0b0d';ctx.fillRect(0,0,W,H);ctx.strokeStyle='rgba(255,255,255,.05)';ctx.lineWidth=1;const gx=10,gy=8;for(let i=1;i<gx;i++){const x=i*W/gx;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,H);ctx.stroke();}for(let i=1;i<gy;i++){const y=i*H/gy;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(W,y);ctx.stroke();}if(!data||data.length===0){ctx.fillStyle='#9ca3af';ctx.font=`${14*DPR}px system-ui`;ctx.fillText('Selecione um arquivo e clique em Carregar',16*DPR,28*DPR);return;}const N=data.length;const t=N/(fs||1);const pad=8*DPR;const yMin=min,yMax=max,k=(H-2*pad)/(yMax-yMin||1);ctx.strokeStyle='#22d3ee';ctx.lineWidth=Math.max(1,1*DPR);ctx.beginPath();for(let i=0;i<N;i++){const x=pad+(i*(W-2*pad))/(N-1);const y=H-pad-(data[i]-yMin)*k;if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);}ctx.stroke();ctx.fillStyle='#9ca3af';ctx.font=`${12*DPR}px system-ui`;ctx.fillText(`${t.toFixed(2)} s @ ${fs} Hz — ${N} pts`,pad,H-6*DPR);}"
"async function loadAndPlot(){const name=sel.value;if(!name||name.startsWith('(')) return;meta.textContent='Baixando…';stat.textContent='';const t0=performance.now();const r=await fetch(`/api/file?name=${encodeURIComponent(name)}`);if(!r.ok){meta.textContent='Erro ao baixar arquivo';return;}const buf=await r.arrayBuffer();const t1=performance.now();let data=parseBuffer(buf);const fs=parseFloat(fsIn.value)||500;let min=Infinity,max=-Infinity;for(let i=0;i<data.length;i++){const v=data[i];if(v<min)min=v;if(v>max)max=v;}const dec=decimate(data);const t2=performance.now();draw(dec,{fs,min,max});meta.textContent=`${name} — ${prettyBytes(buf.byteLength)}`;stat.textContent=`download ${(t1-t0).toFixed(0)} ms · parse/plot ${(t2-t1).toFixed(0)} ms · pts ${data.length}→${dec.length}`;}"
"btn.addEventListener('click', loadAndPlot);(async()=>{await listFiles();draw([],{fs:1,min:0,max:0});})();"
"</script></body></html>";

// ---- Handlers ----
static esp_err_t root_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t api_files_handler(httpd_req_t *req){
    DIR *dir = opendir(BASE_DIR);
    httpd_resp_set_type(req, "application/json");
    if (!dir) return httpd_resp_sendstr(req, "[]");

    httpd_resp_sendstr_chunk(req, "[");
    struct dirent *ent;
    bool first = true;
    char path[512];
    struct stat st;

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (!name || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        size_t nlen = strlen(name);
        if (nlen < 4 || strcasecmp(name + nlen - 4, ".bin") != 0) continue;

        size_t blen = strlen(BASE_DIR);
        if (blen + 1 + nlen >= sizeof(path)) continue;
        memcpy(path, BASE_DIR, blen);
        path[blen] = '/';
        memcpy(path + blen + 1, name, nlen);
        path[blen + 1 + nlen] = '\0';

        if (stat(path, &st) != 0) continue;

        if (!first) httpd_resp_sendstr_chunk(req, ",");
        httpd_resp_sendstr_chunk(req, "{\"name\":\"");
        httpd_resp_sendstr_chunk(req, name);
        httpd_resp_sendstr_chunk(req, "\",\"size\":");

        char sizebuf[32];
        int m = snprintf(sizebuf, sizeof(sizebuf), "%ld", (long)st.st_size);
        if (m < 0) strcpy(sizebuf, "0");
        httpd_resp_sendstr_chunk(req, sizebuf);
        httpd_resp_sendstr_chunk(req, "}");
        first = false;
    }
    closedir(dir);
    httpd_resp_sendstr_chunk(req, "]");
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t api_file_handler(httpd_req_t *req){
    char name[128]={0}, query[128]={0};
    int qlen = httpd_req_get_url_query_len(req);
    if (qlen < 0 || qlen >= (int)sizeof(query))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "query too long");
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing query");
    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing name");
    if (contains_dotdot(name))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid name");

    char path[512];
    size_t blen = strlen(BASE_DIR), nlen = strlen(name);
    if (blen + 1 + nlen >= sizeof(path))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "name too long");

    memcpy(path, BASE_DIR, blen); path[blen] = '/';
    memcpy(path + blen + 1, name, nlen);
    path[blen + 1 + nlen] = '\0';

    FILE *f = fopen(path, "rb");
    if (!f) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    char buf[2048]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
            fclose(f); httpd_resp_send_chunk(req, NULL, 0); return ESP_FAIL;
        }
    }
    fclose(f);
    return httpd_resp_send_chunk(req, NULL, 0);
}

httpd_handle_t start_webserver(void){
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.server_port = 80;
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return NULL;
    }
    httpd_uri_t root  = { .uri="/",         .method=HTTP_GET, .handler=root_handler  };
    httpd_uri_t files = { .uri="/api/files", .method=HTTP_GET, .handler=api_files_handler };
    httpd_uri_t file  = { .uri="/api/file",  .method=HTTP_GET, .handler=api_file_handler  };
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &files);
    httpd_register_uri_handler(server, &file);
    ESP_LOGI(TAG, "HTTP pronto em http://192.168.4.1");
    return server;
}

void stop_webserver(httpd_handle_t server){
    if (server) httpd_stop(server);
}
