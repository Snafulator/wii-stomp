# SPDX-License-Identifier: GPL-3.0-only
"""settings.json: wii-stomp's tuning values."""

import json
import os

from .driver import default_settings


def user_settings_path():
	base = os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config")
	return os.path.join(base, "wii-stomp", "settings.json")


class Settings(dict):
	"""the built-in defaults with a settings file on top; remembers what the
	file had so changes() can show old -> new"""

	def __init__(self, path=None):
		self.path = path or user_settings_path()
		self.defaults = default_settings()
		self.saved = {}
		if os.path.exists(self.path):
			with open(self.path) as f:
				self.saved = json.load(f)
		super().__init__(self.defaults)
		self.update(self.saved)

	def changes(self):
		"""{name: (old, new)} for every value that differs from what's in effect now"""
		return {k: (self.saved.get(k, self.defaults[k]), v) for k, v in self.items()
			if self.saved.get(k, self.defaults[k]) != v}

	def save(self):
		os.makedirs(os.path.dirname(self.path) or ".", exist_ok=True)
		with open(self.path, "w") as f:
			json.dump(dict(self), f, indent="\t")
			f.write("\n")
		self.saved = dict(self)
