#!/usr/bin/env python3
import argparse
import sys
import time

import rclpy
from lifecycle_msgs.msg import Transition
from lifecycle_msgs.srv import ChangeState
from rclpy.node import Node

from hal.msg import HalDvl
from hal.msg import HalDvlControl


MODE_ACOUSTIC_ENABLED = 0
MODE_ACOUSTIC_DISABLED = 1


class DvlControlTopicTester(Node):
    def __init__(self, args):
        super().__init__("test_dvl_control_topic")
        self.args = args
        self.last_dvl = None
        self.last_dvl_time = None

        self.control_pub = self.create_publisher(HalDvlControl, args.control_topic, 10)
        self.dvl_sub = self.create_subscription(HalDvl, args.dvl_topic, self._dvl_callback, 10)

    def _dvl_callback(self, msg):
        self.last_dvl = msg
        self.last_dvl_time = time.monotonic()
        mode_text = self._mode_text(msg.modecontrol_cmd)
        print(
            f"[recv] {self.args.dvl_topic}: modecontrol_cmd={msg.modecontrol_cmd}({mode_text}), "
            f"connection_status={msg.connection_status}, "
            f"vel=({msg.velocity_x:.4f}, {msg.velocity_y:.4f}, {msg.velocity_z:.4f})"
        )

    def publish_command(self, command):
        msg = HalDvlControl()
        msg.command = command
        self.control_pub.publish(msg)
        print(f"[send] {self.args.control_topic}: command={command}({self._command_text(command)})")

    def wait_for_mode(self, expected_mode, timeout):
        deadline = time.monotonic() + timeout
        while rclpy.ok() and time.monotonic() < deadline:
            rclpy.spin_once(self, timeout_sec=0.1)
            if self.last_dvl is not None and self.last_dvl.modecontrol_cmd == expected_mode:
                return True
        return False

    def wait_for_any_dvl(self, timeout):
        deadline = time.monotonic() + timeout
        while rclpy.ok() and time.monotonic() < deadline:
            rclpy.spin_once(self, timeout_sec=0.1)
            if self.last_dvl is not None:
                return True
        return False

    def activate_lifecycle_node(self, node_name):
        for transition_id, transition_name in (
            (Transition.TRANSITION_CONFIGURE, "configure"),
            (Transition.TRANSITION_ACTIVATE, "activate"),
        ):
            client = self.create_client(ChangeState, f"{node_name}/change_state")
            if not client.wait_for_service(timeout_sec=self.args.lifecycle_timeout):
                print(f"[warn] lifecycle service not available: {node_name}/change_state")
                return False
            req = ChangeState.Request()
            req.transition.id = transition_id
            future = client.call_async(req)
            rclpy.spin_until_future_complete(self, future, timeout_sec=self.args.lifecycle_timeout)
            if future.result() is None:
                print(f"[warn] lifecycle {transition_name} timed out for {node_name}")
                return False
            if not future.result().success:
                print(f"[warn] lifecycle {transition_name} rejected for {node_name}")
                return False
            print(f"[ok] lifecycle {transition_name}: {node_name}")
        return True

    def run_command(self, command_name):
        if self.args.activate:
            self.activate_lifecycle_node(self.args.lifecycle_node)

        time.sleep(self.args.discovery_delay)
        if self.args.wait_initial:
            self.wait_for_any_dvl(self.args.wait_timeout)

        if command_name == "cycle":
            return self.run_cycle()

        command = self._command_value(command_name)
        self.publish_command(command)
        if command == HalDvlControl.CMD_ENABLE:
            return 0 if self._check_expected_mode(MODE_ACOUSTIC_ENABLED, "enable") else 1
        if command == HalDvlControl.CMD_DISABLE:
            return 0 if self._check_expected_mode(MODE_ACOUSTIC_DISABLED, "disable") else 1

        self.wait_for_any_dvl(self.args.wait_timeout)
        print("[info] query command has no response topic; check hal_dvl_node log for query output.")
        print("[result] PASS")
        return 0

    def run_cycle(self):
        all_ok = True

        print("\n[test] query before enable")
        self.publish_command(HalDvlControl.CMD_QUERY)
        self.wait_for_any_dvl(self.args.wait_timeout)

        print("\n[test] enable acoustic")
        self.publish_command(HalDvlControl.CMD_ENABLE)
        if not self._check_expected_mode(MODE_ACOUSTIC_ENABLED, "enable", print_result=False):
            all_ok = False

        print("\n[test] query after enable")
        self.publish_command(HalDvlControl.CMD_QUERY)
        self.wait_for_any_dvl(self.args.wait_timeout)

        print("\n[test] disable acoustic")
        self.publish_command(HalDvlControl.CMD_DISABLE)
        if not self._check_expected_mode(MODE_ACOUSTIC_DISABLED, "disable", print_result=False):
            all_ok = False

        print("\n[test] query after disable")
        self.publish_command(HalDvlControl.CMD_QUERY)
        self.wait_for_any_dvl(self.args.wait_timeout)

        print("\n[result] " + ("PASS" if all_ok else "FAIL"))
        return 0 if all_ok else 1

    def _check_expected_mode(self, expected_mode, action_name, print_result=True):
        ok = self.wait_for_mode(expected_mode, self.args.wait_timeout)
        if not ok:
            print(
                f"[fail] {action_name} did not produce modecontrol_cmd={expected_mode} "
                f"within {self.args.wait_timeout:.1f}s"
            )
            print("[hint] If this is hardware test, confirm DVL serial is connected and returns wra ACK.")
        elif print_result:
            print("[result] PASS")
        return ok

    @staticmethod
    def _command_value(name):
        return {
            "disable": HalDvlControl.CMD_DISABLE,
            "enable": HalDvlControl.CMD_ENABLE,
            "query": HalDvlControl.CMD_QUERY,
        }[name]

    @staticmethod
    def _command_text(command):
        return {
            HalDvlControl.CMD_DISABLE: "disable",
            HalDvlControl.CMD_ENABLE: "enable",
            HalDvlControl.CMD_QUERY: "query",
        }.get(command, "unknown")

    @staticmethod
    def _mode_text(mode):
        return {
            MODE_ACOUSTIC_ENABLED: "acoustic_enabled",
            MODE_ACOUSTIC_DISABLED: "acoustic_disabled",
        }.get(mode, "unknown")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Publish /hal/dvlcontrol and watch /hal/dvl modecontrol_cmd."
    )
    parser.add_argument("command", choices=["enable", "disable", "query", "cycle"], default="cycle")
    parser.add_argument("--control-topic", default="/hal/dvlcontrol")
    parser.add_argument("--dvl-topic", default="/hal/dvl")
    parser.add_argument("--wait-timeout", type=float, default=4.0)
    parser.add_argument("--discovery-delay", type=float, default=0.5)
    parser.add_argument("--wait-initial", action="store_true", help="wait for first /hal/dvl before sending")
    parser.add_argument("--activate", action="store_true", help="try lifecycle configure+activate first")
    parser.add_argument("--lifecycle-node", default="/hal_dvl_node")
    parser.add_argument("--lifecycle-timeout", type=float, default=3.0)
    return parser.parse_args()


def main():
    args = parse_args()
    rclpy.init()
    node = DvlControlTopicTester(args)
    try:
        return node.run_command(args.command)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    sys.exit(main())
