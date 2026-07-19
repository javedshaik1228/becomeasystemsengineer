# Mock A fixture: frame ownership and hostile length

Use this only for Mock A's 15-minute injected-defect segment, after implementing
the blank-file decoder. The fixture contains two independent faults. Do not read
the whole file looking for suspicious lines first; reproduce each mode, preserve
the first useful evidence, then inspect from that evidence.

```bash
mkdir -p capstone/work/evidence/day-07
cp interview/fixtures/mock-a/mock_a_frame_faults.cpp \
  capstone/work/evidence/day-07/mock-a.cpp
c++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  capstone/work/evidence/day-07/mock-a.cpp -o build/mock-a

build/mock-a oversize
ASAN_OPTIONS=detect_stack_use_after_return=1 build/mock-a lifetime
```

The supplied version must fail both modes: `oversize` reports work performed
before rejection, and `lifetime` produces a sanitizer failure or corrupted
payload. Fix one causal mechanism at a time. Both original commands must then
exit zero under ASan/UBSan.

In the debrief, state the ownership of the decoded payload, the point at which an
untrusted length becomes safe to use, the complexity bound, and one regression
test for truncated input. Save the initial reports, final output, and explanation
beside the copied source.

