# Linux, WSL, and macOS setup

## WSL 2 (recommended on this machine)

WSL 2 is enabled, but no distribution is currently installed. Open an elevated
PowerShell window once and run:

```powershell
wsl --install -d Ubuntu-24.04
```

Restart if Windows requests it, finish the Ubuntu first-run user setup, then open
this folder through VS Code's **WSL: Open Folder in WSL** command. From the WSL
terminal:

```bash
cd /mnt/e/CodexProjects/study-biyatch
./scripts/bootstrap.sh
./scripts/doctor.sh
```

If `/mnt/e` filesystem performance becomes noticeable during repeated builds,
copy the course into your Linux home directory. Do not keep build output on both
sides and compare timings as though they were equivalent.

## Ubuntu/Debian Linux

`bootstrap.sh` installs or checks `build-essential`, CMake, Ninja, GDB, Clang,
LLDB, Valgrind, `strace`, `ltrace`, `perf`, `tcpdump`, and Wireshark CLI tools.
It prints every package operation before running it.

## macOS

Install Xcode command-line tools and Homebrew, then:

```bash
xcode-select --install
brew install cmake ninja llvm lldb wireshark
export PATH="$(brew --prefix llvm)/bin:$PATH"
./scripts/doctor.sh
```

Persist that LLVM path in your shell profile before Day 10 so `clang++` and
`clang-tidy` resolve to the Homebrew toolchain used by the course gates.

Use LLDB where a lesson says GDB. Use `dtruss`/Instruments where Linux uses
`strace`/`perf`. ThreadSanitizer and AddressSanitizer work with Apple Clang, but
Linux-only `epoll`, `perf`, and some `/proc` exercises have a `poll`/Instruments
alternative in the lesson.

## Manual minimum

If you prefer not to run the bootstrap script, the Core lane needs:

- GCC 12+ or Clang 15+ with C++20 support, plus Clang with libFuzzer support
  for the Day 12 Core gate;
- CMake 3.24+ and Ninja (Make also works);
- GDB or LLDB;
- `tcpdump` and either Wireshark or `tshark`;
- AddressSanitizer, UndefinedBehaviorSanitizer, and ThreadSanitizer support.
- `clang-tidy` for the Day 10/14 static-analysis gates and Python 3 for the
  practical diagnostic and low-latency role-screen runner.

Run `./scripts/doctor.sh` after any manual setup. It reports optional tools as
warnings and fails only when a Core requirement is missing.
