/* Perfetto slices on ftrace's trace_marker: see atrace.h. */
#define _GNU_SOURCE
#include "atrace.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ini.h"
#include "log.h"

static int fd = -1;
static int pid;
static char prog[32] = "";

static int handler(void* user, const char* section, const char* key, const char* value)
{
    if (strcmp(section, "trace") == 0 && strcmp(key, "marker") == 0)
        snprintf((char*)user, 512, "%s", value);
    return 1;
}

void atrace_init(const char* ini_fname, const char* name)
{
    char marker[512] = "";
    snprintf(prog, sizeof prog, "%s", name);
    ini_parse(ini_fname, handler, marker);
    if (marker[0] == '\0')
        return;
    fd = open(marker, O_WRONLY | O_APPEND | O_CLOEXEC);
    pid = getpid();
    if (fd < 0)
        log_warn("trace marker %s cannot be opened; no slices", marker);
    else
        log_info("tracing to %s", marker);
}

bool atrace_enabled(void) { return fd >= 0; }

const char* atrace_name(void) { return prog; }

static void put(const char* buf, int len)
{
    if (len > 0)
        (void)!write(fd, buf, (size_t)len < 256 ? (size_t)len : 255);
}

void atrace_begin(const char* fmt, ...)
{
    if (fd < 0)
        return;
    char buf[256];
    int n = snprintf(buf, sizeof buf, "B|0|");
    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, sizeof buf - n - 1, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof buf - 2)
        n = sizeof buf - 2;
    buf[n++] = '\n';
    put(buf, n);
}

void atrace_end(void)
{
    if (fd >= 0)
        put("E|0\n", 4);
}

void atrace_track_begin(const char* track, int cookie, const char* fmt, ...)
{
    if (fd < 0)
        return;
    char name[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(name, sizeof name, fmt, ap);
    va_end(ap);
    char buf[256];
    put(buf, snprintf(buf, sizeof buf, "G|%d|%s|%s|%d\n", pid, track, name, cookie));
}

void atrace_track_end(const char* track, int cookie)
{
    if (fd < 0)
        return;
    char buf[256];
    put(buf, snprintf(buf, sizeof buf, "H|%d|%s|%d\n", pid, track, cookie));
}

void atrace_thread_name(const char* fmt, ...)
{
    if (fd < 0)
        return;
    char name[16]; /* the kernel's limit, with the terminating 0 */
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(name, sizeof name, fmt, ap);
    va_end(ap);
    pthread_setname_np(pthread_self(), name);
}
