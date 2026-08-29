#!/usr/bin/env python3
import argparse
import ipaddress
import re
import socket
import subprocess
import sys


WIRED_PREFIXES = ("eth", "en", "eno", "enp", "ens", "enx")
SKIP_PREFIXES = ("lo", "docker", "br-", "veth", "virbr", "tailscale", "tun", "tap")


def run_ip_brief():
    try:
        result = subprocess.run(
            ["ip", "-o", "-4", "addr", "show", "up"],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError:
        return None, "command not found: ip"

    if result.returncode != 0:
        return None, result.stderr.strip() or f"ip command failed: {result.returncode}"
    return result.stdout.splitlines(), None


def parse_ip_lines(lines):
    entries = []
    pattern = re.compile(r"^\d+:\s+([^ ]+)\s+inet\s+([0-9.]+/\d+)")
    for line in lines:
        match = pattern.search(line)
        if not match:
            continue
        name = match.group(1).split("@", 1)[0]
        cidr = match.group(2)
        ip = cidr.split("/", 1)[0]
        entries.append((name, cidr, ip))
    return entries


def is_likely_wired(name):
    if name.startswith(SKIP_PREFIXES):
        return False
    return name.startswith(WIRED_PREFIXES)


def local_ip_for_peer(peer, port):
    # UDP connect performs route selection without sending data.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.connect((peer, port))
        return sock.getsockname()[0]


def main():
    parser = argparse.ArgumentParser(
        description="Show the Jetson IPv4 address to use as the UDP target host."
    )
    parser.add_argument(
        "--peer",
        help="upper-computer IPv4 address. If set, print the Jetson IP used to reach it.",
    )
    parser.add_argument(
        "--peer-port",
        type=int,
        default=8113,
        help="upper-computer UDP port used only for route selection.",
    )
    parser.add_argument(
        "--dvl-port",
        type=int,
        default=8114,
        help="hal_dvl_node UDP listening port.",
    )
    args = parser.parse_args()

    if args.peer:
        try:
            ipaddress.ip_address(args.peer)
            selected_ip = local_ip_for_peer(args.peer, args.peer_port)
        except OSError as exc:
            print(f"failed to select route to peer {args.peer}: {exc}", file=sys.stderr)
            return 2
        except ValueError as exc:
            print(f"invalid --peer address: {exc}", file=sys.stderr)
            return 2

        print(f"Jetson IP used to reach upper computer {args.peer}: {selected_ip}")
        print(
            "Use this from the upper computer:\n"
            f"  python3 dvl_acoustic_udp_test.py cycle --host {selected_ip} "
            f"--port {args.dvl_port} --local-port 8113"
        )
        return 0

    lines, error = run_ip_brief()
    if error:
        print(error, file=sys.stderr)
        print("Fallback command: hostname -I", file=sys.stderr)
        return 3

    entries = parse_ip_lines(lines)
    if not entries:
        print("no active IPv4 address found", file=sys.stderr)
        return 1

    print("Active IPv4 addresses:")
    for name, cidr, ip in entries:
        marker = "likely wired" if is_likely_wired(name) else "check"
        print(f"  {name:<12} {cidr:<18} {marker}")

    wired = [(name, cidr, ip) for name, cidr, ip in entries if is_likely_wired(name)]
    if len(wired) == 1:
        _, _, ip = wired[0]
        print(
            "\nUse this from the upper computer:\n"
            f"  python3 dvl_acoustic_udp_test.py cycle --host {ip} "
            f"--port {args.dvl_port} --local-port 8113"
        )
    elif len(wired) > 1:
        print("\nMultiple wired-looking interfaces found.")
        print("Pick the IP on the same subnet as the upper computer.")
        print("Or rerun with: --peer <upper_computer_ip>")
    else:
        print("\nNo wired-looking interface was identified.")
        print("Pick the IP on the same subnet as the upper computer.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
