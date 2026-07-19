# NetForge mock scorecard

Copy this file into the relevant `capstone/work/evidence/day-NN/` directory.
Score only behavior that the round exposed. Use the fixed profile and normalization
rule in [SCORECARD.md](SCORECARD.md); an unobserved row is not a zero.

## Round record

- Date/time:
- Round: Mock A / Mock B / Mock C / Mock D / Day 14 coding + oral
- Prompt or branch:
- Timer length and interruptions:
- Evidence path or recording timestamps:
- Scorer: self / independent
- Independent scorer role (broad label or alias only):
- AI, notes, autocomplete, or reference code used during timer: none / describe

## Observed rows

| Dimension | Weight | Rating (0–4) | Exact quote, code observation, or timestamp |
|---|---:|---:|---|
| C++ correctness/API | 20 |  |  |
| Resource safety | 15 |  |  |
| Concurrency | 15 |  |  |
| Networking/IPC | 15 |  |  |
| Debug/performance | 10 |  |  |
| Storage/reliability | 10 |  |  |
| System design | 10 |  |  |
| Communication/ownership | 5 |  |  |

Included weight:  
Earned included points (`weight × rating / 4`):  
Normalized score (`100 × earned / included weight`):  
Any included zero: yes / no  
Any automatic no-pass condition: yes / no  
Unresolved memory-safety or data-race defect: yes / no  
Result: pass / repeat / provisional self-score only

## Three-line debrief

1. Strongest demonstrated signal:
2. Highest-risk gap, with exact evidence:
3. Next experiment that could change the score:

## Independent calibration, when required

The scorer completes this copy before seeing the learner's score. Record neither
private contact details nor employer/customer material. If the scores differ by
more than ten points, keep the lower score, name the evidence disagreement, and
repeat after targeted practice.
