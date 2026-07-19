import { readdir, readFile, stat } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(process.argv[2] || path.join(scriptDirectory, ".."));
const ignoredDirectories = new Set([".git", ".idea", ".vscode-test", "build", "coverage", "dist", "node_modules", "out"]);
const expectedLessonCount = 14;
const expectedTotalMinutes = 2430;
const manifestMinimumBlockMinutes = 15;
const manifestMaximumBlockMinutes = 45;
const issues = [];
const htmlCache = new Map();

function relativeName(filePath) {
  return path.relative(repositoryRoot, filePath).split(path.sep).join("/") || ".";
}

function issue(filePath, message) {
  issues.push(`${relativeName(filePath)}: ${message}`);
}

async function walk(directory) {
  const files = [];
  let entries;
  try {
    entries = await readdir(directory, { withFileTypes: true });
  } catch (error) {
    if (error.code === "ENOENT") return files;
    throw error;
  }
  entries.sort((left, right) => left.name.localeCompare(right.name));
  for (const entry of entries) {
    if (entry.isDirectory() && ignoredDirectories.has(entry.name)) continue;
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...await walk(entryPath));
    else if (entry.isFile()) files.push(entryPath);
  }
  return files;
}

function attribute(tag, name) {
  const escapedName = name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const match = tag.match(new RegExp(`\\b${escapedName}\\s*=\\s*(?:"([^"]*)"|'([^']*)'|([^\\s>]+))`, "i"));
  return match ? (match[1] ?? match[2] ?? match[3] ?? "") : undefined;
}

function metaValue(html, names) {
  const acceptedNames = new Set(names.map((name) => name.toLowerCase()));
  for (const match of html.matchAll(/<meta\b[^>]*>/gi)) {
    const name = attribute(match[0], "name")?.toLowerCase();
    if (name && acceptedNames.has(name)) return attribute(match[0], "content");
  }
  return undefined;
}

function visibleText(fragment) {
  return fragment
    .replace(/<script\b[\s\S]*?<\/script>/gi, " ")
    .replace(/<style\b[\s\S]*?<\/style>/gi, " ")
    .replace(/<[^>]+>/g, " ")
    .replace(/&(?:nbsp|#160);/gi, " ")
    .replace(/&(?:amp|#38);/gi, "&")
    .replace(/&(?:lt|#60);/gi, "<")
    .replace(/&(?:gt|#62);/gi, ">")
    .replace(/&(?:quot|#34);/gi, "\"")
    .replace(/&#39;|&apos;/gi, "'")
    .replace(/\s+/g, " ")
    .trim();
}

function wordCount(fragment) {
  const text = visibleText(fragment);
  return text ? text.split(/\s+/u).length : 0;
}

function markdownHeadingId(heading) {
  return heading
    .toLowerCase()
    .replace(/<[^>]*>/g, "")
    .replace(/[`*_~]/g, "")
    .replace(/[^\p{L}\p{N}\s-]/gu, "")
    .trim()
    .replace(/\s/g, "-");
}

async function verifyFragment(sourceFile, targetFile, fragment) {
  if (!fragment) return;
  const targetText = await readFile(targetFile, "utf8");
  const extension = path.extname(targetFile).toLowerCase();
  if (extension === ".html") {
    const escaped = fragment.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
    if (!new RegExp(`\\b(?:id|name)\\s*=\\s*(?:"${escaped}"|'${escaped}')`, "i").test(targetText)) {
      issue(sourceFile, `missing HTML fragment #${fragment} in ${relativeName(targetFile)}`);
    }
    return;
  }
  if (extension === ".md") {
    const headingIds = [...targetText.matchAll(/^#{1,6}\s+(.+)$/gm)]
      .map((match) => markdownHeadingId(match[1].trim()));
    if (!headingIds.includes(fragment)) {
      issue(sourceFile, `missing Markdown heading fragment #${fragment} in ${relativeName(targetFile)}`);
    }
  }
}

async function htmlText(filePath) {
  if (!htmlCache.has(filePath)) htmlCache.set(filePath, await readFile(filePath, "utf8"));
  return htmlCache.get(filePath);
}

function localReference(rawValue) {
  const value = rawValue.trim().replaceAll("&amp;", "&");
  if (!value || value === "#" || value.startsWith("//")) return undefined;
  if (/^[a-z][a-z0-9+.-]*:/i.test(value)) return undefined;
  const hashIndex = value.indexOf("#");
  const beforeHash = hashIndex >= 0 ? value.slice(0, hashIndex) : value;
  const queryIndex = beforeHash.indexOf("?");
  const pathname = queryIndex >= 0 ? beforeHash.slice(0, queryIndex) : beforeHash;
  return pathname;
}

function isInsideRepository(candidate) {
  const relative = path.relative(repositoryRoot, candidate);
  return relative === "" || (!relative.startsWith(`..${path.sep}`) && relative !== ".." && !path.isAbsolute(relative));
}

async function verifyLocalTargets(htmlFiles) {
  for (const htmlFile of htmlFiles) {
    const html = await htmlText(htmlFile);
    const linkPattern = /\b(?:href|src)\s*=\s*(?:"([^"]*)"|'([^']*)')/gi;
    for (const match of html.matchAll(linkPattern)) {
      const rawTarget = match[1] ?? match[2] ?? "";
      let decodedTarget;
      try {
        decodedTarget = decodeURIComponent(rawTarget.trim().replaceAll("&amp;", "&"));
      } catch {
        issue(htmlFile, `invalid percent-encoding in local target ${JSON.stringify(rawTarget)}`);
        continue;
      }
      const hashIndex = decodedTarget.indexOf("#");
      const fragment = hashIndex >= 0 ? decodedTarget.slice(hashIndex + 1) : "";
      let pathname = localReference(decodedTarget);
      if (pathname === undefined || pathname === "") continue;

      const target = pathname.startsWith("/")
        ? path.resolve(repositoryRoot, `.${pathname}`)
        : path.resolve(path.dirname(htmlFile), pathname);
      if (!isInsideRepository(target)) {
        issue(htmlFile, `local target escapes the repository: ${JSON.stringify(rawTarget)}`);
        continue;
      }
      try {
        await stat(target);
        await verifyFragment(htmlFile, target, fragment);
      } catch (error) {
        if (error.code === "ENOENT") issue(htmlFile, `missing local target ${JSON.stringify(rawTarget)}`);
        else issue(htmlFile, `cannot inspect ${JSON.stringify(rawTarget)} (${error.message})`);
      }
    }
  }
}

async function verifyLessons(allFiles) {
  const lessonsDirectory = path.join(repositoryRoot, "lessons");
  const lessonHtmlFiles = allFiles.filter((filePath) =>
    path.dirname(filePath) === lessonsDirectory && filePath.toLowerCase().endsWith(".html"));
  const lessonPattern = /^(\d{4})-([a-z0-9]+(?:-[a-z0-9]+)*)\.html$/;
  const lessonsBySequence = new Map();
  const checkIdOwners = new Map();
  const resourceShelf = await readFile(path.join(repositoryRoot, "RESOURCES.md"), "utf8");
  let totalDuration = 0;

  for (const lessonFile of lessonHtmlFiles) {
    const match = path.basename(lessonFile).match(lessonPattern);
    if (!match) {
      issue(lessonFile, "lesson filename must match NNNN-kebab-case-slug.html");
      continue;
    }
    const sequence = Number(match[1]);
    if (lessonsBySequence.has(sequence)) {
      issue(lessonFile, `duplicate lesson sequence ${match[1]}`);
      continue;
    }
    lessonsBySequence.set(sequence, lessonFile);
  }

  for (let sequence = 1; sequence <= expectedLessonCount; sequence += 1) {
    if (!lessonsBySequence.has(sequence)) {
      issue(lessonsDirectory, `missing lesson ${String(sequence).padStart(4, "0")}-<slug>.html`);
    }
  }
  for (const [sequence, lessonFile] of lessonsBySequence) {
    if (sequence < 1 || sequence > expectedLessonCount) issue(lessonFile, "lesson sequence must be between 0001 and 0014");
  }

  for (const [sequence, lessonFile] of [...lessonsBySequence.entries()].sort((a, b) => a[0] - b[0])) {
    if (sequence < 1 || sequence > expectedLessonCount) continue;
    const html = await htmlText(lessonFile);
    const expectedDay = String(sequence).padStart(2, "0");
    const bodyMatches = [...html.matchAll(/<body\b[^>]*>/gi)];
    if (bodyMatches.length !== 1) {
      issue(lessonFile, `expected exactly one <body>, found ${bodyMatches.length}`);
      continue;
    }

    const body = bodyMatches[0][0];
    const bodyDay = attribute(body, "data-day");
    const metaDay = metaValue(html, ["netforge:day", "lesson:day"]);
    if (bodyDay === undefined) issue(lessonFile, `<body> is missing data-day="${expectedDay}"`);
    else if (bodyDay !== expectedDay) issue(lessonFile, `body data-day=${JSON.stringify(bodyDay)} disagrees with filename day ${expectedDay}`);
    if (metaDay !== undefined && metaDay !== expectedDay) issue(lessonFile, `meta day ${JSON.stringify(metaDay)} disagrees with filename day ${expectedDay}`);

    const bodyTitle = attribute(body, "data-title");
    const metaTitle = metaValue(html, ["netforge:title", "lesson:title"]);
    const lessonTitle = bodyTitle ?? metaTitle;
    if (!lessonTitle?.trim()) issue(lessonFile, "missing non-empty body data-title or netforge:title meta value");
    if (bodyTitle !== undefined && metaTitle !== undefined && bodyTitle.trim() !== metaTitle.trim()) {
      issue(lessonFile, "body data-title and title meta value disagree");
    }

    const bodyDuration = attribute(body, "data-duration");
    const metaDuration = metaValue(html, ["netforge:duration", "netforge:minutes", "lesson:duration"]);
    const duration = bodyDuration ?? metaDuration;
    if (duration === undefined || !/^\d+$/.test(duration) || Number(duration) <= 0) {
      issue(lessonFile, "duration must be a positive minute count in body data-duration or a duration meta value");
    } else {
      totalDuration += Number(duration);
    }
    if (bodyDuration !== undefined && metaDuration !== undefined && bodyDuration !== metaDuration) {
      issue(lessonFile, "body data-duration and duration meta value disagree");
    }

    const titleMatch = html.match(/<title\b[^>]*>([\s\S]*?)<\/title>/i);
    if (!titleMatch?.[1].trim()) issue(lessonFile, "missing a non-empty document <title>");

    const requiredSignals = [
      [/\brecall\b/i, "a recall/retrieval section"],
      [/\bproof gate\b/i, "a proof gate"],
      [/\bstuck\b/i, "a stuck protocol"],
      [/ask (?:the )?agent/i, "an agent follow-up reminder"],
      [/capstone\/work/i, "a capstone/work project action"],
    ];
    for (const [pattern, description] of requiredSignals) {
      if (!pattern.test(html)) issue(lessonFile, `missing ${description}`);
    }
    const externalLinks = [...html.matchAll(/href\s*=\s*(?:"(https?:\/\/[^\"]+)"|'(https?:\/\/[^']+)')/gi)]
      .map((match) => (match[1] ?? match[2] ?? "").replaceAll("&amp;", "&"));
    if (externalLinks.length === 0) {
      issue(lessonFile, "missing an external primary or authoritative source link");
    }
    for (const externalLink of externalLinks) {
      let withoutFragment = externalLink;
      try {
        const parsed = new URL(externalLink);
        parsed.hash = "";
        withoutFragment = parsed.toString();
      } catch {
        issue(lessonFile, `invalid external source URL ${JSON.stringify(externalLink)}`);
        continue;
      }
      if (!resourceShelf.includes(externalLink) && !resourceShelf.includes(withoutFragment)) {
        issue(lessonFile, `external lesson source is not catalogued in RESOURCES.md: ${JSON.stringify(externalLink)}`);
      }
    }

    const quizSections = [...html.matchAll(/<section\b(?=[^>]*\bdata-quiz\b)[^>]*>[\s\S]*?<\/section>/gi)]
      .map((match) => match[0]);
    if (quizSections.length !== 1) {
      issue(lessonFile, `expected exactly one data-quiz section, found ${quizSections.length}`);
    } else {
      const quiz = quizSections[0];
      const openingTag = quiz.match(/^<section\b[^>]*>/i)?.[0] ?? "";
      const answer = attribute(openingTag, "data-answer");
      const radioValues = [...quiz.matchAll(/<input\b(?=[^>]*\btype\s*=\s*(?:"radio"|'radio'|radio))[^>]*>/gi)]
        .map((match) => attribute(match[0], "value"))
        .filter((value) => value !== undefined);
      const choices = [...quiz.matchAll(/<label\b(?=[^>]*\bclass\s*=\s*(?:"[^"]*\bchoice\b[^"]*"|'[^']*\bchoice\b[^']*'))[^>]*>([\s\S]*?)<\/label>/gi)]
        .map((match) => match[1]);
      if (answer === undefined || !radioValues.includes(answer)) {
        issue(lessonFile, "quiz data-answer must match one radio value");
      }
      if (radioValues.length !== 3 || new Set(radioValues).size !== 3 || choices.length !== 3) {
        issue(lessonFile, "quiz must contain exactly three uniquely-valued choices");
      }
      if (!attribute(openingTag, "data-correct")?.trim() || !attribute(openingTag, "data-incorrect")?.trim()) {
        issue(lessonFile, "quiz must provide specific correct and incorrect feedback");
      }
      if (!/class\s*=\s*(?:"[^"]*\bfeedback\b[^"]*"|'[^']*\bfeedback\b[^']*')[^>]*\baria-live\s*=/i.test(quiz)) {
        issue(lessonFile, "quiz feedback must announce changes with aria-live");
      }
      const choiceWordCounts = choices.map(wordCount);
      if (choiceWordCounts.length === 3 && new Set(choiceWordCounts).size !== 1) {
        issue(lessonFile, `quiz choices must have equal word counts; found ${choiceWordCounts.join(", ")}`);
      }
    }

    const checkIds = [...html.matchAll(/\bdata-check-id\s*=\s*(?:"([^"]+)"|'([^']+)'|([^\s>]+))/gi)]
      .map((match) => match[1] ?? match[2] ?? match[3] ?? "");
    if (checkIds.length < 3) issue(lessonFile, `expected at least three proof checks, found ${checkIds.length}`);
    for (const checkId of checkIds) {
      const previousOwner = checkIdOwners.get(checkId);
      if (previousOwner) issue(lessonFile, `duplicate data-check-id ${JSON.stringify(checkId)} also used by ${relativeName(previousOwner)}`);
      else checkIdOwners.set(checkId, lessonFile);
    }

    if (!/href\s*=\s*(?:"\.\.\/index\.html"|'\.\.\/index\.html')/i.test(html)) {
      issue(lessonFile, "lesson navigation must link back to the dashboard");
    }
    if (sequence > 1) {
      const previousLesson = path.basename(lessonsBySequence.get(sequence - 1));
      if (!html.includes(previousLesson)) issue(lessonFile, `missing previous-lesson link to ${previousLesson}`);
    }
    if (sequence < expectedLessonCount) {
      const nextLesson = path.basename(lessonsBySequence.get(sequence + 1));
      if (!html.includes(nextLesson)) issue(lessonFile, `missing next-lesson link to ${nextLesson}`);
    }

    const completionValues = [...html.matchAll(/\bdata-lesson-complete\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/gi)]
      .map((match) => match[1] ?? match[2] ?? match[3] ?? "");
    if (completionValues.length !== 1) {
      issue(lessonFile, `expected exactly one data-lesson-complete control, found ${completionValues.length}`);
    } else if (completionValues[0] !== `day-${expectedDay}`) {
      issue(lessonFile, `completion id ${JSON.stringify(completionValues[0])} must be "day-${expectedDay}"`);
    }
  }

  if (totalDuration !== expectedTotalMinutes) {
    issue(lessonsDirectory, `lesson durations total ${totalDuration} minutes; expected ${expectedTotalMinutes}`);
  }

  const dashboardPath = path.join(repositoryRoot, "index.html");
  const dashboard = await htmlText(dashboardPath);
  for (const [sequence, lessonFile] of lessonsBySequence) {
    if (sequence < 1 || sequence > expectedLessonCount) continue;
    const expectedDay = String(sequence).padStart(2, "0");
    if (!dashboard.includes(`lessons/${path.basename(lessonFile)}`)) {
      issue(dashboardPath, `missing lesson link for day ${expectedDay}`);
    }
    const cardMatches = [...dashboard.matchAll(/\bdata-day-card\s*=\s*(?:"([^"]+)"|'([^']+)'|([^\s>]+))/gi)]
      .map((match) => match[1] ?? match[2] ?? match[3] ?? "")
      .filter((value) => value === `day-${expectedDay}`);
    if (cardMatches.length !== 1) issue(dashboardPath, `expected exactly one day-${expectedDay} dashboard card`);
  }
}

async function verifyCurriculumManifest() {
  const manifestPath = path.join(repositoryRoot, "curriculum-manifest.json");
  let manifest;
  try {
    manifest = JSON.parse(await readFile(manifestPath, "utf8"));
  } catch (error) {
    issue(manifestPath, `invalid or unreadable JSON (${error.message})`);
    return;
  }
  if (manifest.schema_version !== 1) issue(manifestPath, "schema_version must be 1");
  if (manifest.total_minutes !== expectedTotalMinutes) {
    issue(manifestPath, `total_minutes must be ${expectedTotalMinutes}`);
  }
  const days = Array.isArray(manifest.days) ? manifest.days : [];
  if (days.length !== expectedLessonCount) issue(manifestPath, `expected ${expectedLessonCount} manifest days, found ${days.length}`);
  const dayIds = new Set();
  const blockIds = new Set();
  const knownDayIds = new Set(days.map((day) => day?.id).filter(Boolean));
  let totalMinutes = 0;
  for (const day of days) {
    if (!day || typeof day !== "object") {
      issue(manifestPath, "every day entry must be an object");
      continue;
    }
    if (dayIds.has(day.id)) issue(manifestPath, `duplicate day id ${JSON.stringify(day.id)}`);
    dayIds.add(day.id);
    if (!Number.isInteger(day.day) || day.day < 1 || day.day > expectedLessonCount) {
      issue(manifestPath, `invalid numeric day ${JSON.stringify(day.day)}`);
    }
    if (typeof day.lesson !== "string" || !isInsideRepository(path.resolve(repositoryRoot, day.lesson))) {
      issue(manifestPath, `lesson target escapes repository or is not a string for ${JSON.stringify(day.id)}`);
    } else {
      try { await stat(path.resolve(repositoryRoot, day.lesson)); }
      catch { issue(manifestPath, `missing lesson target ${JSON.stringify(day.lesson)}`); }
    }
    const blocks = Array.isArray(day.blocks) ? day.blocks : [];
    const modes = new Set();
    let dayMinutes = 0;
    for (const block of blocks) {
      if (!block || typeof block !== "object") {
        issue(manifestPath, `day ${JSON.stringify(day.id)} contains a non-object block`);
        continue;
      }
      if (blockIds.has(block.id)) issue(manifestPath, `duplicate block id ${JSON.stringify(block.id)}`);
      blockIds.add(block.id);
      if (!Number.isInteger(block.minutes) || block.minutes < manifestMinimumBlockMinutes || block.minutes > manifestMaximumBlockMinutes) {
        issue(manifestPath, `block ${JSON.stringify(block.id)} must be ${manifestMinimumBlockMinutes}-${manifestMaximumBlockMinutes} minutes`);
      }
      if (!block.title || !block.action || !block.evidence || !block.mode) {
        issue(manifestPath, `block ${JSON.stringify(block.id)} needs title, action, mode, and evidence`);
      }
      if (block.scope !== "core") issue(manifestPath, `block ${JSON.stringify(block.id)} must be core-counted or omitted from the manifest`);
      modes.add(block.mode);
      dayMinutes += Number(block.minutes) || 0;
    }
    if (dayMinutes !== day.duration_minutes) issue(manifestPath, `day ${JSON.stringify(day.id)} blocks total ${dayMinutes}; expected ${day.duration_minutes}`);
    if (!["recall", "build", "proof"].every((mode) => modes.has(mode))) {
      issue(manifestPath, `day ${JSON.stringify(day.id)} must include recall, build, and proof blocks`);
    }
    for (const required of (Array.isArray(day.requires) ? day.requires : [])) {
      if (!knownDayIds.has(required)) issue(manifestPath, `day ${JSON.stringify(day.id)} requires unknown ${JSON.stringify(required)}`);
    }
    totalMinutes += dayMinutes;
  }
  if (totalMinutes !== expectedTotalMinutes) issue(manifestPath, `manifest blocks total ${totalMinutes}; expected ${expectedTotalMinutes}`);
}

const allFiles = await walk(repositoryRoot);
const htmlFiles = allFiles.filter((filePath) => filePath.toLowerCase().endsWith(".html"));
if (htmlFiles.length === 0) issues.push("repository: no HTML files found");

await verifyLocalTargets(htmlFiles);
await verifyCurriculumManifest();
await verifyLessons(allFiles);

if (issues.length > 0) {
  console.error(`Content check failed with ${issues.length} issue(s):`);
  for (const message of issues.sort()) console.error(`- ${message}`);
  process.exitCode = 1;
} else {
  console.log(`Content check passed: ${htmlFiles.length} HTML file(s), 14 sequenced lessons, and all local href/src targets exist.`);
}
