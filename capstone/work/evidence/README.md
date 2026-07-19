# Evidence, not checkboxes

Create `day-NN/` as each lesson requests. Save small, reviewable proof:

- the exact command and relevant output from a passing/failing test;
- a sanitizer, debugger, syscall trace, packet capture summary, or profile;
- a short hypothesis → experiment → conclusion note;
- the day's spoken answer score or design rubric.

Do not commit huge captures, binaries, core files, or raw benchmark dumps. Extract
the decisive lines and record how they changed your model. Browser progress can
be reset; these artifacts are durable practice evidence, but they are not a
Teach learning record by themselves. After evidence demonstrates a non-trivial
insight or corrected misconception, use `scripts/record-learning.sh`
to create the short record that should steer future study.
