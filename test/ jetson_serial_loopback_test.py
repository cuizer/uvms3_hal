#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Jetson RS-232 串口回环测试程序

用途：
    在外部已经正确短接 TX/RX 的情况下，验证串口完整收发回环是否正常。

默认串口：
    /dev/ttyUART_232_A

默认参数：
    115200, 8N1, 无软/硬件流控

使用：
    sudo python3 jetson_serial_loopback_test.py

指定串口：
    sudo python3 jetson_serial_loopback_test.py --port /dev/ttyUART_232_C

连续测试 100 次：
    sudo python3 jetson_serial_loopback_test.py --count 100

依赖：
    python3-serial
"""

import argparse
import sys
import time
import serial


def read_exactly(ser, expected_len, timeout):
    """在 timeout 时间内尽量读取 expected_len 个字节。"""
    deadline = time.monotonic() + timeout
    data = bytearray()

    while len(data) < expected_len and time.monotonic() < deadline:
        waiting = ser.in_waiting

        if waiting > 0:
            chunk = ser.read(min(waiting, expected_len - len(data)))
            data.extend(chunk)
        else:
            time.sleep(0.002)

    return bytes(data)


def build_frame(seq):
    """
    每次发送不同帧，避免把旧缓存误认为当前回环数据。
    帧格式：
        55 AA + 4字节序号 + 11 22 33 44 A5 5A + 0D 0A
    """
    return (
        b"\x55\xAA"
        + seq.to_bytes(4, byteorder="big", signed=False)
        + b"\x11\x22\x33\x44\xA5\x5A\x0D\x0A"
    )


def main():
    parser = argparse.ArgumentParser(
        description="Jetson RS-232/UART TX-RX shorted loopback test"
    )

    parser.add_argument(
        "--port",
        default="/dev/ttyUART_232_A",
        help="串口设备，默认 /dev/ttyUART_232_A",
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="波特率，默认 115200",
    )

    parser.add_argument(
        "--count",
        type=int,
        default=20,
        help="测试次数，默认 20 次；设置 0 表示无限循环",
    )

    parser.add_argument(
        "--timeout",
        type=float,
        default=1.0,
        help="每帧等待回传超时，默认 1.0 秒",
    )

    parser.add_argument(
        "--interval",
        type=float,
        default=0.2,
        help="每次测试间隔，默认 0.2 秒",
    )

    args = parser.parse_args()

    print("=" * 70)
    print("Jetson 串口 TX/RX 回环测试")
    print(f"PORT      : {args.port}")
    print(f"BAUD      : {args.baud}")
    print("FORMAT    : 8N1")
    print("FLOW CTRL : none")
    print(f"COUNT     : {'infinite' if args.count == 0 else args.count}")
    print(f"TIMEOUT   : {args.timeout} s")
    print("=" * 70)

    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0,
            write_timeout=2.0,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
        )
    except serial.SerialException as e:
        print(f"\n[ERROR] 无法打开串口：{e}")
        print("请检查：")
        print("  1. 串口设备是否存在")
        print("  2. 是否被其他进程占用")
        print("  3. 当前用户是否有串口权限")
        sys.exit(1)

    passed = 0
    failed = 0
    seq = 0

    try:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        print("\n开始测试。请确认 TX/RX 已在正确位置短接。")
        print("按 Ctrl+C 可随时停止。\n")

        while args.count == 0 or seq < args.count:
            frame = build_frame(seq)

            ser.reset_input_buffer()

            try:
                written = ser.write(frame)
                ser.flush()
            except serial.SerialTimeoutException:
                failed += 1
                print(
                    f"[{seq:06d}] FAIL 发送超时 | "
                    f"TX={frame.hex(' ')}"
                )
                seq += 1
                time.sleep(args.interval)
                continue

            time.sleep(0.01)

            rx = read_exactly(ser, len(frame), args.timeout)

            if rx == frame:
                passed += 1
                print(
                    f"[{seq:06d}] PASS "
                    f"| TX={written:2d} bytes "
                    f"| RX={len(rx):2d} bytes "
                    f"| {rx.hex(' ')}"
                )

            elif len(rx) == 0:
                failed += 1
                print(
                    f"[{seq:06d}] FAIL NO DATA "
                    f"| TX={written:2d} bytes "
                    f"| RX=0 bytes"
                )

            else:
                failed += 1
                print(
                    f"[{seq:06d}] FAIL MISMATCH\n"
                    f"          TX: {frame.hex(' ')}\n"
                    f"          RX: {rx.hex(' ')}"
                )

            seq += 1
            time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\n\n用户终止测试。")

    finally:
        ser.close()

    total = passed + failed

    print("\n" + "=" * 70)
    print("测试结果")
    print(f"TOTAL : {total}")
    print(f"PASS  : {passed}")
    print(f"FAIL  : {failed}")

    if total > 0:
        print(f"PASS RATE : {passed / total * 100:.1f}%")

    if total > 0 and failed == 0:
        print("\nRESULT: LOOPBACK PASS")
        print("说明当前测试链路的 TX 和 RX 可以完成正常回环。")

    elif passed == 0 and failed > 0:
        print("\nRESULT: LOOPBACK FAIL")
        print("所有测试均未成功。若均为 RX=0，请重点检查：")
        print("  1. TX/RX 是否确实短接在正确的输入/输出端")
        print("  2. RS-232 转 TTL 模块是否正确供电")
        print("  3. RS-232 线序是否正确")
        print("  4. Jetson TX 是否真正从物理接口输出")
        print("  5. Jetson RX / RS-232 接收通道是否正常")
        print("  6. ModemManager 或其他程序是否占用串口")

    else:
        print("\nRESULT: UNSTABLE LOOPBACK")
        print("部分成功、部分失败，说明链路可能存在接触不良、干扰或丢字节。")

    print("=" * 70)


if __name__ == "__main__":
    main()