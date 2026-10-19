# SPDX-License-Identifier: GPL-3.0-only
"""Guiding someone through timed actions on the board and recording them."""

import json
import time

from .driver import RawReader
from .samples import Sample


class Step:
	def __init__(self, key, text, seconds):
		self.key, self.text, self.seconds = key, text, seconds


class Session:
	"""prompts for each Step, counts down, and records the board while they do it"""

	def __init__(self, device=None):
		self.reader = RawReader(device)
		self.recording = {}

	def run(self, steps):
		try:
			for step in steps:
				self.recording[step.key] = self._step(step)
		finally:
			self.reader.close()
		return self.recording

	def _step(self, step):
		print()
		print(step.text)
		input("  Press Enter when ready... ")
		for n in (3, 2, 1):
			print(f"\r  starting in {n}", end="", flush=True)
			time.sleep(1)
		print("\r  GO!            ")
		n = self.reader.count()
		end = time.monotonic() + step.seconds
		while time.monotonic() < end:
			self.reader.check()
			print(f"\r  recording... {end - time.monotonic():4.1f}s ", end="", flush=True)
			time.sleep(0.1)
		print("\r  done.               ")
		got = self.reader.since(n)
		if not got:
			raise SystemExit("No data from the board. Is it connected?")
		return got


def save_recording(recording, path):
	with open(path, "w") as f:
		json.dump(recording, f)


def load_recording(path):
	with open(path) as f:
		return {k: [Sample(*s[:5]) for s in v] for k, v in json.load(f).items()}
