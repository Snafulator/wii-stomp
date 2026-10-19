# SPDX-License-Identifier: GPL-3.0-only
"""Running the wii-stomp program: finding it, its defaults, and raw readings."""

import json
import os
import shutil
import subprocess
import sys
import threading

from .samples import Sample

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find_program():
	"""wii-stomp next to this checkout, else on PATH"""
	local = os.path.join(ROOT, "wii-stomp")
	if os.path.exists(local):
		return local
	found = shutil.which("wii-stomp")
	if not found:
		sys.exit("Can't find wii-stomp; build it with `make` first.")
	return found


def is_running():
	return subprocess.run(["pgrep", "-x", "wii-stomp"], capture_output=True).returncode == 0


def default_settings():
	"""the program's built-in settings (wii-stomp -d)"""
	out = subprocess.run([find_program(), "-d"], capture_output=True, text=True, check=True)
	return json.loads(out.stdout)


class RawReader:
	"""runs `wii-stomp -r` in the background and keeps every Sample"""

	def __init__(self, device=None):
		cmd = [find_program(), "-r"] + ([device] if device else [])
		self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		self.samples = []
		self.lock = threading.Lock()
		threading.Thread(target=self._read, daemon=True).start()

	def _read(self):
		for line in self.proc.stdout:
			p = line.split()
			if len(p) >= 5:
				with self.lock:
					self.samples.append(Sample(*(int(v) for v in p[:5])))

	def count(self):
		with self.lock:
			return len(self.samples)

	def since(self, n):
		with self.lock:
			return self.samples[n:]

	def check(self):
		"""exits with the program's error if it has stopped (e.g. no board)"""
		if self.proc.poll() is not None:
			sys.exit(self.proc.stderr.read().strip() or "wii-stomp -r stopped")

	def close(self):
		self.proc.terminate()
