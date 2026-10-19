#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Walks you through some actions on the balance board and works out
settings for lean movement and bash detection.

Usage: ./calibrate.py [--only lean|bash] [--device /dev/input/eventN] [-o settings.json]

Writes ~/.config/wii-stomp/settings.json by default. Every recording is saved
to calibration-recording.json, and `--replay calibration-recording.json`
re-runs the maths on it without the board.
"""

import argparse
import os

from wii_stomp_tools import driver
from wii_stomp_tools.bash import BashCalibration
from wii_stomp_tools.lean import LeanCalibration
from wii_stomp_tools.session import Session, load_recording, save_recording
from wii_stomp_tools.settings import Settings


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("--only", choices=("lean", "bash"))
	ap.add_argument("--device", help="the board's /dev/input/eventN (found automatically)")
	ap.add_argument("-o", "--output", help="settings file to write (default ~/.config/wii-stomp/settings.json)")
	ap.add_argument("--replay", help="analyse a saved recording instead of recording")
	args = ap.parse_args()

	cfg = Settings(args.output)

	if args.replay:
		rec = load_recording(args.replay)
	else:
		steps = []
		if args.only != "bash":
			steps += LeanCalibration.steps
		if args.only != "lean":
			steps += BashCalibration.steps
		if driver.is_running():
			print("wii-stomp is running and will press keys/buttons while you calibrate.")
			if input("Continue anyway? [y/N] ").strip().lower() != "y":
				return
		print("Calibrate in the position you'll play in, seated or standing.")
		rec = Session(args.device).run(steps)
		path = os.path.join(os.getcwd(), "calibration-recording.json")
		save_recording(rec, path)
		print(f"\n(recording saved to {path})")

	if "rest" in rec:
		LeanCalibration(rec).apply(cfg)
	if "moving" in rec:
		BashCalibration(rec, cfg).apply(cfg)

	changes = cfg.changes()
	print()
	if not changes:
		print("Nothing to change.")
		return
	print("Proposed changes:")
	for k, (old, new) in changes.items():
		print(f"  {k}: {old} -> {new}")
	if input(f"Write them to {cfg.path}? [Y/n] ").strip().lower() in ("", "y", "yes"):
		cfg.save()
		print("Saved. Restart wii-stomp to use them.")


if __name__ == "__main__":
	main()
