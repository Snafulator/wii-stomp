# SPDX-License-Identifier: GPL-3.0-only
"""Bash detection: the same maths as wii-stomp's bash.c, and calibrating its thresholds."""

from .session import Step

BASHES = 5


class BashDetector:
	"""replays recordings through wii-stomp's bash detection"""

	def __init__(self, cfg):
		self.cfg = cfg

	def rises(self, samples):
		"""per sample: (ms, left rise, right rise) above each side's low in the window"""
		window, out = self.cfg["bash_window_ms"], []
		for i, s in enumerate(samples):
			low_l, low_r, j = s.left, s.right, i
			while j >= 0 and s.ms - samples[j].ms <= window:
				low_l = min(low_l, samples[j].left)
				low_r = min(low_r, samples[j].right)
				j -= 1
			out.append((s.ms, s.left - low_l, s.right - low_r))
		return out

	def fires(self, rise, other):
		c = self.cfg
		return rise >= c["bash_rise"] and rise - other >= c["bash_margin"] and other >= c["bash_jolt"]

	def simulate(self, rises):
		"""how many bashes wii-stomp would fire on each side, including its cooldown"""
		fired, last = {"left": 0, "right": 0}, {"left": -1e9, "right": -1e9}
		for t, rl, rr in rises:
			for side, rise, other in (("left", rl, rr), ("right", rr, rl)):
				if t - last[side] >= self.cfg["bash_cooldown_ms"] and self.fires(rise, other):
					fired[side] += 1
					last[side] = t
		return fired


def bash_events(rises, side):
	"""groups the samples of each bash on one side: [[(rise, other), ...], ...]"""
	groups, last_t = [], None
	for t, rl, rr in rises:
		rise, other = (rl, rr) if side == "left" else (rr, rl)
		if rise >= 150 and rise > other:
			if last_t is None or t - last_t > 250:
				groups.append([])
			groups[-1].append((rise, other))
			last_t = t
	if not groups:
		return []
	# drop small bumps (weight shifts between bashes)
	top = max(max(r for r, _ in g) for g in groups)
	return [g for g in groups if max(r for r, _ in g) >= top * 0.3]


def ratio(rise, other, R, M, J):
	""">= 1 means the detector would fire on this sample with thresholds R, M, J"""
	r = min(rise / R, (rise - other) / M)
	return min(r, other / J) if J > 0 else r


class BashCalibration:
	steps = [
		Step("bash_left", f"Bash your LEFT foot down {BASHES} times, about a second apart, as you would in game.", 9),
		Step("bash_right", f"Bash your RIGHT foot down {BASHES} times, about a second apart, as you would in game.", 9),
		Step("moving", "Now move around like you do while playing: lean, shift your weight, fidget.\n"
			"  Do NOT bash.", 15),
	]

	def __init__(self, recording, cfg):
		self.detector = BashDetector(cfg)
		self.rises = {k: self.detector.rises(recording[k]) for k in ("bash_left", "bash_right", "moving")}
		self.left = bash_events(self.rises["bash_left"], "left")
		self.right = bash_events(self.rises["bash_right"], "right")

	def noise(self):
		"""(rise, other) samples that must NOT fire: all of 'moving' on both sides,
		and the opposite side while bashing"""
		r = self.rises
		return ([(rl, rr) for _, rl, rr in r["moving"]] + [(rr, rl) for _, rl, rr in r["moving"]]
			+ [(rr, rl) for _, rl, rr in r["bash_left"]] + [(rl, rr) for _, rl, rr in r["bash_right"]])

	def apply(self, cfg):
		print(f"\n  found {len(self.left)} left and {len(self.right)} right bashes (expected {BASHES} each)")
		if not self.left or not self.right:
			print("  ! Need bashes on both sides; keeping bash settings.")
			return
		for name, events in (("left", self.left), ("right", self.right)):
			print(f"  {name}: rises " + ", ".join(f"{r} (other {o})" for r, o in (max(g) for g in events)))

		sep = self.fit(cfg)
		if sep < 1:
			print("  ! Some of your movement looks exactly like a weak bash; you'll get some misses or false bashes.")
			print("    Bashing harder, or moving more gently, will help.")
		for key, label in (("bash_left", "left bashes"), ("bash_right", "right bashes"), ("moving", "moving")):
			f = self.detector.simulate(self.rises[key])
			print(f"  with these settings, {label}: fires left {f['left']}, right {f['right']}")

	def fit(self, cfg):
		"""Tries different shapes (margin and jolt relative to rise), scales each so the
		weakest bash and the strongest non-bash sit equally far from the threshold,
		and keeps the shape that separates them best. Returns that separation (<1 = overlap)."""
		events, noise = self.left + self.right, self.noise()
		weakest = min(max(r for r, _ in g) for g in events)
		best = None
		for m10 in range(1, 16):
			for j100 in [0] + list(range(2, 52, 4)):
				R, M, J = weakest, weakest * m10 / 10, weakest * j100 / 100
				bash_lo = min(max(ratio(r, o, R, M, J) for r, o in g) for g in events)
				noise_hi = max(ratio(r, o, R, M, J) for r, o in noise)
				if bash_lo <= 0:
					continue
				sep = bash_lo / noise_hi if noise_hi > 0 else 1e9
				if best is None or sep > best[0]:
					best = (sep, bash_lo, R, M, J)
		sep, bash_lo, R, M, J = best
		# headroom on the weakest bash: balanced against the moving data,
		# but never closer than 1/1.4 of it even if moving never came close
		k = bash_lo / min(sep ** 0.5, 1.4)
		cfg["bash_rise"], cfg["bash_margin"], cfg["bash_jolt"] = round(R * k), round(M * k), round(J * k)
		return sep
