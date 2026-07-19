(() => {
  "use strict";

  const KEY = "netforge-matching-capstone-v1";
  const steps = [
    ["step-01", "steps/01-contract.html", "Model the contract"],
    ["step-02", "steps/02-price-levels.html", "Build price levels"],
    ["step-03", "steps/03-price-time.html", "Match in price-time order"],
    ["step-04", "steps/04-lifecycle.html", "Complete the lifecycle"],
    ["step-05", "steps/05-engine.html", "Route through one engine"],
    ["step-06", "steps/06-replay.html", "Generate and replay data"],
    ["step-07", "steps/07-lock-free-ingress.html", "Build lock-free ingress"],
    ["step-08", "steps/08-pipeline.html", "Connect the pipeline"],
    ["step-09", "steps/09-redis-kafka.html", "Isolate Redis and Kafka"],
    ["step-10", "steps/10-measure-defend.html", "Measure and defend"],
  ];
  const script = document.currentScript;
  const capstoneRoot = new URL("../capstone/matching/", script.src);

  const empty = () => ({ checks: {}, completed: {} });
  const load = () => {
    try {
      const parsed = JSON.parse(localStorage.getItem(KEY) || "{}");
      return {
        checks: parsed.checks && typeof parsed.checks === "object" ? parsed.checks : {},
        completed: parsed.completed && typeof parsed.completed === "object" ? parsed.completed : {},
      };
    } catch { return empty(); }
  };
  let state = load();
  const save = () => {
    try { localStorage.setItem(KEY, JSON.stringify(state)); return true; }
    catch { return false; }
  };

  const render = () => {
    const completedCount = steps.filter(([id]) => state.completed[id]).length;
    document.querySelectorAll("[data-capstone-progress-count]").forEach((node) => {
      node.textContent = `${completedCount} / ${steps.length} milestones`;
    });
    document.querySelectorAll("[data-capstone-progress-bar]").forEach((node) => {
      node.style.width = `${(completedCount / steps.length) * 100}%`;
    });
    const firstIncomplete = steps.findIndex(([id]) => !state.completed[id]);
    const resumeIndex = firstIncomplete === -1 ? steps.length - 1 : firstIncomplete;
    document.querySelectorAll("[data-capstone-resume]").forEach((link) => {
      link.href = new URL(steps[resumeIndex][1], capstoneRoot).href;
      link.textContent = firstIncomplete === -1 ? "Review final milestone" : completedCount === 0 ? "Start Step 1" : `Resume Step ${resumeIndex + 1}`;
    });
    steps.forEach(([id], index) => {
      document.querySelectorAll(`[data-capstone-step-card="${id}"]`).forEach((card) => {
        const current = index === resumeIndex && firstIncomplete !== -1;
        card.dataset.state = state.completed[id] ? "complete" : current ? "current" : "upcoming";
        const status = card.querySelector("[data-capstone-step-status]");
        if (status) status.textContent = state.completed[id] ? "Complete" : current ? "Up next" : "Upcoming";
      });
    });
    document.querySelectorAll("[data-capstone-complete]").forEach((button) => {
      const complete = Boolean(state.completed[button.dataset.capstoneComplete]);
      button.textContent = complete ? "Completed — undo" : "Mark milestone complete";
      button.setAttribute("aria-pressed", String(complete));
    });
  };

  document.querySelectorAll("[data-capstone-check]").forEach((input) => {
    input.checked = Boolean(state.checks[input.dataset.capstoneCheck]);
    input.addEventListener("change", () => {
      state.checks[input.dataset.capstoneCheck] = input.checked;
      save();
    });
  });

  document.querySelectorAll("[data-capstone-complete]").forEach((button) => {
    button.addEventListener("click", () => {
      const id = button.dataset.capstoneComplete;
      const feedback = button.parentElement.querySelector("[data-capstone-feedback]");
      if (!state.completed[id]) {
        const unchecked = [...button.parentElement.querySelectorAll("[data-capstone-check]:not(:checked)")];
        if (unchecked.length) {
          feedback.textContent = `Finish the ${unchecked.length} unchecked proof gate${unchecked.length === 1 ? "" : "s"} first.`;
          feedback.className = "feedback incorrect";
          unchecked[0].focus();
          return;
        }
      }
      state.completed[id] = !state.completed[id];
      save();
      feedback.textContent = state.completed[id] ? "Milestone saved. Continue only when you can explain the invariant without notes." : "Milestone reopened; proof checks are preserved.";
      feedback.className = "feedback correct";
      render();
    });
  });

  document.querySelectorAll("[data-copy]").forEach((button) => {
    button.addEventListener("click", async () => {
      const target = document.querySelector(button.dataset.copy);
      if (!target) return;
      try { await navigator.clipboard.writeText(target.textContent.trim()); button.textContent = "Copied"; }
      catch { button.textContent = "Copy unavailable"; }
    });
  });

  document.querySelectorAll("[data-capstone-reset]").forEach((button) => {
    button.addEventListener("click", () => {
      if (!window.confirm("Reset matching-capstone progress? Your source code and evidence remain untouched.")) return;
      state = empty(); save();
      document.querySelectorAll("[data-capstone-check]").forEach((input) => { input.checked = false; });
      render();
    });
  });

  window.addEventListener("storage", (event) => {
    if (event.key !== KEY) return;
    state = load(); render();
  });

  render();
})();
