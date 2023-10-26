#!/usr/bin/python3

import os
import sys
import argparse
import subprocess

# Python 2 support
python2 = sys.version_info.major == 2

# Command line options.
options = None


# Error exception.
class Error(Exception):
    pass


def printBacktrace(line):
    """Print backtrace.
    """
    # Backtrace:0x400e21fe:0x3ffdd520 0x400e23ae:0x3ffdd540 0x400ddbe5:0x3ffdd570 0x400ddda3:0x3ffdd590
    print(line)
    if line.startswith("Backtrace:"):
        line = line[10:]
    frames = line.split(" ")
    for frame in frames:
        addrs = frame.split(":")
        if len(addrs) == 0:
            continue
        addr = addrs[0]

        # ~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line   -pfiaCe .pio/build/esp32dev/firmware.elf 0x400dad5b
        output = subprocess.check_output([options.addr2line, "-pfiCe", options.exe, addr]).decode("latin1").strip()
        # 0x400dad5b: ble_hs_event_reset at /home/ov/.platformio/packages/framework-espidf/components/bt/host/nimble/nimble/nimble/host/src/ble_hs.c:538
        print("{}: {}".format(frame, output))


def main():
    """Main function of this module.
    """
    global options
    usage = """%(prog)s INFILE_or_BACKTRACE_LINE

    Examples:

    Get bracktrace(s) from input logfile:
    > resolve_backtrace.py logfile.txt

    Get backtrace from backtrace line:
    > resolve_backtrace.py Backtrace:0x400e21fe:0x3ffdd520 0x400e23ae:0x3ffdd540 0x400ddbe5:0x3ffdd570 0x400ddda3:0x3ffdd590
"""
    version = "0.0.1"
    parser = argparse.ArgumentParser(usage=usage + "\n(Version " + version + ")\n")
    parser.add_argument("args", nargs="*", help="Input LOGFILE or line starting with \"Backtrace:\".")
    parser.add_argument("-e", "--exe", help="Executable/firmware.elf file.", type=str, default=".pio/build/esp32-target/firmware.elf", metavar="FILE")
    parser.add_argument("-a", "--addr2line", help="addr2line tool to use.", type=str, default="~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line", metavar="FILE")
    parser.add_argument("-v", "--verbose",  default=0, action="count", help="Be more verbose.")
    options = parser.parse_args()
    options.exe = os.path.expanduser(options.exe)
    options.addr2line = os.path.expanduser(options.addr2line)

    try:
        if len(options.args) == 0:
            raise Error("Expecting lofile or backtrace line.")
        if os.path.exists(options.args[0]):
            f = open(options.args[0])
            for line in f:
                if line.startswith("Backtrace:"):
                    printBacktrace(line)
        else:
            line = " ".join(options.args)
            if not line.startswith("Backtrace:"):
                raise Error("Either file not found or backtrace line does not start with \"Backtrace:\"")
            printBacktrace(line)
    except Error as e:
        print("Error: {}".format(e))
        return 1

    return 0

# call main()
if __name__ == "__main__":
    sys.exit(main())



