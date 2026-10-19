# SPDX-License-Identifier: GPL-3.0-only
"""Lean calibration: feet-off weight, resting balance, and how far each lean goes."""

from .samples import median, settled
from .session import Step

AXIS_MAX = 8192  # wii-stomp's stick range


class LeanCalibration:
	steps = [
		Step("empty", "Take your feet OFF the board.", 3),
		Step("rest", "Feet on the board, sitting/standing as you normally would. Keep still.", 5),
		Step("forward", "Lean FORWARD as far as you'd want for full-speed forward, and hold it.", 5),
		Step("back", "Lean BACK as far as you'd want for full-speed backward, and hold it.", 5),
		Step("left", "Lean LEFT as far as you'd want for a full turn left, and hold it.", 5),
		Step("right", "Lean RIGHT as far as you'd want for a full turn right, and hold it.", 5),
	]

	def __init__(self, recording):
		self.rec = recording
		rest = settled(recording["rest"], 1000)
		self.empty = median(s.total for s in recording["empty"])
		self.rest_total = median(s.total for s in rest)
		self.rest_x = median(s.raw_x for s in rest)
		self.rest_y = median(s.raw_y for s in rest)
		self.noise_x = max(abs(s.raw_x - self.rest_x) for s in rest)
		self.noise_y = max(abs(s.raw_y - self.rest_y) for s in rest)

	def held(self, key, attr):
		"""median reading while holding a lean, relative to resting"""
		rest = self.rest_x if attr == "raw_x" else self.rest_y
		return median(getattr(s, attr) for s in settled(self.rec[key], 1500)) - rest

	def apply(self, cfg):
		print(f"\n  empty board {self.empty}, feet on {self.rest_total}, "
			f"resting wobble x ±{self.noise_x} y ±{self.noise_y}")
		self.apply_min_weight(cfg)
		self.apply_forward_back(cfg)
		self.apply_left_right(cfg)

	def apply_min_weight(self, cfg):
		if self.rest_total - self.empty < 200:
			print("  ! Hardly any weight difference between feet off and on; keeping min_weight.")
		else:
			cfg["min_weight"] = round(self.empty + (self.rest_total - self.empty) * 0.3)

	def apply_forward_back(self, cfg):
		# thresholds are in wii-stomp's units: lean = (raw_y - centre) * y_mult
		ym = cfg["y_mult"]
		lean = {k: self.held(k, "raw_y") * ym for k in ("forward", "back")}
		print(f"  held lean: forward {lean['forward']}, back {lean['back']}")
		pos, neg = max(lean.values()), min(lean.values())
		if pos <= 0 or neg >= 0:
			print("  ! Forward and back leans didn't move in opposite directions; keeping those settings.")
			return
		if lean["forward"] > 0:
			print("  (y_mult is inverted, so forward lean sets the 'back' thresholds)")
		floor = self.noise_y * abs(ym) * 2  # never trigger from just sitting still
		for name, held in (("back", pos), ("fwd", -neg)):
			on, off = round(held * 0.6), round(held * 0.4)
			if on <= floor:
				print(f"  ! {name} lean ({held}) is barely more than resting wobble; lean further next time.")
				on, off = round(floor * 1.2), round(floor)
			cfg[name + "_on"], cfg[name + "_off"] = on, max(off, 1)

	def apply_left_right(self, cfg):
		# a held full lean should give full stick
		dl, dr = self.held("left", "raw_x"), self.held("right", "raw_x")
		print(f"  held lean: left {dl}, right {dr}")
		if dl >= 0 or dr <= 0:
			print("  ! Left and right leans didn't move the way expected; keeping those settings.")
			return
		cfg["x_mult"] = max(1, round(AXIS_MAX / ((dr - dl) / 2)))
		cfg["x_deadzone"] = max(100, round(self.noise_x * cfg["x_mult"] * 1.5))
