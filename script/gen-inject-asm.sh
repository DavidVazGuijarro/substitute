#!/bin/sh
cat <<END
/* Generated by script/gen-inject-asm.sh.  The relevant source is in-tree (make
 * out/darwin-inject-asm.S), but this file has been checked in too, in case
 * your C compiler doesn't support all of these architectures.
 * This file contains code for 4 architectures in one text page; it's remapped
 * into the target process and the appropriate thunk executed.  Having ARM code
 * here on x86 and whatnot is currently pointless (and use of that code is
 * disabled in case any future Rosetta-like emulator breaks naive attempts to
 * inject into foreign-architecture processes), but we need two architectures
 * anyway, so the rest are included in case doing so is useful someday. */
.align 14
.globl _inject_page_start
_inject_page_start:
END
for i in x86_64 i386 arm arm64; do
    echo ".align 2"
    echo ".globl _inject_start_$i"
    echo "_inject_start_$i:"
    printf  ".byte "
    xxd -i < out/inject-asm-raw-$i.bin | xargs echo
done
