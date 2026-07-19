# Solo debugging fixtures

These fixtures make the injected portions of Mocks A and B runnable without a
second person. They are intentionally defective, self-contained, and excluded
from the NetForge build. Copy a fixture into the day's evidence directory before
editing it so the original remains a repeatable interview prompt.

- [Mock A: frame ownership and hostile length](mock-a/README.md)
- [Mock B: shutdown hang](mock-b/README.md)

During the timer, preserve the first failing output or stack before changing
code. A pass means the original reproducer exits successfully after a causal fix;
deleting an assertion, adding an arbitrary sleep, or increasing a timeout is not
a fix.

