"""CLI and real-PTY integration checks; no interactive terminal required."""
import errno
import fcntl
import os
from pathlib import Path
import pty
import select
import signal
import struct
import subprocess
import sys
import tempfile
import termios
import time


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    binary = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="izumi-cli-") as temp:
        env = dict(os.environ, HOME=temp, TERM="xterm")
        for option, expected in [("--help", b"Usage:"), ("-h", b"Usage:"),
                                 ("--version", b"izumi "), ("-v", b"izumi ")]:
            result = subprocess.run([binary, option], capture_output=True, env=env, timeout=5)
            check(result.returncode == 0 and expected in result.stdout, option)
        trace = Path(temp) / "trace.log"
        trace.write_text("Kanata\t0004\nI\t0\t0\t0\nL\t0\t0\t0x10: nop\n"
                         "S\t0\t0\tF\nC\t2\nR\t0\t0\t0\n")
        config = Path(temp) / ".config" / "izumi"
        config.mkdir(parents=True)
        (config / "config").write_text("newpanel\n")
        master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
        process = subprocess.Popen([binary, str(trace)], stdin=slave, stdout=slave,
                                   stderr=slave, env=env, start_new_session=True,
                                   preexec_fn=lambda: fcntl.ioctl(0, termios.TIOCSCTTY, 0))
        os.close(slave)
        output = bytearray()

        def drain_until(marker):
            deadline = time.monotonic() + 5
            start = len(output)
            while time.monotonic() < deadline:
                if marker in output[start:]:
                    return
                if not select.select([master], [], [], 0.1)[0]:
                    continue
                try:
                    data = os.read(master, 65536)
                except OSError as exc:
                    if exc.errno == errno.EIO:
                        break
                    raise
                if not data:
                    break
                output.extend(data)
            raise AssertionError(f"PTY did not display {marker!r}: {bytes(output)!r}")

        try:
            drain_until(b"trace.log")
            fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack("HHHH", 18, 70, 0, 0))
            os.kill(process.pid, signal.SIGWINCH)
            os.write(master, b":findinst nop\n:set show_empty_rows off\n")
            drain_until(b"off")
            os.write(master, b":quit\n")
            while process.poll() is None:
                if select.select([master], [], [], 0.1)[0]:
                    try:
                        output.extend(os.read(master, 65536))
                    except OSError as exc:
                        if exc.errno != errno.EIO:
                            raise
                if len(output) > 1024 * 1024:
                    raise AssertionError("excessive terminal output")
                # wait() below has a timeout; poll loop also needs one.
                if time.monotonic() > started + 15:
                    raise AssertionError("quit timed out")
            check(process.wait(timeout=5) == 0, bytes(output).decode(errors="replace"))
            check(b"ERROR: AddressSanitizer" not in output and b"runtime error:" not in output,
                  "sanitizer diagnostic in terminal output")
            check(b"No result" not in output,
                  "CLI trace was not loaded into the focused panel after config-created panels")
        finally:
            if process.poll() is None:
                process.kill()
            process.wait(timeout=5)
            os.close(master)
    print("PASS: CLI help/version and PTY startup/resize/quit")


if __name__ == "__main__":
    started = time.monotonic()
    main()
