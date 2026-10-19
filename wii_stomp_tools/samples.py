# SPDX-License-Identifier: GPL-3.0-only
"""Board readings and small statistics helpers."""

from collections import namedtuple


class Sample(namedtuple("Sample", "ms fl fr bl br")):
	"""one board report: time in ms and the four sensors"""

	@property
	def total(self): return self.fl + self.fr + self.bl + self.br

	@property
	def left(self): return self.fl + self.bl

	@property
	def right(self): return self.fr + self.br

	@property
	def raw_x(self): return self.right - self.left  # same as wii-stomp's raw_x

	@property
	def raw_y(self): return (self.bl + self.br) - (self.fl + self.fr)  # back - front


def median(values):
	values = sorted(values)
	return values[len(values) // 2] if values else 0


def settled(samples, skip_ms):
	"""drops the first skip_ms, while you're still moving into position"""
	t0 = samples[0].ms
	return [s for s in samples if s.ms - t0 >= skip_ms]
