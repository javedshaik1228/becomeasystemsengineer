# NetForge: 14 days to C++ systems interview readiness

NetForge is a self-contained, project-based course for C++ systems, networking,
storage, and security interviews. Over fourteen focused days you will build and
defend a framed TCP key-value service in C++20, then diagnose it with the same
tools and mental models expected in senior technical rounds.

## Start here

1. Read [MISSION.md](MISSION.md) and the evidence behind the syllabus in
   [RESEARCH.md](RESEARCH.md).
2. Open [index.html](index.html) for the course dashboard. It stores progress in
   your browser only.
3. Follow Day 1. Every lesson links the exact lab, references, checks, interview
   drill, and completion evidence for that day.
4. Work only in `capstone/work/`. A complete implementation lives in
   `capstone/reference/`; use it only after the lesson's stuck protocol.

The recommended pace is 2.5–3 hours on weekdays and 3–4 hours on the two capstone
days. If you have less time, preserve the build/debug/interview loops and skip
the clearly marked stretch work.

## Environment

The labs target Linux or WSL 2 and also support macOS. Run:

```bash
./scripts/bootstrap.sh
./scripts/doctor.sh
./scripts/check.sh reference all
./scripts/analyze.sh reference
```

On Windows, run those commands inside a WSL distribution. WSL 2 is enabled on
this machine, but a distribution still needs to be installed; see
[SETUP.md](SETUP.md).

## What you will ship

`netforge-server` is a bounded, concurrent TCP service with explicit framing,
RAII-managed descriptors, graceful shutdown, a thread pool, synchronized state,
a write-ahead log, protocol limits, metrics, and recovery tests. Its companion
client, packet decoder, IPC exercise, failure drills, benchmark, architecture
record, and interview scorecards turn the code into interview evidence—not just
a toy repository.

## Course map

- [COURSE.md](COURSE.md): schedule, timeboxes, gates, and readiness rubric.
- [curriculum-manifest.json](curriculum-manifest.json): the machine-checked
  108-block schedule; every Core block is 15–45 minutes and sums to 2,430.
- [RESOURCES.md](RESOURCES.md): curated primary and authoritative sources.
- [reference/](reference/): printable quick-reference documents.
- [lessons/](lessons/): fourteen focused, interactive lessons.
- [labs/](labs/): executable focused labs for measured parallelism and debugging.
- [capstone/](capstone/): editable work tree, tests, and reference implementation.
- [Advanced matching-engine capstone](capstone/matching/README.md): a modern C++ price-time-priority order book with lock-free ingress, replayable mock data, and Redis/Kafka edges.
- [interview/](interview/): question bank, mock rounds, story prompts, scorecards.
- [.vscode/](.vscode/): optional focused VS Code setup and one-click tasks.

No later Teach-plugin prompt is required: the dashboard and each lesson contain
the next action, feedback loop, source links, and proof-of-completion gate.

## Publish the studio

This folder includes an official GitHub Pages Actions workflow. Follow
[PAGES.md](PAGES.md) to connect it to a repository and publish the dashboard as
a static site.
