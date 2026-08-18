# Building on Linux

The intro toolchain is Windows-only, but all of it runs on Linux inside a
container. This directory holds a `Containerfile` for podman and a `Makefile`
that drives it. Nothing here is needed on Windows — the `.bat` files in the
repository root keep working unchanged.

## Usage

```sh
make -C linux-cross fast     # release build, seconds       -> out/intro-fast.exe
make -C linux-cross slow     # release build, minutes       -> out/intro-slow.exe
make -C linux-cross debug    # windowed, no music, GLCHECK  -> out/intro-debug.exe
```

The first invocation builds the container image (a few minutes, ~870 MB) and
tags it `localhost/bepct4k-build`; later builds reuse it and take as long as
the linker does. The image is rebuilt automatically whenever `Containerfile` or
`build.sh` change.

Intermediates go to `obj/` and results to `out/` in the repository root, the
same places the Windows build uses, so `out/report-fast.html` and friends are
where you expect them. Under rootless podman the files belong to you.

Other targets:

```sh
make -C linux-cross image      # (re)build the image explicitly
make -C linux-cross shell      # interactive shell in the container, repo at /work
make -C linux-cross clean      # remove obj/ and out/
make -C linux-cross distclean  # also drop the image
```

Useful variables: `PODMAN` (default `podman`), `IMAGE`, `PODMAN_RUN_FLAGS`
(e.g. `--userns=keep-id` when running podman as root), and `MOUNT_OPTS`
(default `:Z` for SELinux hosts).

## What runs where

| tool            | how it runs on Linux                                    |
| --------------- | ------------------------------------------------------- |
| nasm            | native, from Debian; `-fwin32` output is the same COFF   |
| shader_minifier | upstream ships a .NET PE only, so it runs under `mono`   |
| crinkler        | win64 binary under 64-bit wine — no substitute exists    |

The two downloaded tools are pinned by URL and SHA-256 in the `Containerfile`,
at the versions `deps/get-deps.ps1` uses, and are baked into the image, so a
build needs no network access.

## The syswow64 detail

Crinkler resolves each imported DLL through the Windows `PATH` and reads its
export table, and it only accepts 32-bit DLLs — it skips PE32+ files with a
warning and then fails with `Cannot find DLL 'OPENGL32.DLL'`. On real Windows
it finds the 32-bit copies because it rewrites `system32` to `syswow64` in
every `PATH` entry.

This image has no 32-bit wine (crinkler itself is 64-bit, and nothing here runs
the produced exe), so instead of pulling in all of i386 wine it extracts just
the five DLLs the intro imports from — `opengl32`, `winmm`, `kernel32`,
`user32`, `gdi32` — out of the `libwine:i386` package and drops them into the
prefix's `syswow64`. Nothing ever loads them; they are only read as files.

Crinkler unions those exports with its own built-in table of known Windows
exports before picking a collision-free import hash, so wine's export lists are
adequate here. If you add a library to `LIBS` in `build.sh`, add the matching
DLL to `SYSWOW64_DLLS` in the `Containerfile` too.

## What is not covered

`dump-music.bat` is not ported: it *runs* the 32-bit exe it links, which would
require i386 wine (roughly +800 MB to the image). If you need `music.raw` for
kiyanka, build it on Windows, or write a native Linux renderer around
`__4klang_render`.

Kiyanka itself is unaffected — it is a normal CMake project (its
`CMakeLists.txt` currently only wires up the Windows audio backend).
