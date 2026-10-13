/*
 * Perfetto slices on ftrace's trace_marker (ajrx fork, branch aj; kraken-antijam PLAN.md D37).
 *
 * Off unless daq_chain_config.ini has [trace] marker = <path> (ajrx writes it when run.trace is on)
 * and that file opens; then every call is one write() of atrace's text format:
 *   B|0|name            begin a slice on the calling thread (tgid 0: Perfetto takes it from the kernel)
 *   E|0                 end it
 *   G|pid|track|name|c  begin a slice on the process's named track (c tells overlapping ones apart)
 *   H|pid|track|c       end it
 * Off, each call is a branch.
 */
#ifndef ATRACE_H
#define ATRACE_H

#include <stdbool.h>

/* Reads [trace] marker from the ini; `name` (e.g. "rebuffer") labels this program's buffer slices. */
void atrace_init(const char* ini_fname, const char* name);
bool atrace_enabled(void);
/* This program's name, as given to atrace_init. */
const char* atrace_name(void);
void atrace_begin(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void atrace_end(void);
void atrace_track_begin(const char* track, int cookie, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
void atrace_track_end(const char* track, int cookie);
/* Names the calling thread (Perfetto shows the kernel's thread names), only when tracing. */
void atrace_thread_name(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
