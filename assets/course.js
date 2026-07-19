(() => {
  "use strict";

  const COURSE_KEY = "netforge-course-v1";
  const lessonPaths = [
    "lessons/0001-own-the-bytes.html",
    "lessons/0002-never-leak-a-descriptor.html",
    "lessons/0003-modern-cpp-interfaces.html",
    "lessons/0004-make-shared-state-boring.html",
    "lessons/0005-threads-with-lifetimes.html",
    "lessons/0006-read-the-wire.html",
    "lessons/0007-socket-reality.html",
    "lessons/0008-scale-the-event-loop.html",
    "lessons/0009-cross-process-boundaries.html",
    "lessons/0010-debug-under-fire.html",
    "lessons/0011-persist-and-recover.html",
    "lessons/0012-harden-the-boundary.html",
    "lessons/0013-design-and-explain.html",
    "lessons/0014-prove-readiness.html",
  ];
  const lessonIds = lessonPaths.map((_, index) => `day-${String(index + 1).padStart(2, "0")}`);
  const domainLabels = {
    networking: "networking and framing (Days 1, 6–8)",
    resources: "resource ownership (Days 1–2)",
    concurrency: "concurrency (Days 4–5)",
    storage: "durability and storage (Day 11)",
    "modern-cpp": "modern C++ interfaces (Days 2–3)",
  };
  const dayPrerequisiteDomain = {
    "01": "networking",
    "02": "resources",
    "03": "modern-cpp",
    "04": "concurrency",
    "05": "concurrency",
    "06": "networking",
    "07": "networking",
    "08": "networking",
    "11": "storage",
  };
  const diagnosticReviewDay = {
    networking: "day-01",
    resources: "day-02",
    "modern-cpp": "day-03",
    concurrency: "day-04",
    storage: "day-11",
  };
  const reviewIntervals = [1, 3, 7];
  const dayMilliseconds = 24 * 60 * 60 * 1000;

  const emptyState = () => ({ lessons: {}, checks: {}, reviews: {} });
  const load = () => {
    try {
      const parsed = JSON.parse(localStorage.getItem(COURSE_KEY) || "{}");
      if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) return emptyState();
      return {
        ...parsed,
        lessons: parsed.lessons && typeof parsed.lessons === "object" ? parsed.lessons : {},
        checks: parsed.checks && typeof parsed.checks === "object" ? parsed.checks : {},
        reviews: parsed.reviews && typeof parsed.reviews === "object" ? parsed.reviews : {},
      };
    } catch {
      return emptyState();
    }
  };

  let state = load();
  const save = () => {
    try {
      localStorage.setItem(COURSE_KEY, JSON.stringify(state));
      return true;
    } catch {
      return false;
    }
  };

  const completionFeedback = (button) => {
    let feedback = button.parentElement?.querySelector("[data-completion-feedback]");
    if (feedback) return feedback;
    feedback = document.createElement("p");
    feedback.className = "feedback";
    feedback.dataset.completionFeedback = "";
    feedback.setAttribute("aria-live", "polite");
    button.insertAdjacentElement("afterend", feedback);
    return feedback;
  };

  const syncChecks = () => {
    document.querySelectorAll("[data-check-id]").forEach((input) => {
      input.checked = Boolean(state.checks[input.dataset.checkId]);
    });
  };

  document.querySelectorAll("[data-check-id]").forEach((input) => {
    input.addEventListener("change", () => {
      state.checks[input.dataset.checkId] = input.checked;
      save();
      const completionButton = input.closest("article")?.querySelector("[data-lesson-complete]");
      if (completionButton) {
        const unchecked = completionButton.closest("article")?.querySelectorAll("[data-check-id]:not(:checked)") ?? [];
        if (unchecked.length === 0) completionFeedback(completionButton).textContent = "Proof gates attested. You can now complete the lesson.";
      }
    });
  });
  syncChecks();

  const paintCompletionButton = (button) => {
    const complete = Boolean(state.lessons[button.dataset.lessonComplete]);
    button.textContent = complete ? "Completed — undo" : "Mark lesson complete";
    button.setAttribute("aria-pressed", String(complete));
  };

  const syncCompletionButtons = () => {
    document.querySelectorAll("[data-lesson-complete]").forEach(paintCompletionButton);
  };

  document.querySelectorAll("[data-lesson-complete]").forEach((button) => {
    button.addEventListener("click", () => {
      const id = button.dataset.lessonComplete;
      const complete = Boolean(state.lessons[id]);
      const feedback = completionFeedback(button);
      if (!complete) {
        const unchecked = [...(button.closest("article")?.querySelectorAll("[data-check-id]:not(:checked)") ?? [])];
        if (unchecked.length > 0) {
          feedback.textContent = `Finish the ${unchecked.length} unchecked proof gate${unchecked.length === 1 ? "" : "s"} before completing this lesson.`;
          feedback.className = "feedback incorrect";
          unchecked[0].focus();
          return;
        }
      }

      state.lessons[id] = !complete;
      const persisted = save();
      paintCompletionButton(button);
      feedback.textContent = !complete
        ? persisted
          ? "Lesson completed with every proof gate attested. If the evidence established a non-trivial insight, record it with scripts/record-learning.sh; do not log mere coverage."
          : "Lesson completed for this tab; browser storage is unavailable."
        : "Completion removed; your proof-gate checks remain saved.";
      feedback.className = "feedback correct";
      window.dispatchEvent(new Event("netforge-progress"));
    });
  });
  syncCompletionButtons();

  const renderProgress = () => {
    const completed = lessonIds.filter((id) => Boolean(state.lessons[id])).length;
    document.querySelectorAll("[data-progress-count]").forEach((node) => {
      node.textContent = `${completed} / ${lessonIds.length} lessons`;
    });
    document.querySelectorAll("[data-progress-bar]").forEach((node) => {
      node.style.width = `${(completed / lessonIds.length) * 100}%`;
    });
    document.querySelectorAll("[data-day-card]").forEach((card) => {
      card.dataset.complete = String(Boolean(state.lessons[card.dataset.dayCard]));
    });

    const resumeIndex = lessonIds.findIndex((id) => !state.lessons[id]);
    const targetIndex = resumeIndex === -1 ? lessonIds.length - 1 : resumeIndex;
    document.querySelectorAll("[data-resume-link]").forEach((link) => {
      link.href = lessonPaths[targetIndex];
      link.textContent = resumeIndex === -1 ? "Review Day 14" : completed === 0 ? "Start Day 1" : `Resume Day ${targetIndex + 1}`;
    });
  };

  const renderDiagnostic = () => {
    document.querySelectorAll("[data-diagnostic-summary]").forEach((node) => {
      if (state.diagnostic?.lane) {
        const missed = state.diagnostic.missedDomains?.map((domain) => domainLabels[domain] ?? domain).join(", ");
        node.textContent = `Knowledge lane: ${state.diagnostic.lane} · ${state.diagnostic.correct}/${state.diagnostic.total}.${missed ? ` Review first: ${missed}.` : " No domain miss."} The practical gate chooses the final lower lane; Core proof gates remain mandatory.`;
      } else {
        node.textContent = "Your lane is not set yet. Day 1 begins with a knowledge check and a 20-minute practical gate.";
      }
    });
  };

  const renderReviewQueue = () => {
    const now = Date.now();
    const active = Object.entries(state.reviews ?? {})
      .filter(([id, review]) => lessonIds.includes(id) && review && !review.mastered && Number.isFinite(review.due))
      .sort((left, right) => left[1].due - right[1].due);
    const due = active.filter(([, review]) => review.due <= now);
    document.querySelectorAll("[data-review-summary]").forEach((node) => {
      if (due.length > 0) {
        node.textContent = `${due.length} spaced review item${due.length === 1 ? " is" : "s are"} due. Retrieve before adding Stretch work.`;
      } else if (active.length > 0) {
        const daysUntil = Math.max(1, Math.ceil((active[0][1].due - now) / dayMilliseconds));
        node.textContent = `No review is due. The next 1/3/7-day retrieval opens in about ${daysUntil} day${daysUntil === 1 ? "" : "s"}.`;
      } else {
        node.textContent = "The 1/3/7-day review queue starts after your first quiz.";
      }
    });
    document.querySelectorAll("[data-review-link]").forEach((link) => {
      if (due.length === 0) {
        link.hidden = true;
        return;
      }
      const lessonIndex = lessonIds.indexOf(due[0][0]);
      link.href = lessonPaths[lessonIndex];
      link.textContent = `Review Day ${lessonIndex + 1} now →`;
      link.hidden = false;
    });
  };

  const recordQuizResult = (id, correct) => {
    if (!lessonIds.includes(id)) return "";
    state.reviews ||= {};
    const now = Date.now();
    const current = state.reviews[id];
    if (!correct) {
      state.reviews[id] = {
        stage: current?.stage ?? 0,
        due: now,
        misses: (current?.misses ?? 0) + 1,
        reason: "quiz miss",
      };
      save();
      renderReviewQueue();
      return " This concept is now due in the spaced review queue.";
    }
    if (!current) {
      state.reviews[id] = { stage: 0, due: now + reviewIntervals[0] * dayMilliseconds, misses: 0, reason: "initial retrieval" };
      save();
      renderReviewQueue();
      return " Spaced review is scheduled in 1 day.";
    }
    if (current.reason === "quiz miss" || current.reason === "diagnostic miss") {
      state.reviews[id] = { ...current, stage: 0, due: now + reviewIntervals[0] * dayMilliseconds, reason: "scheduled retrieval" };
      save();
      renderReviewQueue();
      return " The miss is cleared; retrieve it again in 1 day.";
    }
    if (current.due > now) return " The existing spaced-review date is unchanged.";
    if (current.stage >= reviewIntervals.length - 1) {
      state.reviews[id] = { ...current, mastered: true, due: 0 };
      save();
      renderReviewQueue();
      return " The 1/3/7-day cycle is complete; keep using it in mocks.";
    }
    const nextStage = current.stage + 1;
    state.reviews[id] = {
      ...current,
      stage: nextStage,
      due: now + reviewIntervals[nextStage] * dayMilliseconds,
      reason: "scheduled retrieval",
    };
    save();
    renderReviewQueue();
    return ` Next review is scheduled in ${reviewIntervals[nextStage]} days.`;
  };

  const renderPrerequisiteReminder = () => {
    const day = document.body.dataset.day;
    const domain = dayPrerequisiteDomain[day];
    const article = document.querySelector("article");
    const existing = article?.querySelector("[data-prerequisite-reminder]");
    if (!domain || !state.diagnostic?.missedDomains?.includes(domain)) {
      existing?.remove();
      return;
    }
    if (!article || existing) return;
    const reminder = document.createElement("aside");
    reminder.className = "callout warning";
    reminder.dataset.prerequisiteReminder = "";
    reminder.innerHTML = `<strong>Diagnostic review.</strong> Your Day 1 knowledge check missed ${domainLabels[domain]}. Keep the Core lane for this module and retrieve the linked reference before Stretch work.`;
    article.insertBefore(reminder, article.children[1] ?? null);
  };

  const renderRetrievalReminder = () => {
    const day = document.body.dataset.day;
    if (!day || document.querySelector("[data-retrieval-reminder]")) return;
    const recallHeading = [...document.querySelectorAll("h2")]
      .find((heading) => /recall/i.test(heading.textContent));
    if (!recallHeading) return;
    const reminder = document.createElement("p");
    reminder.className = "quiet";
    reminder.dataset.retrievalReminder = "";
    reminder.innerHTML = `Start with the <a href="../interview/RETRIEVAL-DECK.md#day-${day}">Day ${Number(day)} D-1/D-3/D-7 retrieval set</a>. Mark partial or missed prompts for the next review.`;
    recallHeading.insertAdjacentElement("afterend", reminder);
  };

  renderProgress();
  renderDiagnostic();
  renderReviewQueue();
  renderPrerequisiteReminder();
  renderRetrievalReminder();
  window.addEventListener("netforge-progress", renderProgress);
  window.addEventListener("storage", (event) => {
    if (event.key !== COURSE_KEY) return;
    state = load();
    syncChecks();
    syncCompletionButtons();
    renderProgress();
    renderDiagnostic();
    renderReviewQueue();
    renderPrerequisiteReminder();
  });

  document.querySelectorAll("[data-quiz]").forEach((quiz) => {
    const answer = quiz.dataset.answer;
    const feedback = quiz.querySelector(".feedback");
    const button = quiz.querySelector("button");
    if (!button || !feedback) return;
    button.addEventListener("click", () => {
      const selected = quiz.querySelector("input[type=radio]:checked");
      if (!selected) {
        feedback.textContent = "Choose one answer first.";
        feedback.className = "feedback incorrect";
        return;
      }
      const correct = selected.value === answer;
      const baseFeedback = correct
        ? quiz.dataset.correct || "Correct. Explain why before moving on."
        : quiz.dataset.incorrect || "Not yet. Rebuild the causal chain, then retry.";
      const day = document.body.dataset.day;
      feedback.textContent = baseFeedback + recordQuizResult(`day-${day}`, correct);
      feedback.className = `feedback ${correct ? "correct" : "incorrect"}`;
    });
  });

  document.querySelectorAll("[data-diagnostic]").forEach((diagnostic) => {
    const button = diagnostic.querySelector("[data-score-diagnostic]");
    const output = diagnostic.querySelector("[data-diagnostic-result]");
    if (!button || !output) return;
    button.addEventListener("click", () => {
      const questions = [...diagnostic.querySelectorAll("[data-diagnostic-answer]")];
      let answered = 0;
      let correct = 0;
      const domains = {};
      questions.forEach((question) => {
        const selected = question.querySelector("input[type=radio]:checked");
        if (!selected) return;
        answered += 1;
        const domain = question.dataset.diagnosticDomain || "general";
        domains[domain] ||= { correct: 0, total: 0 };
        domains[domain].total += 1;
        if (selected.value === question.dataset.diagnosticAnswer) {
          correct += 1;
          domains[domain].correct += 1;
        }
      });
      if (answered !== questions.length) {
        output.textContent = `Answer all ${questions.length} items before scoring.`;
        output.className = "feedback incorrect";
        return;
      }
      const percent = Math.round((correct / questions.length) * 100);
      const lane = percent < 60 ? "Core" : percent < 80 ? "Core + one Stretch item" : "Compressed + Stretch";
      const missedDomains = Object.entries(domains)
        .filter(([, score]) => score.correct < score.total)
        .map(([domain]) => domain);
      state.diagnostic = { correct, total: questions.length, percent, lane, domains, missedDomains };
      state.reviews ||= {};
      for (const domain of missedDomains) {
        const reviewDay = diagnosticReviewDay[domain];
        if (reviewDay && !state.reviews[reviewDay]) {
          state.reviews[reviewDay] = { stage: 0, due: Date.now(), misses: 1, reason: "diagnostic miss" };
        }
      }
      for (const [domain, score] of Object.entries(domains)) {
        const reviewDay = diagnosticReviewDay[domain];
        const review = reviewDay ? state.reviews[reviewDay] : undefined;
        if (score.correct === score.total && review?.reason === "diagnostic miss") {
          state.reviews[reviewDay] = { ...review, stage: 0, due: Date.now() + dayMilliseconds, reason: "scheduled retrieval" };
        }
      }
      save();
      const remediation = missedDomains.map((domain) => domainLabels[domain] ?? domain).join(", ");
      output.textContent = `${correct}/${questions.length} (${percent}%). Provisional knowledge lane: ${lane}.${remediation ? ` Review first: ${remediation}.` : ""} Use the lower result after the practical gate.`;
      output.className = "feedback correct";
      renderDiagnostic();
      renderReviewQueue();
      renderPrerequisiteReminder();
    });
  });

  const copyText = async (text) => {
    if (navigator.clipboard?.writeText) {
      try {
        await navigator.clipboard.writeText(text);
        return true;
      } catch {
        // Fall through to the local-file-compatible selection path.
      }
    }
    const helper = document.createElement("textarea");
    helper.value = text;
    helper.setAttribute("readonly", "");
    helper.style.position = "fixed";
    helper.style.opacity = "0";
    document.body.appendChild(helper);
    helper.select();
    const copied = document.execCommand("copy");
    helper.remove();
    return copied;
  };

  document.querySelectorAll("[data-copy]").forEach((button) => {
    button.addEventListener("click", async () => {
      const target = document.querySelector(button.dataset.copy);
      if (!target) return;
      const previous = button.textContent;
      button.textContent = await copyText(target.textContent) ? "Copied" : "Select and copy manually";
      setTimeout(() => { button.textContent = previous; }, 1600);
    });
  });

  const reset = document.querySelector("[data-reset-progress]");
  if (reset) {
    reset.addEventListener("click", () => {
      if (!window.confirm("Reset all local NetForge lesson progress? Repository evidence is untouched.")) return;
      try {
        localStorage.removeItem(COURSE_KEY);
      } catch {
        // In-memory reset still keeps the page usable when storage is denied.
      }
      state = emptyState();
      syncChecks();
      syncCompletionButtons();
      renderProgress();
      renderDiagnostic();
      renderReviewQueue();
    });
  }
})();
