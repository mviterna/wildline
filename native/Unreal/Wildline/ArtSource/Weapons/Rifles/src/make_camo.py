"""Generate a Kryptek-Highlander-style camo texture (tileable noise + hex scale overlay)."""
import sys
import numpy as np
from PIL import Image

W = 2048
rng = np.random.default_rng(7)


def periodic_noise(sigma):
	"""Gaussian-filtered white noise via FFT; result tiles seamlessly."""
	white = rng.standard_normal((W, W))
	fx = np.fft.fftfreq(W)
	k2 = fx[None, :] ** 2 + fx[:, None] ** 2
	filt = np.exp(-k2 * (2 * np.pi * sigma) ** 2 / 2)
	n = np.real(np.fft.ifft2(np.fft.fft2(white) * filt))
	return (n - n.mean()) / n.std()


def hexgrid(size):
	"""Return (edge distance in px, per-cell hash) for a pointy-top hex grid."""
	y, x = np.mgrid[0:W, 0:W].astype(np.float64)
	q = (np.sqrt(3) / 3 * x - y / 3) / size
	r = (2 / 3 * y) / size
	cx, cz = q, r
	cy = -cx - cz
	rx, ry, rz = np.round(cx), np.round(cy), np.round(cz)
	dx, dy, dz = np.abs(rx - cx), np.abs(ry - cy), np.abs(rz - cz)
	m1 = (dx > dy) & (dx > dz)
	m2 = ~m1 & (dy > dz)
	rx = np.where(m1, -ry - rz, rx)
	ry = np.where(m2, -rx - rz, ry)
	rz = np.where(~m1 & ~m2, -rx - ry, rz)
	centerx = size * np.sqrt(3) * (rx + rz / 2)
	centery = size * 1.5 * rz
	vx, vy = x - centerx, y - centery
	inr = size * np.sqrt(3) / 2
	d = np.zeros_like(x)
	for a in (0, 60, 120):
		t = np.radians(a)
		d = np.maximum(d, np.abs(vx * np.cos(t) + vy * np.sin(t)))
	h = np.sin(rx * 12.9898 + rz * 78.233) * 43758.5453
	h = h - np.floor(h)
	return inr - d, h


def hexc(s):
	return np.array([int(s[i:i + 2], 16) for i in (1, 3, 5)], dtype=np.float64) / 255


light = hexc('#cdbf9f')
tan = hexc('#a8957002'[:7])
khaki = hexc('#8a7a5a')
olive = hexc('#5f5d3c')
brown = hexc('#5a4130')
dark = hexc('#2d231b')

macro = periodic_noise(70)
mid = periodic_noise(28)
fine = periodic_noise(6)
grain = periodic_noise(1.2)

# Base mottle
img = np.empty((W, W, 3))
img[:] = tan
img[macro + 0.35 * mid > 0.55] = light
img[(macro + 0.4 * mid < -0.35)] = khaki
img[(mid + 0.3 * fine > 0.9)] = olive
img[(macro + 0.5 * mid < -0.95)] = brown

# Hex scale overlay in bands
edge, cellh = hexgrid(20)
band = periodic_noise(45) + 0.35 * periodic_noise(14)
scale_zone = band > 0.15
dark_zone = band > 0.75
outline = edge < 2.6

fillsel = scale_zone & (cellh > 0.55)
img[fillsel] = img[fillsel] * 0.7 + brown * 0.3
img[dark_zone & (cellh > 0.35)] = dark * 0.6 + brown * 0.4
img[dark_zone & (cellh > 0.8)] = olive * 0.8
img[scale_zone & outline] = dark
# faint outlines elsewhere
faint = ~scale_zone & outline & (band > -0.4)
img[faint] = img[faint] * 0.75 + dark * 0.25

img *= (1 + 0.06 * grain[..., None] + 0.03 * fine[..., None])
img = np.clip(img, 0, 1)
Image.fromarray((img * 255).astype(np.uint8)).save(sys.argv[1])
print('wrote', sys.argv[1])
