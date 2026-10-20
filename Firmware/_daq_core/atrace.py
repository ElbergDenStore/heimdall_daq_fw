"""Perfetto slices on ftrace's trace_marker for heimdall's Python modules: the twin of atrace.c.

crparx fork, branch CRPA-Research (Kraken-CRPA PLAN.md D37). Off unless daq_chain_config.ini
has [trace] marker = <path>; see atrace.h for the records.
"""
import ctypes
import os
from configparser import ConfigParser
from contextlib import contextmanager

_fd = None
_pid = os.getpid()
name = ""


def init(ini_fname, prog):
    """Reads [trace] marker; `prog` labels this program's buffer slices and names its main thread."""
    global _fd, name
    name = prog
    parser = ConfigParser()
    parser.read([ini_fname])
    marker = parser.get("trace", "marker", fallback="")
    if not marker:
        return
    try:
        _fd = os.open(marker, os.O_WRONLY | os.O_APPEND | os.O_CLOEXEC)
    except OSError:
        return
    try:  # the kernel's thread name, which Perfetto shows: else every module is "python3"
        ctypes.CDLL(None).prctl(15, prog.encode()[:15], 0, 0, 0)  # PR_SET_NAME
    except (OSError, AttributeError):
        pass


def enabled():
    return _fd is not None


def _write(text):
    global _fd
    if _fd is None:
        return
    try:
        os.write(_fd, text.encode())
    except OSError:  # tracing stopped under us: stop writing, keep running
        _fd = None


def begin(slice_name):
    _write(f"B|0|{slice_name}\n")


def end():
    _write("E|0\n")


@contextmanager
def slice(slice_name):
    begin(slice_name)
    try:
        yield
    finally:
        end()


def track_begin(track, cookie, slice_name):
    _write(f"G|{_pid}|{track}|{slice_name}|{cookie}\n")


def track_end(track, cookie):
    _write(f"H|{_pid}|{track}|{cookie}\n")
