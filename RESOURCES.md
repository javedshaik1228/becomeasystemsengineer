# NetForge learning resources

This is the curated source shelf for the two-week NetForge course. It favors specifications, standards, official manuals, and primary engineering guidance. The annotations tell you what each source is good for and how deeply to read it during the sprint.

## Knowledge

### Course field guides

- [C++ lifetime and ownership](reference/0001-cpp-lifetime-ownership.html) - Core. The working model for object lifetime, RAII, move-only resource handles, borrowed views, and exception safety. Read before Days 1-3; use its trap table for interview drills.
- [Concurrency and the C++ memory model](reference/0002-concurrency-memory-model.html) - Core. Data races, happens-before, mutexes, condition variables, bounded queues, cancellation, atomics, and deadlock prevention. Read before Days 4-5.
- [POSIX I/O and sockets](reference/0003-posix-io-sockets.html) - Core. File descriptors, short I/O, TCP stream framing, nonblocking readiness, backpressure, IPC, and signal-safe shutdown. Read before Days 7-9.
- [Networking protocols](reference/0004-networking-protocols.html) - Core plus clearly marked stretch. Ethernet/IP/TCP/UDP packet reasoning, byte order, CIDR, routing, captures, and security vocabulary. Read before Days 6, 12, and 13.
- [Debugging and performance](reference/0005-debugging-performance.html) - Core. Evidence-first debugging, sanitizers, debuggers, syscall tracing, profiling, and benchmark hygiene. Read before Day 10 and reuse during the capstone.
- [Storage and reliability](reference/0006-storage-reliability.html) - Core reliability mechanics plus specialist orientation. Page cache, `fsync`, atomic replacement, WAL recovery, failure models, and a bounded introduction to NAS protocols. Read before Days 11 and 13.
- [Systems design interview guide](reference/0007-system-design-interview.html) - Core. A timed interview structure for a bounded C++ network service, capacity math, overload control, observability, failure analysis, and role-specific branches. Read before Days 13-14.

### C++ language and implementation

- [Current ISO C++ standard status](https://isocpp.org/std/the-standard) - Authoritative orientation. Confirms the published standard and edition; this course intentionally uses an explicit C++20 baseline even when a compiler defaults to something newer.
- [Public C++ working draft](https://eel.is/c++draft/) - Primary normative reference. Search it when an interview claim depends on exact language semantics; do not try to read it front to back.
- [Object lifetime](https://eel.is/c++draft/basic.life) - Primary source for when an object's lifetime begins and ends and when a pointer may be used. Pair with the first field guide and implement the examples.
- [`unique_ptr`](https://eel.is/c++draft/unique.ptr), [`span`](https://eel.is/c++draft/views.span), and [`variant`](https://eel.is/c++draft/variant) - Primary sources for unique ownership, non-owning contiguous views, and closed typed alternatives. Contrast lifetime behavior and invalid states aloud.
- [Threads](https://eel.is/c++draft/thread), [multithreaded executions and data races](https://eel.is/c++draft/intro.multithread), [atomic ordering](https://eel.is/c++draft/atomics.order), and [`jthread`](https://eel.is/c++draft/thread.jthread.class) - Primary concurrency anchors. Read selected clauses only after the field guide gives you a map.
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - Maintained engineering guidance. Focus on the Resource (R), Concurrency (CP), and Performance (Per) sections; treat rules as design prompts, not substitutes for measurement.
- [GCC C++ status](https://gcc.gnu.org/projects/cxx-status.html) and [Clang C++ status](https://clang.llvm.org/cxx_status.html) - Official implementation matrices. Use when a feature behaves differently across Linux, WSL, and macOS toolchains.

### POSIX, Linux, and macOS systems APIs

- [POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/) - The portable contract. Use it to separate POSIX behavior from Linux-only conveniences.
- [`socket(2)`](https://man7.org/linux/man-pages/man2/socket.2.html), [`socket(7)`](https://man7.org/linux/man-pages/man7/socket.7.html), and [`tcp(7)`](https://man7.org/linux/man-pages/man7/tcp.7.html) - High-trust Linux API documentation. Read with a terminal open; verify return values, errors, options, and shutdown behavior in code.
- [`close(2)`](https://man7.org/linux/man-pages/man2/close.2.html) and [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html) - Authoritative Linux details for descriptor release and readiness. Use for the `EINTR`, reuse, timeout, and event-mask failure cases in Days 2 and 8.
- [POSIX `socketpair()`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/socketpair.html) - Portable process-boundary contract. Use on Day 9 to distinguish a real inherited process endpoint from an in-process thread simulation.
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html) and [Apple `kqueue`/`kevent`](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kevent.2.html) - Platform readiness APIs. Core outcome is explaining readiness and backpressure; production-grade reactor implementation is stretch.
- [`pthreads(7)`](https://man7.org/linux/man-pages/man7/pthreads.7.html), [`unix(7)`](https://man7.org/linux/man-pages/man7/unix.7.html), [`pipe(7)`](https://man7.org/linux/man-pages/man7/pipe.7.html), [POSIX shared memory](https://man7.org/linux/man-pages/man7/shm_overview.7.html), and [`mmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html) - Focused references for thread and IPC tradeoffs. Use the comparison table in the POSIX field guide before choosing one.
- [Signal safety](https://man7.org/linux/man-pages/man7/signal-safety.7.html), [resource limits](https://man7.org/linux/man-pages/man2/getrlimit.2.html), and [`sendfile(2)`](https://man7.org/linux/man-pages/man2/sendfile.2.html) - Operational details that distinguish a robust service from a toy. Signal handlers should remain minimal; zero-copy is optional optimization, not a default design answer.

### Networking standards

- [TCP, RFC 9293](https://www.rfc-editor.org/rfc/rfc9293.html) - Current core TCP specification. Use to settle state, sequence-space, and connection-semantics questions; the course expects explanatory fluency, not RFC memorization.
- [UDP, RFC 768](https://www.rfc-editor.org/rfc/rfc768.html), [IPv4, RFC 791](https://www.rfc-editor.org/rfc/rfc791.html), [IPv6, RFC 8200](https://www.rfc-editor.org/rfc/rfc8200.html), [ARP, RFC 826](https://www.rfc-editor.org/rfc/rfc826.html), and [ICMP, RFC 792](https://www.rfc-editor.org/rfc/rfc792.html) - Protocol anchors for packet parsing and path reasoning. Read diagrams and field definitions; validate lengths before interpreting payloads.
- [TLS 1.3, RFC 8446](https://www.rfc-editor.org/rfc/rfc8446.html) - Security-layer orientation. Core outcome is knowing what TLS protects and where it sits; implementing TLS or cryptography is out of scope.
- [OSPFv2, RFC 2328](https://www.rfc-editor.org/rfc/rfc2328.html) and [BGP-4, RFC 4271](https://www.rfc-editor.org/rfc/rfc4271.html) - Stretch references for L2/L3-oriented roles. Learn purpose, state, and failure vocabulary only during this sprint; do not claim protocol implementation mastery.

### Storage, reliability, and NAS orientation

- [Linux VFS documentation](https://docs.kernel.org/filesystems/vfs.html) - Kernel documentation for the filesystem abstraction and object model. Use to locate the page cache and filesystem boundary in a design explanation.
- [`fsync(2)`](https://man7.org/linux/man-pages/man2/fsync.2.html) and [`rename(2)`](https://man7.org/linux/man-pages/man2/rename.2.html) - Core durability primitives. Read the errors and directory-entry caveats; test a WAL and atomic-replace path under injected failures.
- [NFSv4.1, RFC 8881](https://www.rfc-editor.org/rfc/rfc8881.html) and [Microsoft SMB2/3 open specification](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/5606ad47-5ee0-437a-817e-70c366052962) - Authoritative specialist references. In two weeks, target protocol purpose, state, caching/locking, recovery, and diagnostic vocabulary - not production implementation expertise.
- [iSCSI, RFC 7143](https://www.rfc-editor.org/rfc/rfc7143.html) and [NVMe specifications](https://nvmexpress.org/specifications/) - Stretch block-storage orientation for role conversations. Read only if a target job explicitly emphasizes the storage path.

### Build, test, debug, and profile

- [CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html), [CTest tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/Testing%20and%20CTest.html), [CTest manual](https://cmake.org/cmake/help/latest/manual/ctest.1.html), and [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) - Official build/test workflow. Prefer repeatable configurations and finite test timeouts over IDE-only settings.
- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html), [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html), and [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) - Official diagnostics. Maintain separate ASan/UBSan and TSan configurations; a clean run increases confidence but never proves correctness.
- [Redis serialization protocol](https://redis.io/docs/latest/develop/reference/protocol-spec/) and [PUBLISH](https://redis.io/docs/latest/commands/publish/) - Official wire-format and command contracts. Use in matching capstone Step 9 to test exact RESP2 bytes and integer replies without putting Redis on the matching thread.
- [Apache Kafka documentation](https://kafka.apache.org/documentation/) - Official event-streaming guarantees and configuration reference. Use in matching capstone Step 9 to separate adapter delivery policy from core matching correctness.
- [`clang-tidy`](https://clang.llvm.org/extra/clang-tidy/) - Official static-analysis reference. Keep checks intentional and warnings actionable rather than enabling every check.
- [GDB manual](https://sourceware.org/gdb/current/onlinedocs/gdb.html), [LLDB tutorial](https://lldb.llvm.org/use/tutorial.html), [`strace`](https://man7.org/linux/man-pages/man1/strace.1.html), and [`perf` tutorial](https://perfwiki.github.io/main/tutorial/) - Primary tool documentation for Linux/WSL and macOS-adjacent workflows. Practice one real crash, hang, syscall trace, and profile instead of memorizing commands.
- [libFuzzer](https://llvm.org/docs/LibFuzzer.html) - Official in-process coverage-guided fuzzing reference. Use on Day 12 only after deterministic boundary tests exist; preserve a minimized reproducer for every finding.
- [Valgrind manual](https://valgrind.org/docs/manual/manual.html) and [Memcheck](https://valgrind.org/docs/manual/mc-manual.html) - Useful Linux fallback and complementary instrumentation. Expect higher overhead; use sanitizers first when supported.

### Reliable service design

- [Google SRE: Handling overload](https://sre.google/sre-book/handling-overload/) - Primary operational guidance for admission control, load shedding, and graceful degradation. Translate each idea into an explicit NetForge queue or connection policy.
- [Google SRE: Addressing cascading failures](https://sre.google/sre-book/addressing-cascading-failures/) - Primary guidance for retry amplification, load shedding, and recovery. Use during Days 12–13 failure injection to connect local backpressure to system-wide behavior.
- [Google SRE: Monitoring distributed systems](https://sre.google/sre-book/monitoring-distributed-systems/) - High-trust model for useful signals. Instrument latency distributions, traffic, errors, and saturation rather than dashboarding everything.
- [AWS Builders' Library: load shedding](https://aws.amazon.com/builders-library/using-load-shedding-to-avoid-overload/) and [timeouts, retries, backoff, and jitter](https://aws.amazon.com/builders-library/timeouts-retries-and-backoff-with-jitter/) - Production engineering essays. Use them to reason about bounded work and retry amplification in system-design interviews.

## Wisdom communities

These are places to observe how experienced practitioners reason. Search archives first, reduce problems to a minimal reproducible example, and verify advice against the primary sources above.

- [ISO C++ discussion forums](https://isocpp.org/about/forums) - Standards-oriented, public discussion channels. Best for understanding design rationale and proposals; too specialized for routine debugging questions.
- [C++ Alliance Slack](https://cppalliance.org/slack/) - Practitioner community around modern C++ and networking libraries. Ask focused questions with compiler, standard level, platform, and a small example; community answers are guidance, not normative rules.
- [Stack Overflow: C++](https://stackoverflow.com/questions/tagged/c%2b%2b), [Linux](https://stackoverflow.com/questions/tagged/linux), and [sockets](https://stackoverflow.com/questions/tagged/sockets) - Moderated archives with many precise implementation questions. Prefer highly voted answers that cite standards/manuals, and re-check old answers against current specifications.
- [Unix & Linux Stack Exchange](https://unix.stackexchange.com/) - Strong archive for command-line, syscall, and operating-system behavior. Include kernel/distribution versions and observable evidence in questions.
- [IETF mailing lists](https://www.ietf.org/participate/lists/) - Protocol-development discussions. Read the relevant RFC and list archive before participating; use for specialist depth, not beginner help-desk support.
- [Linux netdev archives](https://lore.kernel.org/netdev/) - Advanced kernel-networking discussion and patch review. This is a stretch observation source for understanding production tradeoffs; it is not required for the two-week core.

## Gaps

- The linked job pages can change or close, and employers rarely publish their exact interview loops. The curriculum maps recurring role requirements, not employer-verified question banks.
- No local mentor, study group, or preferred live community has been recorded. A weekly peer mock would add feedback that documents cannot provide.
- Vendor NOS/ASIC SDK details and some NAS implementation behavior are proprietary or platform-specific. Public RFC fluency does not equal vendor implementation experience.
- Linux/WSL is the primary hands-on path. macOS parity for `kqueue`, Instruments, sandbox behavior, and filesystem durability still needs validation on a real Mac.
- Two weeks can build demonstrable fundamentals and interview fluency; it cannot honestly confer mastery of NFS/SMB internals, routing-protocol implementation, kernel networking, cryptography, or distributed storage.
- Specifications define contracts, not all production failure modes. Keep an evidence log from experiments, fault injection, packet captures, sanitizer runs, and mocks to close the practice gap.
