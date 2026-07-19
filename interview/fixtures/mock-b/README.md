# Mock B fixture: shutdown hang

This is the deterministic diagnosis segment for Mock B. It starts one worker
blocked in socket input, requests shutdown by immediately joining that worker,
and leaves the event that could wake it on the wrong side of the join.

```bash
mkdir -p capstone/work/evidence/day-10
cp interview/fixtures/mock-b/mock_b_shutdown_hang.cpp \
  capstone/work/evidence/day-10/mock-b.cpp
c++ -std=c++20 -O0 -g -pthread -Wall -Wextra -Wpedantic \
  capstone/work/evidence/day-10/mock-b.cpp -o build/mock-b

timeout 3s build/mock-b
strace -ff -tt -e trace=network,read,write,futex \
  -o capstone/work/evidence/day-10/mock-b-strace \
  timeout 3s build/mock-b
```

The first command should time out. For the debugger view, run
`gdb --args build/mock-b`, start it, interrupt after the two status lines, and use
`thread apply all bt`. On macOS, use `perl -e 'alarm 3; exec @ARGV' build/mock-b`
and LLDB's `thread backtrace all`; Instruments or `sample` can replace `strace`.

Fix the cancellation/ownership order without detaching, polling a flag around a
blocking call, or relying on a longer sleep. The same binary must then exit zero
without `timeout`. In the debrief, draw who owns each endpoint, the event that
wakes the blocked syscall, the join dependency, and how the design changes for a
real listening socket plus many accepted clients.
