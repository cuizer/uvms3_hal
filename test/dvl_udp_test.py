#!/usr/bin/env python3
import argparse
import os
import random
import socket
import struct
import sys
import time


REQ_HEADER = b"\x0d\x0c"
RESP_HEADER = b"\x0d\x8c"
DEFAULT_HOST = os.environ.get("DVL_UDP_HOST", "127.0.0.1")

COMMANDS = {
    "disable": 0x00,
    "enable": 0x01,
    "query": 0x02,
}

RESULTS = {
    0x00: "ok",
    0x01: "invalid",
    0x02: "serial_not_ready",
    0x03: "write_failed",
    0x04: "ack_timeout",
    0x05: "nack",
    0x06: "busy",
}


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def make_packet(command: str, seq: int) -> bytes:
    prefix = REQ_HEADER + bytes([COMMANDS[command]]) + struct.pack("<H", seq)
    return prefix + struct.pack("<H", crc16_modbus(prefix))


def decode_response(response: bytes) -> dict:
    if len(response) != 9:
        raise ValueError(f"bad response length: {len(response)}")
    if response[:2] != RESP_HEADER:
        raise ValueError(f"bad response header: {response[:2].hex(' ')}")

    expected_crc = crc16_modbus(response[:7])
    received_crc = struct.unpack("<H", response[7:9])[0]
    if expected_crc != received_crc:
        raise ValueError(
            f"crc mismatch: expected 0x{expected_crc:04x}, got 0x{received_crc:04x}"
        )

    return {
        "command": response[2],
        "result": response[3],
        "enabled": bool(response[4]),
        "seq": struct.unpack("<H", response[5:7])[0],
    }


def send_command(sock: socket.socket, host: str, port: int, command: str, seq: int) -> dict:
    packet = make_packet(command, seq)
    print(
        f"> {command:<7} seq={seq:5d} dst={host}:{port} "
        f"packet={packet.hex(' ')}",
        flush=True,
    )
    sock.sendto(packet, (host, port))
    response, peer = sock.recvfrom(128)
    decoded = decode_response(response)
    decoded["peer"] = peer
    decoded["raw"] = response
    return decoded


def print_response(decoded: dict) -> None:
    result = decoded["result"]
    peer_host, peer_port = decoded["peer"]
    print(
        f"< peer={peer_host}:{peer_port} command=0x{decoded['command']:02x} "
        f"result={RESULTS.get(result, 'unknown')} "
        f"acoustic_enabled={decoded['enabled']} seq={decoded['seq']} "
        f"packet={decoded['raw'].hex(' ')}",
        flush=True,
    )


def check_response(command: str, seq: int, decoded: dict, expected_enabled) -> bool:
    ok = True
    if decoded["command"] != COMMANDS[command]:
        print(
            f"FAIL: response command mismatch, expected 0x{COMMANDS[command]:02x}",
            file=sys.stderr,
        )
        ok = False
    if decoded["seq"] != seq:
        print(f"FAIL: response seq mismatch, expected {seq}", file=sys.stderr)
        ok = False
    if decoded["result"] != 0:
        print(
            f"FAIL: node returned {RESULTS.get(decoded['result'], 'unknown')}",
            file=sys.stderr,
        )
        ok = False
    if expected_enabled is not None and decoded["enabled"] != expected_enabled:
        print(
            f"FAIL: acoustic_enabled={decoded['enabled']}, expected {expected_enabled}",
            file=sys.stderr,
        )
        ok = False
    return ok


def next_seq(base_seq, offset: int) -> int:
    if base_seq is None:
        return random.randint(0, 0xFFFF)
    return (base_seq + offset) & 0xFFFF


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Simulate upper-computer UDP control for hal_dvl_node acoustic mode."
    )
    parser.add_argument(
        "command",
        choices=["enable", "disable", "query", "cycle"],
        help="cycle runs: query, enable, query, disable, query",
    )
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=(
            "hal_dvl_node host. Default is DVL_UDP_HOST if set, "
            "otherwise 127.0.0.1."
        ),
    )
    parser.add_argument("--port", type=int, default=8114, help="hal_dvl_node UDP port")
    parser.add_argument(
        "--bind-ip",
        default="",
        help="local bind IP; empty means all local interfaces",
    )
    parser.add_argument(
        "--local-port",
        type=int,
        default=8113,
        help="local UDP port. Use 0 if 8113 is already occupied.",
    )
    parser.add_argument("--seq", type=int, default=None, help="base sequence number")
    parser.add_argument("--timeout", type=float, default=3.0, help="response timeout, seconds")
    parser.add_argument("--delay", type=float, default=0.5, help="delay between cycle commands")
    args = parser.parse_args()

    sequence = (
        [("query", None), ("enable", True), ("query", True), ("disable", False), ("query", False)]
        if args.command == "cycle"
        else [(args.command, None)]
    )

    all_ok = True
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(args.timeout)
        try:
            sock.bind((args.bind_ip, args.local_port))
        except OSError as exc:
            print(
                f"bind local UDP {args.bind_ip or '0.0.0.0'}:{args.local_port} failed: {exc}",
                file=sys.stderr,
            )
            print("Tip: retry with --local-port 0.", file=sys.stderr)
            return 10

        for idx, (command, expected_enabled) in enumerate(sequence):
            seq = next_seq(args.seq, idx)
            try:
                decoded = send_command(sock, args.host, args.port, command, seq)
                print_response(decoded)
            except socket.timeout:
                print("FAIL: timeout waiting for UDP response", file=sys.stderr)
                return 2
            except ValueError as exc:
                print(f"FAIL: invalid response: {exc}", file=sys.stderr)
                return 3

            all_ok = check_response(command, seq, decoded, expected_enabled) and all_ok
            if idx + 1 < len(sequence):
                time.sleep(args.delay)

    if args.command == "cycle":
        print("PASS: enable and disable UDP acoustic control both returned ok")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
