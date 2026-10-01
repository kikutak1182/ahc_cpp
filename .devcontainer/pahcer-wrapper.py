#!/usr/bin/python3
"""Add the local C++ test setup after the upstream pahcer initializes it."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys


PAHCER_BINARY = "/root/.cargo/bin/pahcer"
VIS_BUILD = '''[[test.compile_steps]]
program = "cargo"
args = ["build", "--release", "--bin", "vis"]
current_dir = "./tools"

'''
SCORE_REGEX = r"score_regex = '(?m)^\s*Score\s*=\s*(?P<score>\d+)\s*$'"


def configure_cpp(config_path):
    config = config_path.read_text(encoding="utf-8")
    # Keep the test-section comments together when the upstream template has them.
    boundary = re.search(r"(?m)^# =+\n#\s+TEST STEPS\n# =+\n", config)
    if boundary is None:
        boundary = re.search(r"(?m)^\[\[test\.test_steps\]\]", config)
    if boundary is None:
        raise ValueError("generated config has no test.test_steps section")
    config = config[:boundary.start()] + VIS_BUILD + config[boundary.start():]
    config = re.sub(
        r"(?m)^score_regex\s*=.*$", lambda _: SCORE_REGEX, config, count=1
    )
    config_path.write_text(config, encoding="utf-8")


def main():
    args = sys.argv[1:]
    command = [PAHCER_BINARY, *args]
    if not args or args[0] != "init" or "--help" in args or "-h" in args:
        os.execv(PAHCER_BINARY, command)

    config_path = Path("pahcer_config.toml")
    existed = config_path.exists()
    result = subprocess.run(command)
    if result.returncode != 0:
        return result.returncode if result.returncode > 0 else 128 - result.returncode
    if existed:
        return 0

    # Upstream validates the arguments first; only inspect the options we need.
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument("-p", "--problem")
    parser.add_argument("-o", "--objective")
    parser.add_argument("-l", "--lang", "--language", dest="language")
    parser.add_argument("-i", "--interactive", action="store_true")
    options, _ = parser.parse_known_args(args[1:])
    if options.language == "cpp" and not options.interactive:
        try:
            configure_cpp(config_path)
        except (OSError, ValueError) as error:
            print(f"Error: failed to configure pahcer: {error}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
