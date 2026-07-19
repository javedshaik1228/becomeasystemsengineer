import { cp, mkdir, readFile, readdir, rm, stat, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const outputRoot = path.join(root, ".pages");
const ignored = new Set([".git", ".git-publish", ".pages", "build", "node_modules"]);

function escapeHtml(value) {
  return value.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;").replaceAll("'", "&#39;");
}

function slug(value) {
  return value.toLowerCase().replace(/<[^>]*>/g, "").replace(/[`*_~]/g, "")
    .replace(/[^\p{L}\p{N}\s-]/gu, "").trim().replace(/\s/g, "-");
}

function inline(value) {
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

function cells(line) { return line.trim().replace(/^\||\|$/g, "").split("|").map((cell) => cell.trim()); }

function renderBody(source) {
  const lines = source.replaceAll("\r\n", "\n").split("\n");
  const output = [];
  let index = 0;
  while (index < lines.length) {
    const line = lines[index];
    if (!line.trim()) { index += 1; continue; }
    const fence = line.match(/^```([^\s`]*)\s*$/);
    if (fence) {
      const body = []; index += 1;
      while (index < lines.length && !/^```\s*$/.test(lines[index])) body.push(lines[index++]);
      if (index < lines.length) index += 1;
      output.push(`<pre><code class="language-${escapeHtml(fence[1] || "text")}">${escapeHtml(body.join("\n"))}</code></pre>`);
      continue;
    }
    const heading = line.match(/^(#{1,6})\s+(.+)$/);
    if (heading) { const level = heading[1].length; output.push(`<h${level} id="${slug(heading[2].trim())}">${inline(heading[2].trim())}</h${level}>`); index += 1; continue; }
    if (line.includes("|") && index + 1 < lines.length && /^\s*\|?\s*:?-{3,}/.test(lines[index + 1])) {
      const headers = cells(line); index += 2; const rows = [];
      while (index < lines.length && lines[index].includes("|") && lines[index].trim()) rows.push(cells(lines[index++]));
      output.push(`<div class="table-scroll"><table><thead><tr>${headers.map((cell) => `<th>${inline(cell)}</th>`).join("")}</tr></thead><tbody>${rows.map((row) => `<tr>${row.map((cell) => `<td>${inline(cell)}</td>`).join("")}</tr>`).join("")}</tbody></table></div>`);
      continue;
    }
    const list = line.match(/^\s*(?:([-+*])|(\d+)\.)\s+(.+)$/);
    if (list) {
      const ordered = Boolean(list[2]); const items = [];
      while (index < lines.length) { const item = lines[index].match(/^\s*(?:([-+*])|(\d+)\.)\s+(.+)$/); if (!item || Boolean(item[2]) !== ordered) break; items.push(item[3]); index += 1; }
      output.push(`<${ordered ? "ol" : "ul"}>${items.map((item) => `<li>${inline(item)}</li>`).join("")}</${ordered ? "ol" : "ul"}>`);
      continue;
    }
    if (/^>\s?/.test(line)) { const quoted = []; while (index < lines.length && /^>\s?/.test(lines[index])) quoted.push(lines[index++].replace(/^>\s?/, "")); output.push(`<blockquote>${inline(quoted.join(" "))}</blockquote>`); continue; }
    if (/^<(?:details|\/details|summary)\b/i.test(line)) { output.push(line); index += 1; continue; }
    if (/^---+$/.test(line.trim())) { output.push("<hr>"); index += 1; continue; }
    const paragraph = [line.trim()]; index += 1;
    while (index < lines.length && lines[index].trim() && !/^(?:#{1,6}\s|```|\s*(?:[-+*]|\d+\.)\s+|>\s?|<(?:details|\/details|summary)\b)/i.test(lines[index])) { paragraph.push(lines[index].trim()); index += 1; }
    output.push(`<p>${inline(paragraph.join(" "))}</p>`);
  }
  return output.join("\n");
}

function rewriteMarkdownLinks(html) {
  return html.replace(/(href\s*=\s*["'][^"']*?)\.md(#[^"']*)?(["'])/gi, "$1.html$2$3");
}

function pageShell(body, title, relativeDirectory) {
  const prefix = relativeDirectory ? "../".repeat(relativeDirectory.split("/").length) : "";
  return `<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>${escapeHtml(title)} · NetForge</title><link rel="stylesheet" href="${prefix}assets/course.css"></head><body><main class="shell lesson-shell"><header class="masthead"><a class="brand" href="${prefix}index.html">NetForge</a><span class="quiet">Reference page</span></header><article class="markdown-document">${body}</article></main></body></html>`;
}

async function walk(directory, relative = "") {
  const entries = await readdir(directory, { withFileTypes: true });
  const result = [];
  for (const entry of entries) {
    if (entry.isDirectory() && ignored.has(entry.name)) continue;
    const relativePath = path.posix.join(relative, entry.name);
    const absolutePath = path.join(directory, entry.name);
    if (entry.isDirectory()) result.push(...await walk(absolutePath, relativePath));
    else if (entry.isFile()) result.push({ absolutePath, relativePath });
  }
  return result;
}

await rm(outputRoot, { recursive: true, force: true });
await mkdir(outputRoot, { recursive: true });
const files = await walk(root);
for (const file of files) {
  const destination = path.join(outputRoot, file.relativePath);
  await mkdir(path.dirname(destination), { recursive: true });
  if (file.relativePath.toLowerCase().endsWith(".md")) {
    const source = await readFile(file.absolutePath, "utf8");
    const htmlPath = destination.replace(/\.md$/i, ".html");
    const relativeDirectory = path.posix.dirname(file.relativePath) === "." ? "" : path.posix.dirname(file.relativePath);
    await writeFile(htmlPath, pageShell(rewriteMarkdownLinks(renderBody(source)), path.basename(file.relativePath, ".md"), relativeDirectory), "utf8");
  } else if (file.relativePath.toLowerCase().endsWith(".html")) {
    await writeFile(destination, rewriteMarkdownLinks(await readFile(file.absolutePath, "utf8")), "utf8");
  } else {
    await cp(file.absolutePath, destination);
  }
}
console.log(`GitHub Pages site built at ${outputRoot}`);

