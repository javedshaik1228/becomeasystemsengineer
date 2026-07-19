#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGES=()

note() {
  printf '[bootstrap] %s\n' "$*"
}

warn() {
  printf '[bootstrap] warning: %s\n' "$*" >&2
}

have() {
  command -v "$1" >/dev/null 2>&1
}

add_package() {
  local candidate="$1"
  local existing
  for existing in "${PACKAGES[@]}"; do
    if [[ "$existing" == "$candidate" ]]; then
      return
    fi
  done
  PACKAGES+=("$candidate")
}

confirm_install() {
  local reply
  if [[ ! -t 0 ]]; then
    warn 'No interactive terminal is available, so no packages will be installed.'
    return 1
  fi
  printf '[bootstrap] Proceed with this package-manager operation? [y/N] '
  read -r reply
  [[ "$reply" =~ ^[Yy]([Ee][Ss])?$ ]]
}

run_doctor() {
  note 'Package step complete. Running the read-only environment doctor.'
  bash "$ROOT_DIR/scripts/doctor.sh"
}

bootstrap_debian() {
  if ! have g++ && ! have clang++; then add_package build-essential; fi
  if ! have make; then add_package build-essential; fi
  if ! have cmake; then add_package cmake; fi
  if ! have ninja; then add_package ninja-build; fi
  if ! have gdb; then add_package gdb; fi
  if ! have clang++; then add_package clang; fi
  if ! have clangd; then add_package clangd; fi
  if ! have clang-format; then add_package clang-format; fi
  if ! have clang-tidy; then add_package clang-tidy; fi
  if ! have lldb; then add_package lldb; fi
  if ! have valgrind; then add_package valgrind; fi
  if ! have strace; then add_package strace; fi
  if ! have ltrace; then add_package ltrace; fi
  if ! have tcpdump; then add_package tcpdump; fi
  if ! have tshark; then add_package tshark; fi
  if ! have python3; then add_package python3; fi
  if ! have node; then add_package nodejs; fi
  if ! have rg; then add_package ripgrep; fi

  if ((${#PACKAGES[@]} == 0)); then
    note 'All recommended Debian/Ubuntu commands are already present; nothing to install.'
    run_doctor
    return
  fi

  local apt_command=(apt-get)
  if ((EUID != 0)); then
    if ! have sudo; then
      warn 'Installation needs root privileges, but sudo is not available.'
      warn 'Install the packages listed below manually, then run scripts/doctor.sh.'
      printf '  %s\n' "${PACKAGES[@]}" >&2
      return 1
    fi
    apt_command=(sudo apt-get)
  fi

  note 'The following packages are missing from the recommended Ubuntu/Debian toolset:'
  printf '  %s\n' "${PACKAGES[@]}"
  note 'With approval, bootstrap will run exactly these two package-manager commands:'
  printf '  '
  printf '%q ' "${apt_command[@]}" update
  printf '\n  '
  printf '%q ' "${apt_command[@]}" install -y "${PACKAGES[@]}"
  printf '\n'

  if ! confirm_install; then
    note 'Installation declined; the system was not changed.'
    run_doctor
    return
  fi

  "${apt_command[@]}" update
  "${apt_command[@]}" install -y "${PACKAGES[@]}"
  run_doctor
}

brew_formula_missing() {
  ! brew list --versions "$1" >/dev/null 2>&1
}

bootstrap_macos() {
  if ! xcode-select -p >/dev/null 2>&1; then
    warn 'Xcode Command Line Tools are missing.'
    warn 'Run `xcode-select --install`, approve Apple’s installer, then rerun bootstrap.'
  fi

  if ! have brew; then
    warn 'Homebrew is not installed; bootstrap will not install it or run a remote installer.'
    warn 'Install Homebrew from https://brew.sh/ if wanted, or install the tools in SETUP.md manually.'
    run_doctor
    return
  fi

  if ! have cmake && brew_formula_missing cmake; then add_package cmake; fi
  if ! have ninja && brew_formula_missing ninja; then add_package ninja; fi
  if { ! have clangd || ! have clang-format || ! have clang-tidy; } && brew_formula_missing llvm; then add_package llvm; fi
  if ! have lldb && brew_formula_missing lldb; then add_package lldb; fi
  if ! have tshark && brew_formula_missing wireshark; then add_package wireshark; fi
  if ! have python3 && brew_formula_missing python; then add_package python; fi
  if ! have node && brew_formula_missing node; then add_package node; fi
  if ! have rg && brew_formula_missing ripgrep; then add_package ripgrep; fi

  if ((${#PACKAGES[@]} == 0)); then
    note 'All recommended Homebrew formulas are already present; nothing to install.'
    run_doctor
    return
  fi

  note 'The following Homebrew formulas are missing from the recommended toolset:'
  printf '  %s\n' "${PACKAGES[@]}"
  note 'With approval, bootstrap will run exactly this package-manager command:'
  printf '  brew install'
  printf ' %q' "${PACKAGES[@]}"
  printf '\n'

  if ! confirm_install; then
    note 'Installation declined; the system was not changed.'
    run_doctor
    return
  fi

  brew install "${PACKAGES[@]}"
  run_doctor
}

case "$(uname -s)" in
  Darwin)
    bootstrap_macos
    ;;
  Linux)
    if [[ -r /etc/os-release ]]; then
      # shellcheck disable=SC1091
      . /etc/os-release
    fi
    distro_hint="${ID:-} ${ID_LIKE:-}"
    if [[ "$distro_hint" == *debian* || "$distro_hint" == *ubuntu* ]]; then
      bootstrap_debian
    else
      warn "Automatic package mapping is limited to Ubuntu/Debian; detected: ${PRETTY_NAME:-Linux}."
      warn 'No package-manager command was run. Install the tools in SETUP.md manually.'
      run_doctor
    fi
    ;;
  *)
    warn "Unsupported host: $(uname -s). Use Ubuntu/Debian, WSL, or macOS."
    exit 2
    ;;
esac
