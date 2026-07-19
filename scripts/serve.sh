#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${1:-8000}"

if [[ ! "$PORT" =~ ^[0-9]+$ ]]; then
  printf 'error: port must be an integer from 1 to 65535, got %q\n' "$PORT" >&2
  exit 2
fi
PORT_NUMBER=$((10#$PORT))
if ((PORT_NUMBER < 1 || PORT_NUMBER > 65535)); then
  printf 'error: port must be an integer from 1 to 65535, got %q\n' "$PORT" >&2
  exit 2
fi
PORT="$PORT_NUMBER"

printf 'Serving NetForge at http://127.0.0.1:%s/ (Ctrl+C to stop)\n' "$PORT"

if ! command -v node >/dev/null 2>&1; then
  if command -v python3 >/dev/null 2>&1; then
    printf 'warning: node is unavailable; Markdown pages will be shown as source text.\n' >&2
    exec python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$ROOT_DIR"
  fi
  printf 'error: neither node nor python3 is available; run scripts/bootstrap.sh or install one manually.\n' >&2
  exit 1
fi

exec node - "$ROOT_DIR" "$PORT" <<'NODE'
const http = require("node:http");
const fs = require("node:fs");
const path = require("node:path");
const { URL } = require("node:url");

const root = fs.realpathSync(path.resolve(process.argv[2]));
const port = Number(process.argv[3]);
const host = "127.0.0.1";
const contentTypes = new Map([
  [".css", "text/css; charset=utf-8"],
  [".gif", "image/gif"],
  [".html", "text/html; charset=utf-8"],
  [".ico", "image/x-icon"],
  [".jpeg", "image/jpeg"],
  [".jpg", "image/jpeg"],
  [".js", "text/javascript; charset=utf-8"],
  [".json", "application/json; charset=utf-8"],
  [".mjs", "text/javascript; charset=utf-8"],
  [".pdf", "application/pdf"],
  [".png", "image/png"],
  [".svg", "image/svg+xml; charset=utf-8"],
  [".txt", "text/plain; charset=utf-8"],
  [".webp", "image/webp"],
]);

function escapeHtml(value) {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function headingSlug(value) {
  return value
    .toLowerCase()
    .replace(/<[^>]*>/g, "")
    .replace(/[`*_~]/g, "")
    .replace(/[^\p{L}\p{N}\s-]/gu, "")
    .trim()
    .replace(/\s/g, "-");
}

function inlineMarkdown(value) {
  const code = [];
  let text = value.replace(/`([^`]+)`/g, (_, body) => {
    const index = code.push(`<code>${escapeHtml(body)}</code>`) - 1;
    return `\u0000CODE${index}\u0000`;
  });
  text = escapeHtml(text)
    .replace(/\[([^\]]+)\]\(([^\s)]+)(?:\s+"[^"]*")?\)/g, '<a href="$2">$1</a>')
    .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
    .replace(/__([^_]+)__/g, "<strong>$1</strong>")
    .replace(/(?<!\*)\*([^*]+)\*(?!\*)/g, "<em>$1</em>")
    .replace(/\u0000CODE(\d+)\u0000/g, (_, index) => code[Number(index)]);
  return text;
}

function tableCells(line) {
  return line.trim().replace(/^\||\|$/g, "").split("|").map((cell) => cell.trim());
}

function renderMarkdown(source, title) {
  const lines = source.replaceAll("\r\n", "\n").split("\n");
  const output = [];
  let index = 0;
  while (index < lines.length) {
    const line = lines[index];
    if (line.trim() === "") {
      index += 1;
      continue;
    }
    const fence = line.match(/^```([^\s`]*)\s*$/);
    if (fence) {
      const body = [];
      index += 1;
      while (index < lines.length && !/^```\s*$/.test(lines[index])) body.push(lines[index++]);
      if (index < lines.length) index += 1;
      output.push(`<pre><code class="language-${escapeHtml(fence[1] || "text")}">${escapeHtml(body.join("\n"))}</code></pre>`);
      continue;
    }
    const heading = line.match(/^(#{1,6})\s+(.+)$/);
    if (heading) {
      const level = heading[1].length;
      const text = heading[2].trim();
      output.push(`<h${level} id="${headingSlug(text)}">${inlineMarkdown(text)}</h${level}>`);
      index += 1;
      continue;
    }
    if (line.includes("|") && index + 1 < lines.length && /^\s*\|?\s*:?-{3,}/.test(lines[index + 1])) {
      const headers = tableCells(line);
      index += 2;
      const rows = [];
      while (index < lines.length && lines[index].includes("|") && lines[index].trim() !== "") {
        rows.push(tableCells(lines[index++]));
      }
      output.push(`<div class="table-scroll"><table><thead><tr>${headers.map((cell) => `<th>${inlineMarkdown(cell)}</th>`).join("")}</tr></thead><tbody>${rows.map((row) => `<tr>${row.map((cell) => `<td>${inlineMarkdown(cell)}</td>`).join("")}</tr>`).join("")}</tbody></table></div>`);
      continue;
    }
    const list = line.match(/^\s*(?:([-+*])|(\d+)\.)\s+(.+)$/);
    if (list) {
      const ordered = Boolean(list[2]);
      const items = [];
      while (index < lines.length) {
        const item = lines[index].match(/^\s*(?:([-+*])|(\d+)\.)\s+(.+)$/);
        if (!item || Boolean(item[2]) !== ordered) break;
        items.push(item[3]);
        index += 1;
      }
      const tag = ordered ? "ol" : "ul";
      output.push(`<${tag}>${items.map((item) => `<li>${inlineMarkdown(item)}</li>`).join("")}</${tag}>`);
      continue;
    }
    if (/^>\s?/.test(line)) {
      const quoted = [];
      while (index < lines.length && /^>\s?/.test(lines[index])) quoted.push(lines[index++].replace(/^>\s?/, ""));
      output.push(`<blockquote>${inlineMarkdown(quoted.join(" "))}</blockquote>`);
      continue;
    }
    if (/^<(?:details|\/details|summary)\b/i.test(line)) {
      output.push(line);
      index += 1;
      continue;
    }
    if (/^---+$/.test(line.trim())) {
      output.push("<hr>");
      index += 1;
      continue;
    }
    const paragraph = [line.trim()];
    index += 1;
    while (index < lines.length && lines[index].trim() !== "" &&
           !/^(?:#{1,6}\s|```|\s*(?:[-+*]|\d+\.)\s+|>\s?|<(?:details|\/details|summary)\b)/i.test(lines[index])) {
      if (lines[index].includes("|") && index + 1 < lines.length && /^\s*\|?\s*:?-{3,}/.test(lines[index + 1])) break;
      paragraph.push(lines[index].trim());
      index += 1;
    }
    output.push(`<p>${inlineMarkdown(paragraph.join(" "))}</p>`);
  }
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>${escapeHtml(title)} · NetForge</title><link rel="stylesheet" href="/assets/course.css"></head>
<body><main class="shell lesson-shell"><header class="masthead"><a class="brand" href="/index.html">NetForge</a><span class="quiet">Rendered from the canonical Markdown file</span></header><article class="markdown-document">${output.join("\n")}</article></main></body></html>`;
}

function send(res, status, message, extraHeaders = {}) {
  const body = `${message}\n`;
  res.writeHead(status, {
    "Cache-Control": "no-store",
    "Content-Length": Buffer.byteLength(body),
    "Content-Type": "text/plain; charset=utf-8",
    "X-Content-Type-Options": "nosniff",
    ...extraHeaders,
  });
  res.end(body);
}

function isInsideRoot(candidate) {
  return candidate === root || candidate.startsWith(`${root}${path.sep}`);
}

const server = http.createServer((req, res) => {
  if (req.method !== "GET" && req.method !== "HEAD") {
    send(res, 405, "Method not allowed", { Allow: "GET, HEAD" });
    return;
  }

  let pathname;
  try {
    pathname = decodeURIComponent(new URL(req.url, `http://${host}:${port}`).pathname);
  } catch {
    send(res, 400, "Bad request");
    return;
  }

  const relativePath = pathname.replace(/^\/+/, "");
  let candidate = path.resolve(root, relativePath);
  if (!isInsideRoot(candidate)) {
    send(res, 403, "Forbidden");
    return;
  }

  fs.stat(candidate, (statError, initialStat) => {
    if (statError) {
      send(res, statError.code === "ENOENT" ? 404 : 500, statError.code === "ENOENT" ? "Not found" : "Server error");
      return;
    }
    if (initialStat.isDirectory()) candidate = path.join(candidate, "index.html");

    fs.realpath(candidate, (realpathError, realPath) => {
      if (realpathError) {
        send(res, realpathError.code === "ENOENT" ? 404 : 500, realpathError.code === "ENOENT" ? "Not found" : "Server error");
        return;
      }
      if (!isInsideRoot(realPath)) {
        send(res, 403, "Forbidden");
        return;
      }

      fs.stat(realPath, (fileError, fileStat) => {
        if (fileError || !fileStat.isFile()) {
          send(res, fileError?.code === "ENOENT" ? 404 : 500, fileError?.code === "ENOENT" ? "Not found" : "Server error");
          return;
        }

        if (path.extname(realPath).toLowerCase() === ".md") {
          fs.readFile(realPath, "utf8", (readError, markdown) => {
            if (readError) {
              send(res, 500, "Server error");
              return;
            }
            const body = renderMarkdown(markdown, path.basename(realPath, ".md"));
            res.writeHead(200, {
              "Cache-Control": "no-store",
              "Content-Length": Buffer.byteLength(body),
              "Content-Type": "text/html; charset=utf-8",
              "X-Content-Type-Options": "nosniff",
            });
            if (req.method === "HEAD") res.end();
            else res.end(body);
          });
          return;
        }

        res.writeHead(200, {
          "Cache-Control": "no-store",
          "Content-Length": fileStat.size,
          "Content-Type": contentTypes.get(path.extname(realPath).toLowerCase()) || "application/octet-stream",
          "X-Content-Type-Options": "nosniff",
        });
        if (req.method === "HEAD") {
          res.end();
          return;
        }
        const stream = fs.createReadStream(realPath);
        stream.on("error", () => {
          if (!res.headersSent) send(res, 500, "Server error");
          else res.destroy();
        });
        stream.pipe(res);
      });
    });
  });
});

server.on("error", (error) => {
  console.error(`Static server failed: ${error.message}`);
  process.exitCode = 1;
});
server.listen(port, host);
NODE
