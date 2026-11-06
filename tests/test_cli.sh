#!/bin/sh
if test -z "$PYTHON3"; then
    exit 77
fi
exec "$PYTHON3" "$srcdir/test_cli.py" "$IZUMI_BINARY"
