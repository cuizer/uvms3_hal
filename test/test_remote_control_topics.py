#!/usr/bin/env python3
import argparse
import math
import sys
import time

import rclpy
from lifecycle_msgs.msg import Transition
from lifecycle_msgs.srv import ChangeState
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray

from hal.msg import HalModeControl
from hal.msg import HalRemoteControl


CHANNEL_MIN = 353.0
CHANNEL_MID = 1024.0
CHANNEL_MAX = 1695.0


class RemoteControlTopicTester(Node):
    def __init__(self, args):
        super().__init__("test_remote_control_topics")
        self.args = args
        self.last_thruster = None
        self.last_thruster_time = None

        self.mode_pub = self.create_publisher(HalModeControl, args.mode_topic, 10)
        self.remote_pub = self.create_publisher(HalRemoteControl, args.remote_topic, 10)
        self.thruster_sub = self.create_subscription(
            Float64MultiArray,
            args.thruster_topic,
            self._thruster_callback,
            10,
        )

    def _thruster_callback(self, msg):
        self.last_thruster = list(msg.data)
        self.last_thruster_time = time.monotonic()
        print(f"[recv] {self.args.thruster_topic}: {self._format_values(self.last_thruster)}")

    def publish_mode(self, command):
        msg = HalModeControl()
        msg.command = command
        self.mode_pub.publish(msg)
        print(f"[send] {self.args.mode_topic}: command={command}")

    def publish_remote(self, surge, sway, heave, yaw, pitch=CHANNEL_MID, roll=CHANNEL_MID):
        msg = HalRemoteControl()
        msg.surge = float(surge)
        msg.sway = float(sway)
        msg.heave = float(heave)
        msg.yaw = float(yaw)
        msg.pitch = float(pitch)
        msg.roll = float(roll)
        self.remote_pub.publish(msg)
        print(
            f"[send] {self.args.remote_topic}: "
            f"surge={msg.surge:.1f}, sway={msg.sway:.1f}, heave={msg.heave:.1f}, "
            f"yaw={msg.yaw:.1f}, pitch={msg.pitch:.1f}, roll={msg.roll:.1f}"
        )

    def wait_for_thruster(self, timeout, require_nonzero=None):
        deadline = time.monotonic() + timeout
        start_time = self.last_thruster_time
        while rclpy.ok() and time.monotonic() < deadline:
            rclpy.spin_once(self, timeout_sec=0.05)
            if self.last_thruster is None:
                continue
            if self.last_thruster_time == start_time:
                continue
            if require_nonzero is None:
                return True
            nonzero = any(abs(value) > self.args.zero_epsilon for value in self.last_thruster)
            if nonzero == require_nonzero:
                return True
        return False

    def publish_for_duration(self, channels, duration, rate_hz):
        period = 1.0 / rate_hz
        deadline = time.monotonic() + duration
        while rclpy.ok() and time.monotonic() < deadline:
            self.publish_remote(*channels)
            rclpy.spin_once(self, timeout_sec=0.01)
            time.sleep(period)

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

    def run_cycle(self):
        if self.args.activate:
            self.activate_lifecycle_node(self.args.lifecycle_node)

        time.sleep(self.args.discovery_delay)

        self.publish_mode(HalModeControl.CMD_ENABLE_KEYBOARD)
        self.publish_for_duration(
            (CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID),
            self.args.center_duration,
            self.args.rate,
        )

        test_vectors = [
            ("surge+", (CHANNEL_MAX, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID)),
            ("sway+", (CHANNEL_MID, CHANNEL_MAX, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID)),
            ("heave+", (CHANNEL_MID, CHANNEL_MID, CHANNEL_MAX, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID)),
            ("yaw+", (CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MAX, CHANNEL_MID, CHANNEL_MID)),
        ]

        all_ok = True
        for name, channels in test_vectors:
            print(f"\n[test] {name}")
            self.publish_for_duration(channels, self.args.command_duration, self.args.rate)
            if not self.wait_for_thruster(self.args.wait_timeout, require_nonzero=True):
                print(f"[fail] no non-zero thruster command observed for {name}")
                all_ok = False

        print("\n[test] center")
        self.publish_for_duration(
            (CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID, CHANNEL_MID),
            self.args.center_duration,
            self.args.rate,
        )

        print("\n[test] disable keyboard")
        self.publish_mode(HalModeControl.CMD_DISABLE_KEYBOARD)
        if not self.wait_for_thruster(self.args.wait_timeout, require_nonzero=False):
            print("[fail] no zero thruster command observed after disable")
            all_ok = False

        print("\n[result] " + ("PASS" if all_ok else "FAIL"))
        return 0 if all_ok else 1

    @staticmethod
    def _format_values(values):
        return "[" + ", ".join(f"{value:.3f}" for value in values) + "]"


def parse_args():
    parser = argparse.ArgumentParser(
        description="Publish /hal/modecontrol and /hal/remotecontrol to test BSP remote control."
    )
    parser.add_argument("--mode-topic", default="/hal/modecontrol")
    parser.add_argument("--remote-topic", default="/hal/remotecontrol")
    parser.add_argument("--thruster-topic", default="/hal/thruster/cmd")
    parser.add_argument("--rate", type=float, default=10.0, help="remote command publish rate")
    parser.add_argument("--command-duration", type=float, default=1.0)
    parser.add_argument("--center-duration", type=float, default=0.5)
    parser.add_argument("--wait-timeout", type=float, default=2.0)
    parser.add_argument("--zero-epsilon", type=float, default=1e-3)
    parser.add_argument("--discovery-delay", type=float, default=0.5)
    parser.add_argument("--activate", action="store_true", help="try lifecycle configure+activate first")
    parser.add_argument("--lifecycle-node", default="/bsp_remotecontrol_node")
    parser.add_argument("--lifecycle-timeout", type=float, default=3.0)
    return parser.parse_args()


def main():
    args = parse_args()
    if not math.isfinite(args.rate) or args.rate <= 0.0:
        print("rate must be positive", file=sys.stderr)
        return 2

    rclpy.init()
    node = RemoteControlTopicTester(args)
    try:
        return node.run_cycle()
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    sys.exit(main())
