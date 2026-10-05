"""Procedurally build photoreal single-shot break-action shotguns in Blender and export UE-ready FBX.

Run: blender -b -P build_shotgun.py -- <out_dir> [pardner] [stevens] [--quick]
With no names every gun is built. --quick skips baking and renders the procedural materials at low
settings into Previews/_quick (look-dev only; writes no FBX, textures or .blend).

Reuses the helper section of build_rifles.py (read only, none of its builds run) with the same
conventions: meters in Blender (exported as cm), +X = muzzle, +Z = up, Blender -Y = the gun's right
side (becomes UE +Y), origin at the trigger.

Surfaces come from 3D procedural Cycles materials (growth-ring wood, case colors, bluing with polish
streaks, edge wear and grime from Bevel/AO curvature, checkered spurs) baked into two texture sets per
gun, Wood and Metal, written to Textures/:
  T_<Gun>_<Set>_D    base color, sRGB (also embedded in the FBX)
  T_<Gun>_<Set>_N    tangent-space normal, DirectX / UE convention (green down)
  T_<Gun>_<Set>_ORM  R = ambient occlusion, G = roughness, B = metallic, linear
"""
import os

_here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(_here, 'build_rifles.py'), encoding='utf-8') as f:
	_src = f.read()
exec(_src[:_src.index('# ---------------------------------------------------------------- Rascal')])

import numpy as np

ARGS = sys.argv[sys.argv.index('--') + 2:]
QUICK = '--quick' in ARGS
ONLY = {a.lower() for a in ARGS if not a.startswith('--')}
TEX_RES = 4096
SETS = ('Wood', 'Metal')


def srgb(hexstr):
	"""Hex sRGB color -> linear RGB."""
	c = [int(hexstr[i:i + 2], 16) / 255 for i in (1, 3, 5)]
	return tuple(v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in c)


# ------------------------------------------------------------- shader graphs

class Graph:
	"""Minimal shader-graph builder; helpers return output sockets. Object coordinates are meters."""

	def __init__(self, mat):
		self.nt = mat.node_tree
		self.nt.nodes.clear()
		self.co = self.node('ShaderNodeTexCoord').outputs['Object']
		self.normal = self.node('ShaderNodeNewGeometry').outputs['Normal']

	def node(self, kind, inputs=None, **props):
		n = self.nt.nodes.new(kind)
		for k, v in props.items():
			setattr(n, k, v)
		for k, v in (inputs or {}).items():
			self.put(n.inputs[k], v)
		return n

	def put(self, sock, v):
		if isinstance(v, bpy.types.NodeSocket):
			self.nt.links.new(v, sock)
		elif sock.type == 'RGBA' and len(v) == 3:
			sock.default_value = (*v, 1.0)
		else:
			sock.default_value = v

	def socket(self, v):
		"""Socket for a value: sockets pass through, tuples become RGB nodes, numbers Value nodes."""
		if isinstance(v, bpy.types.NodeSocket):
			return v
		n = self.node('ShaderNodeRGB' if isinstance(v, tuple) else 'ShaderNodeValue')
		n.outputs[0].default_value = (*v, 1.0) if isinstance(v, tuple) else v
		return n.outputs[0]

	def math(self, op, a, b=0.0, c=None, clamp=False):
		n = self.node('ShaderNodeMath', operation=op, use_clamp=clamp)
		self.put(n.inputs[0], a)
		self.put(n.inputs[1], b)
		if c is not None:
			self.put(n.inputs[2], c)
		return n.outputs[0]

	def lin(self, const, *terms, clamp=False):
		"""const + sum(socket * weight)."""
		acc = const
		for s, w in terms:
			acc = self.math('MULTIPLY_ADD', s, w, acc)
		return self.math('ADD', acc, 0.0, clamp=True) if clamp else acc

	def vec(self, op, a, b=None):
		n = self.node('ShaderNodeVectorMath', operation=op)
		self.put(n.inputs[0], a)
		if b is not None:
			self.put(n.inputs[1], b)
		return n.outputs['Value'] if op in ('DOT_PRODUCT', 'LENGTH', 'DISTANCE') else n.outputs['Vector']

	def axis(self, v, i):
		return self.node('ShaderNodeSeparateXYZ', {0: v}).outputs[i]

	def remap(self, v, lo, hi, smooth=True):
		"""0..1 ramp of v between lo and hi (hi < lo gives a falling ramp)."""
		n = self.node('ShaderNodeMapRange', {0: v, 1: lo, 2: hi, 3: 0.0, 4: 1.0}, clamp=True,
			interpolation_type='SMOOTHSTEP' if smooth and hi > lo else 'LINEAR')
		return n.outputs[0]

	def mix(self, fac, a, b):
		n = self.node('ShaderNodeMix', data_type='RGBA', clamp_factor=True)
		self.put(n.inputs[0], fac)
		self.put(n.inputs[6], a)
		self.put(n.inputs[7], b)
		return n.outputs[2]

	def noise(self, vec, scale, detail=2.0, rough=0.5, distortion=0.0, out='Factor'):
		n = self.node('ShaderNodeTexNoise', {'Vector': vec, 'Scale': scale, 'Detail': detail, 'Roughness': rough,
			'Distortion': distortion}, noise_dimensions='3D')
		return n.outputs[out]

	def mapping(self, vec, loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
		return self.node('ShaderNodeMapping', {'Vector': vec, 'Location': loc, 'Rotation': rot, 'Scale': scale}).outputs[0]

	def ramp(self, fac, stops):
		n = self.node('ShaderNodeValToRGB')
		els = n.color_ramp.elements
		els[0].position, els[1].position = stops[0][0], stops[-1][0]
		for pos, _ in stops[1:-1]:
			els.new(pos)
		for e, (_, col) in zip(els, stops):
			e.color = (*col, 1.0)
		self.put(n.inputs[0], fac)
		return n.outputs[0]

	def edge_wear(self, radius, lo, hi, breakup, amount):
		"""Wear on convex edges: Bevel-node curvature, kept off concave corners by local AO, broken up by noise."""
		bev = self.node('ShaderNodeBevel', {'Radius': radius}, samples=8)
		curv = self.math('SUBTRACT', 1.0, self.vec('DOT_PRODUCT', bev.outputs[0], self.normal))
		ao = self.node('ShaderNodeAmbientOcclusion', {'Distance': radius * 4}, samples=8, only_local=True)
		edge = self.math('MULTIPLY', self.remap(curv, lo, hi), self.remap(ao.outputs['AO'], 0.75, 0.97))
		n = self.noise(self.co, breakup, 5.0, 0.65)
		return self.remap(self.math('MULTIPLY', edge, self.math('ADD', n, amount)), 0.75, 1.05)

	def cavity(self, dist):
		ao = self.node('ShaderNodeAmbientOcclusion', {'Distance': dist}, samples=8)
		return self.remap(ao.outputs['AO'], 1.0, 0.5)

	def checkering(self, boxes, pitch):
		"""Diamond checkering height (0..0.5) inside axis-aligned boxes ((x0, x1), (y0, y1), (z0, z1)), on upward faces."""
		if not boxes:
			return 0.0
		x, y, z = (self.axis(self.co, i) for i in range(3))
		mask = None
		for box_ in boxes:
			m = 1.0
			for v, (a, b) in zip((x, y, z), box_):
				m = self.math('MULTIPLY', m, self.math('MULTIPLY', self.math('GREATER_THAN', v, a), self.math('LESS_THAN', v, b)))
			mask = m if mask is None else self.math('MAXIMUM', mask, m)
		mask = self.math('MULTIPLY', mask, self.remap(self.axis(self.normal, 2), 0.2, 0.55))
		tri = []
		for sgn in (1.0, -1.0):
			u = self.math('MULTIPLY', self.math('MULTIPLY_ADD', y, sgn, x), 0.7071 / pitch)
			tri.append(self.math('ABSOLUTE', self.math('SUBTRACT', self.math('FRACT', u), 0.5)))
		return self.math('MULTIPLY', self.math('MINIMUM', *tri), mask)

	def finish(self, color, rough, metal, height):
		"""Wire the Principled BSDF; OUT_* reroutes expose each channel to the baker."""
		for nm, s in (('color', color), ('rough', rough), ('metal', metal)):
			r = self.nt.nodes.new('NodeReroute')
			r.name = 'OUT_' + nm
			self.put(r.inputs[0], self.socket(s))
		bump = self.node('ShaderNodeBump', {'Strength': 1.0, 'Distance': 1.0, 'Height': height})
		bsdf = self.node('ShaderNodeBsdfPrincipled', {'Base Color': color, 'Roughness': rough, 'Metallic': metal,
			'Normal': bump.outputs[0]})
		bsdf.name = 'BSDF'
		out = self.node('ShaderNodeOutputMaterial')
		out.name = 'OUT'
		self.put(out.inputs['Surface'], bsdf.outputs[0])


def node_mat(name):
	m = bpy.data.materials.new(name)
	try:
		m.use_nodes = True
	except Exception:
		pass
	return m


def new_mat(name, bake_set):
	m = node_mat(name)
	m['bake_set'] = bake_set
	return m, Graph(m)


def mat_wood(light, mid, dark, axis, wear):
	"""Stained hardwood: growth rings around a tilted axis off to one side of the stock, so its side faces cut the
	rings nearly tangentially (flat-sawn cathedral figure) and its top runs straight-grained; pores, worn edges."""
	m, g = new_mat('M_Wood_Proc', 'Wood')
	# broad warp bends the rings into figure; a fine lengthwise-stretched warp keeps ring lines from looking drawn
	warp = g.vec('MULTIPLY', g.vec('SUBTRACT', g.noise(g.co, 5.0, 3.0, 0.5, out='Color'), (0.5, 0.5, 0.5)), (0.0, 0.010, 0.010))
	jitter = g.noise(g.mapping(g.co, scale=(6.0, 90.0, 90.0)), 1.0, 3.0, 0.6, out='Color')
	warp = g.vec('ADD', warp, g.vec('MULTIPLY', g.vec('SUBTRACT', jitter, (0.5, 0.5, 0.5)), (0.0, 0.0009, 0.0009)))
	p = g.mapping(g.vec('ADD', g.co, warp), loc=axis, rot=(0.0, math.radians(1.6), math.radians(2.4)))
	rings = g.node('ShaderNodeTexWave', {'Vector': p, 'Scale': 85.0, 'Distortion': 0.0}, wave_type='RINGS',
		rings_direction='X', wave_profile='SAW')
	# ring contrast varies across the board so some growth years read strongly and others barely
	contrast = g.remap(g.noise(g.co, 12.0, 2.0, 0.5), 0.3, 0.7)
	# latewood darkens toward the end of each ring; the hard SAW reset is eased so ring edges don't look cut out
	f = rings.outputs['Factor']
	late = g.math('MULTIPLY', g.remap(f, 0.68, 0.92), g.lin(1.0, (g.remap(f, 0.94, 1.0), -0.6)))
	late = g.math('MULTIPLY', late, g.lin(0.35, (contrast, 0.65)))
	pore = g.remap(g.noise(g.mapping(g.co, scale=(22.0, 1300.0, 1300.0)), 1.0, 2.0, 0.5), 0.60, 0.74)
	tone = g.remap(g.noise(g.co, 4.0, 2.0, 0.5), 0.3, 0.7)
	col = g.mix(tone, light, mid)
	col = g.mix(g.math('MULTIPLY', late, 0.5), col, dark)
	col = g.mix(g.math('MULTIPLY', pore, 0.45), col, dark)
	wear_ = g.edge_wear(0.0025, 0.004, 0.04, 90.0, wear)
	col = g.mix(g.math('MULTIPLY', wear_, 0.6), col, tuple(min(1.0, c * 1.6) for c in light))
	cav = g.cavity(0.012)
	col = g.mix(g.math('MULTIPLY', cav, 0.55), col, tuple(c * 0.35 for c in dark))
	smudge = g.noise(g.co, 9.0, 3.0, 0.6)
	rough = g.lin(0.30, (pore, 0.14), (late, -0.04), (wear_, 0.25), (cav, 0.25), (smudge, 0.08), clamp=True)
	peel = g.noise(g.co, 900.0, 2.0, 0.5)
	height = g.lin(0.0, (pore, -0.00012), (late, 0.00003), (peel, 0.000004))
	g.finish(col, rough, 0.0, height)
	return m


def metal_finish(g, col, rough_base, wear, checker, streak):
	"""Shared steel treatment: bright steel on worn edges, oily grime in crevices, smudges, pitting, checkering."""
	wear_ = g.edge_wear(0.0012, 0.003, 0.03, 160.0, wear)
	col = g.mix(wear_, col, srgb('#a3a4a7'))
	cav = g.cavity(0.006)
	col = g.mix(g.math('MULTIPLY', cav, 0.6), col, srgb('#17130f'))
	chk = g.checkering(checker, 0.0011)
	smudge = g.noise(g.co, 6.0, 3.0, 0.6)
	rough = g.lin(rough_base, (streak, 0.08), (smudge, 0.10), (wear_, -0.05), (cav, 0.3), clamp=True)
	pit = g.remap(g.noise(g.co, 2500.0, 1.0), 0.72, 0.8)
	height = g.lin(0.0, (streak, 0.000004), (chk, 0.0009), (pit, -0.00003))
	g.finish(col, rough, 1.0, height)


def mat_blued(wear, checker):
	"""Hot bluing: blue-black oxide with lengthwise polishing streaks."""
	m, g = new_mat('M_Blued_Proc', 'Metal')
	streak = g.noise(g.mapping(g.co, scale=(4.0, 2500.0, 2500.0)), 1.0, 3.0, 0.6)
	col = g.mix(g.math('MULTIPLY', streak, 0.6), srgb('#2c3038'), srgb('#4a5060'))
	metal_finish(g, col, 0.20, wear, checker, streak)
	return m


def mat_case(wear, fade, checker):
	"""Color case hardening: marbled quench colors over gray steel; fade dulls them toward gray."""
	m, g = new_mat('M_Case_Proc', 'Metal')
	hue = g.noise(g.co, 55.0, 6.0, 0.62, distortion=0.6)
	ramp = ['#26375f', '#4565a0', '#7a7e87', '#a48745', '#6c4a2e', '#5b3d70', '#2c4682']
	col = g.ramp(hue, [(0.30 + i * 0.07, srgb(c)) for i, c in enumerate(ramp)])
	strength = g.math('MULTIPLY', g.remap(g.noise(g.co, 30.0, 4.0, 0.6), 0.35, 0.6), 1.0 - fade)
	col = g.mix(strength, srgb('#8b8e94'), col)
	streak = g.noise(g.mapping(g.co, scale=(30.0, 600.0, 600.0)), 1.0, 2.0, 0.5)
	metal_finish(g, col, 0.26, wear, checker, streak)
	return m


def mat_plastic(rib_pitch):
	"""Black buttplate plastic with horizontal ribs on the butt face and dust caught in the grooves."""
	m, g = new_mat('M_Plastic_Proc', 'Metal')
	back = g.remap(g.axis(g.normal, 0), -0.6, -0.85, smooth=False)
	ribs = g.node('ShaderNodeTexWave', {'Vector': g.co, 'Scale': 0.314 / rib_pitch}, wave_type='BANDS',
		bands_direction='Z', wave_profile='SIN').outputs['Factor']
	groove = g.math('MULTIPLY', g.remap(ribs, 0.45, 0.15, smooth=False), back)
	col = g.mix(g.math('MULTIPLY', groove, 0.5), srgb('#141415'), srgb('#4a4846'))
	scuff = g.remap(g.noise(g.co, 120.0, 4.0, 0.6), 0.6, 0.75)
	rough = g.lin(0.42, (groove, 0.3), (scuff, 0.15), clamp=True)
	g.finish(col, rough, 0.0, g.math('MULTIPLY', groove, -0.0004))
	return m


def mat_brass():
	m, g = new_mat('M_Brass_Proc', 'Metal')
	tarnish = g.remap(g.noise(g.co, 900.0, 3.0, 0.6), 0.4, 0.7)
	col = g.mix(g.math('MULTIPLY', tarnish, 0.5), srgb('#e2c37a'), srgb('#8a6a35'))
	g.finish(col, g.lin(0.22, (tarnish, 0.2), clamp=True), 1.0, 0.0)
	return m


def mat_bore():
	m, g = new_mat('M_Bore_Proc', 'Metal')
	g.finish(srgb('#0b0b0c'), 0.55, 0.6, 0.0)
	return m


def gun_mats(wood, wear, case_fade, checker):
	return {
		'wood': mat_wood(*(srgb(c) for c in wood), axis=(0.0, 0.25, 0.03), wear=wear),
		'blued': mat_blued(wear, checker),
		'case': mat_case(wear, case_fade, checker),
		'plastic': mat_plastic(0.0026),
		'brass': mat_brass(),
		'bore': mat_bore(),
	}


# ------------------------------------------------------------- bake + export

def setup_cycles(samples):
	sc = bpy.context.scene
	sc.render.engine = 'CYCLES'
	prefs = bpy.context.preferences.addons['cycles'].preferences
	for kind in ('OPTIX', 'CUDA', 'HIP', 'ONEAPI', 'METAL'):
		try:
			prefs.compute_device_type = kind
			prefs.refresh_devices()
		except (TypeError, AttributeError):
			continue
		if any(d.type == kind for d in prefs.devices):
			for d in prefs.devices:
				d.use = d.type == kind
			sc.cycles.device = 'GPU'
			break
	sc.cycles.samples = samples
	sc.cycles.use_denoising = True


def studio_world():
	"""Light-gray backdrop for camera rays, dim neutral environment for lighting; AO distance for bakes."""
	w = bpy.data.worlds.new('Studio')
	bpy.context.scene.world = w
	nt = w.node_tree
	nt.nodes.clear()
	lp = nt.nodes.new('ShaderNodeLightPath')
	env = nt.nodes.new('ShaderNodeBackground')
	env.inputs['Color'].default_value = (0.62, 0.64, 0.68, 1)
	env.inputs['Strength'].default_value = 0.3
	bg = nt.nodes.new('ShaderNodeBackground')
	bg.inputs['Color'].default_value = (0.80, 0.81, 0.83, 1)
	mix = nt.nodes.new('ShaderNodeMixShader')
	out = nt.nodes.new('ShaderNodeOutputWorld')
	nt.links.new(lp.outputs['Is Camera Ray'], mix.inputs[0])
	nt.links.new(env.outputs[0], mix.inputs[1])
	nt.links.new(bg.outputs[0], mix.inputs[2])
	nt.links.new(mix.outputs[0], out.inputs['Surface'])
	w.light_settings.distance = 0.04


def join_parts(objs, name):
	bpy.ops.object.select_all(action='DESELECT')
	for o in objs:
		o.select_set(True)
	bpy.context.view_layer.objects.active = objs[0]
	bpy.ops.object.duplicate()
	bpy.ops.object.join()
	sm = bpy.context.active_object
	sm.name = sm.data.name = name
	bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
	sm.modifiers.new('Tri', 'TRIANGULATE').keep_custom_normals = True
	apply_mods(sm)
	return sm


def stash_parts(objs, name):
	"""Keep the separate parts (procedural materials, for re-baking or rigging) in an excluded collection."""
	parts = bpy.data.collections.new('Parts_' + name)
	bpy.context.scene.collection.children.link(parts)
	for o in objs:
		bpy.context.scene.collection.objects.unlink(o)
		parts.objects.link(o)
	parts.hide_render = True
	bpy.context.view_layer.layer_collection.children[parts.name].exclude = True


def unwrap_sets(sm):
	"""One UV atlas per texture set: each set's faces are smart-projected into their own 0..1 square."""
	me = sm.data
	for uv in list(me.uv_layers):
		me.uv_layers.remove(uv)
	me.uv_layers.new(name='UVMap')
	slot_set = [s.material['bake_set'] for s in sm.material_slots]
	activate(sm)
	for s in SETS:
		bpy.ops.object.mode_set(mode='EDIT')
		bpy.ops.mesh.select_all(action='DESELECT')
		bm = bmesh.from_edit_mesh(me)
		for f in bm.faces:
			if slot_set[f.material_index] == s:
				f.select_set(True)
		bmesh.update_edit_mesh(me)
		bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.003, correct_aspect=True, scale_to_bounds=False)
		bpy.ops.object.mode_set(mode='OBJECT')


def pixels(img):
	a = np.empty(img.size[0] * img.size[1] * 4, dtype=np.float32)
	img.pixels.foreach_get(a)
	return a.reshape(img.size[1], img.size[0], 4)


def save_png(img, path):
	img.filepath_raw = path
	img.file_format = 'PNG'
	img.save()


def bake_sets(sm, prefix):
	"""Bake the procedural materials into per-set D/N/ORM PNGs; returns {set: {'D'|'N'|'ORM': path}}."""
	sc = bpy.context.scene
	setup_cycles(8)
	sc.render.bake.margin = TEX_RES // 256
	sc.render.bake.margin_type = 'EXTEND'
	sc.render.bake.normal_space = 'TANGENT'
	imgs = {}
	for s in SETS:
		for ch in ('D', 'R', 'M', 'N', 'AO'):
			img = bpy.data.images.new(f'{prefix}_{s}_{ch}', TEX_RES, TEX_RES, alpha=False)
			img.colorspace_settings.name = 'sRGB' if ch == 'D' else 'Non-Color'
			imgs[s, ch] = img
	mats = list({sl.material for sl in sm.material_slots})
	for m in mats:
		nt = m.node_tree
		nt.nodes.new('ShaderNodeTexImage').name = 'BAKE_TARGET'
		nt.nodes.new('ShaderNodeEmission').name = 'BAKE_EMIT'
	for ch, kind, out, samples in (('D', 'EMIT', 'color', 8), ('R', 'EMIT', 'rough', 8), ('M', 'EMIT', 'metal', 4),
			('N', 'NORMAL', None, 8), ('AO', 'AO', None, 64)):
		sc.cycles.samples = samples
		for m in mats:
			nt = m.node_tree
			surf = nt.nodes['OUT'].inputs['Surface']
			if out:
				emit = nt.nodes['BAKE_EMIT']
				nt.links.new(nt.nodes['OUT_' + out].outputs[0], emit.inputs['Color'])
				nt.links.new(emit.outputs[0], surf)
			else:
				nt.links.new(nt.nodes['BSDF'].outputs[0], surf)
			target = nt.nodes['BAKE_TARGET']
			target.image = imgs[m['bake_set'], ch]
			nt.nodes.active = target
		activate(sm)
		bpy.ops.object.bake(type=kind, use_clear=True, margin=sc.render.bake.margin, normal_space='TANGENT',
			normal_r='POS_X', normal_g='POS_Y', normal_b='POS_Z')
		print(f'BAKED {prefix} {ch}')
	for m in mats:
		nt = m.node_tree
		nt.links.new(nt.nodes['BSDF'].outputs[0], nt.nodes['OUT'].inputs['Surface'])
		nt.nodes.remove(nt.nodes['BAKE_TARGET'])
		nt.nodes.remove(nt.nodes['BAKE_EMIT'])
	paths = {}
	for s in SETS:
		base = os.path.join(OUT, 'Textures', f'{prefix}_{s}_')
		paths[s] = {k: base + k + '.png' for k in ('D', 'N', 'ORM')}
		save_png(imgs[s, 'D'], paths[s]['D'])
		n = pixels(imgs[s, 'N'])
		n[..., 1] = 1.0 - n[..., 1]  # OpenGL -> DirectX green
		imgs[s, 'N'].pixels.foreach_set(n.ravel())
		save_png(imgs[s, 'N'], paths[s]['N'])
		orm = np.ones_like(n)
		for i, ch in enumerate(('AO', 'R', 'M')):
			orm[..., i] = pixels(imgs[s, ch])[..., 0]
		img = bpy.data.images.new(f'{prefix}_{s}_ORM', TEX_RES, TEX_RES, alpha=False)
		img.colorspace_settings.name = 'Non-Color'
		img.pixels.foreach_set(orm.ravel())
		save_png(img, paths[s]['ORM'])
		bpy.data.images.remove(img)
	for img in imgs.values():
		bpy.data.images.remove(img)
	return paths


def final_material(name, paths):
	"""Principled material on the baked maps (base color linked directly so the FBX exporter embeds it)."""
	m = node_mat(name)
	nt = m.node_tree
	nt.nodes.clear()

	def tex(key, color):
		t = nt.nodes.new('ShaderNodeTexImage')
		t.image = bpy.data.images.load(paths[key], check_existing=True)
		t.image.colorspace_settings.name = 'sRGB' if color else 'Non-Color'
		return t.outputs['Color']

	bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
	nt.links.new(bsdf.outputs[0], nt.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
	nt.links.new(tex('D', True), bsdf.inputs['Base Color'])
	orm = nt.nodes.new('ShaderNodeSeparateColor')
	nt.links.new(tex('ORM', False), orm.inputs[0])
	nt.links.new(orm.outputs['Green'], bsdf.inputs['Roughness'])
	nt.links.new(orm.outputs['Blue'], bsdf.inputs['Metallic'])
	# the file is DirectX (green down) for UE; flip back to Blender's OpenGL convention here
	n = nt.nodes.new('ShaderNodeSeparateColor')
	nt.links.new(tex('N', False), n.inputs[0])
	inv = nt.nodes.new('ShaderNodeMath')
	inv.operation = 'SUBTRACT'
	inv.inputs[0].default_value = 1.0
	nt.links.new(n.outputs['Green'], inv.inputs[1])
	comb = nt.nodes.new('ShaderNodeCombineColor')
	nt.links.new(n.outputs['Red'], comb.inputs['Red'])
	nt.links.new(inv.outputs[0], comb.inputs['Green'])
	nt.links.new(n.outputs['Blue'], comb.inputs['Blue'])
	nmap = nt.nodes.new('ShaderNodeNormalMap')
	nmap.uv_map = 'UVMap'
	nt.links.new(comb.outputs[0], nmap.inputs['Color'])
	nt.links.new(nmap.outputs[0], bsdf.inputs['Normal'])
	return m


def assign_sets(sm, mats):
	"""Collapse the procedural slots to one slot per texture set (indices are reapplied after clear(), which zeroes them)."""
	slot_set = [sl.material['bake_set'] for sl in sm.material_slots]
	idx = [SETS.index(slot_set[p.material_index]) for p in sm.data.polygons]
	sm.data.materials.clear()
	for s in SETS:
		sm.data.materials.append(mats[s])
	sm.data.polygons.foreach_set('material_index', idx)
	sm.data.update()


def export_fbx(sm):
	bpy.ops.object.select_all(action='DESELECT')
	activate(sm)
	bpy.ops.export_scene.fbx(
		filepath=os.path.join(OUT, sm.name + '.fbx'),
		use_selection=True, object_types={'MESH', 'EMPTY'},
		apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
		mesh_smooth_type='FACE', use_tspace=True,
		path_mode='COPY', embed_textures=True,
		add_leaf_bones=False, bake_anim=False)
	print(f'EXPORTED {sm.name}: {len(sm.data.polygons)} tris, {len(sm.data.materials)} materials')


# ------------------------------------------------------------- previews

def aim(obj, target):
	obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def area(name, size, size_y, loc, target, energy):
	light = bpy.data.lights.new(name, 'AREA')
	light.shape = 'RECTANGLE'
	light.size, light.size_y = size, size_y
	light.energy = energy
	o = bpy.data.objects.new(name, light)
	bpy.context.scene.collection.objects.link(o)
	o.location = loc
	aim(o, target)
	return o


def studio(side):
	"""Softbox rig on the camera's side (side = sign of camera Y): key, fill, overhead strip, far-side rim."""
	c = (0.15, 0.0, 0.0)
	return [
		area('Key', 1.4, 0.6, (0.45, side * 0.95, 0.80), c, 16),
		area('Fill', 1.2, 1.2, (-0.45, side * 1.1, -0.35), c, 4.5),
		area('Strip', 2.4, 0.22, (0.15, side * 0.30, 0.95), (0.15, side * 0.05, 0.0), 9),
		area('Rim', 1.0, 0.5, (0.10, -side * 0.9, 0.55), c, 8),
	]


def shoot(render_name, ortho, details, folder):
	"""Cycles previews: orthographic right/left plus perspective details ({tag: (camera location, look-at)})."""
	sc = bpy.context.scene
	setup_cycles(32 if QUICK else 192)
	sc.render.resolution_x, sc.render.resolution_y = 1600, 700
	sc.render.film_transparent = False
	for look in ('AgX - Medium High Contrast', 'Medium High Contrast'):
		try:
			sc.view_settings.view_transform = 'AgX'
			sc.view_settings.look = look
			break
		except TypeError:
			continue
	cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
	sc.collection.objects.link(cam)
	sc.camera = cam
	cam.data.clip_start = 0.01
	cx, cz, scale = ortho
	shots = [(tag, 'ORTHO', (cx, side * 2.0, cz), (cx, 0.0, cz)) for tag, side in (('right', -1), ('left', 1))]
	shots += [(tag, 'PERSP', loc, target) for tag, (loc, target) in details.items()]
	os.makedirs(folder, exist_ok=True)
	for tag, kind, loc, target in shots:
		cam.data.type = kind
		cam.data.ortho_scale = scale
		cam.data.lens = 50
		cam.location = loc
		aim(cam, target)
		cam.data.dof.use_dof = kind == 'PERSP'
		cam.data.dof.focus_distance = (Vector(loc) - Vector(target)).length
		cam.data.dof.aperture_fstop = 8.0
		lights = studio(1 if loc[1] > 0 else -1)
		sc.render.filepath = os.path.join(folder, f'{render_name}_{tag}.png')
		bpy.ops.render.render(write_still=True)
		for o in lights:
			bpy.data.objects.remove(o, do_unlink=True)
	bpy.data.objects.remove(cam, do_unlink=True)


def finish_photoreal(name, render_name, prefix, ortho, details):
	objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
	sm = join_parts(objs, 'SM_' + name)
	stash_parts(objs, name)
	studio_world()
	unwrap_sets(sm)
	if QUICK:
		shoot(render_name, ortho, details, os.path.join(OUT, 'Previews', '_quick'))
		return
	paths = bake_sets(sm, prefix)
	assign_sets(sm, {s: final_material(f'M_{prefix[2:]}_{s}', paths[s]) for s in SETS})
	export_fbx(sm)
	shoot(render_name, ortho, details, os.path.join(OUT, 'Previews'))
	blend = os.path.join(OUT, 'src', name + '.blend')
	bpy.context.preferences.filepaths.save_version = 0  # no .blend1 from the second save below
	bpy.ops.wm.save_as_mainfile(filepath=blend, compress=False)
	bpy.ops.file.make_paths_relative()
	bpy.ops.wm.save_as_mainfile(filepath=blend, compress=False)


def barrel(prof, mat, z, section=0.13):
	"""Lathe the barrel in ~section-long pieces so the UV packer gets short islands instead of full-length strips
	(cuts only where the outer profile runs forward, i.e. between the breech face and the muzzle crown)."""
	x0 = prof[2][0]
	cuts = [x0 + section * (i + 1) for i in range(int((prof[-5][0] - x0) // section))]
	pts = [prof[0]]
	for p, q in zip(prof, prof[1:]):
		for c in cuts:
			if p[0] < c < q[0]:
				pts.append((c, p[1] + (q[1] - p[1]) * (c - p[0]) / (q[0] - p[0])))
		pts.append(q)
	splits = [i for i, p in enumerate(pts) if p[0] in cuts]
	for k, (a, b) in enumerate(zip([0] + splits, splits + [len(pts) - 1])):
		lathe(f'Barrel{k}', pts[a:b + 1], mat, z=z, steps=64)


def slotted(screw, x0, x1, y0, y1, z0, z1):
	"""Cut a screwdriver slot (box) into a screw head."""
	boolean_cut(screw, box('SlotCutter', x0, x1, y0, y1, z0, z1, screw.data.materials[0], bev=0))


# ------------------------------------------------------------- NEF Pardner .410

def build_pardner():
	reset()
	zb = 0.040  # bore axis
	xb = 0.028  # breech face against the standing breech
	xr = -0.017  # frame rear / stock joint
	xm = xb + 0.660  # 26 in barrel
	hx, hz, hr = 0.072, 0.0115, 0.016  # hinge pin and the knuckle radius around it
	bottom = hz - hr  # frame underside
	r_ch, r_mz, r_bore = 0.0125, 0.0076, 0.0052  # chamber OD, muzzle OD, .410 bore
	spur = ((xb - 0.052, xb - 0.033), (-0.01, 0.01), (zb + 0.024, zb + 0.040))
	P = gun_mats(('#7a4a2a', '#5a3218', '#2a1408'), wear=0.35, case_fade=0.15, checker=[spur])

	# frame: standing breech, bar under the chamber, rounded knuckle around the hinge pin
	knuckle = [(hx + hr * math.cos(math.radians(a)), hz + hr * math.sin(math.radians(a))) for a in range(90, -91, -15)]
	profile_solid('Frame', [(xr, bottom), (xr, zb + 0.0105), (xb, zb + 0.0125), (xb, zb - r_ch)] + knuckle, [],
		[(xr, 0.028), (xb, 0.027), (hx + hr, 0.026)], P['case'], 0.0025, 3, smooth_iters=0)
	for nm, r, px, pz in (('HingePin', 0.0042, hx, hz), ('HammerPin', 0.0024, xb - 0.026, zb - 0.012),
			('TriggerPin', 0.0022, -0.006, 0.008)):
		post(nm, r, -0.0140, 0.0140, 'Y', px, 0, pz, P['blued'], seg=24)
	# exposed hammer (down), checkered spur over the wrist
	profile_solid('Hammer', [(xb - 0.004, zb + 0.002), (xb - 0.004, zb + 0.016), (xb - 0.008, zb + 0.021),
		(xb - 0.018, zb + 0.025), (xb - 0.032, zb + 0.031), (xb - 0.044, zb + 0.036), (xb - 0.0495, zb + 0.0335),
		(xb - 0.046, zb + 0.029), (xb - 0.034, zb + 0.0225), (xb - 0.026, zb + 0.014), (xb - 0.024, zb + 0.002)],
		[0, 10], [(xb - 0.05, 0.0115), (xb - 0.032, 0.0085), (xb - 0.01, 0.0075)], P['case'], 0.0012, 2)
	# barrel release lever on the right side of the frame
	lever = profile_solid('ReleaseLever', [(xb - 0.002, zb - 0.0035), (xb - 0.002, zb + 0.0045), (xb - 0.020, zb + 0.006),
		(xb - 0.034, zb + 0.0075), (xb - 0.040, zb + 0.006), (xb - 0.040, zb - 0.001), (xb - 0.030, zb - 0.002),
		(xb - 0.014, zb - 0.004)], [0, 1], [(-1, 0.0024), (1, 0.0024)], P['blued'], 0.0006, 2)
	lever.location.y = -0.0148
	box('ReleaseTab', xb - 0.0405, xb - 0.031, -0.0185, -0.0140, zb - 0.0015, zb + 0.0085, P['blued'], bev=0.0012)
	post('ReleasePivot', 0.0038, -0.0135, -0.0168, 'Y', xb - 0.006, 0, zb + 0.0005, P['blued'], seg=24)
	tube('TriggerGuard', [(0.034, 0, bottom + 0.002), (0.033, 0, -0.020), (0.020, 0, -0.031), (-0.012, 0, -0.031),
		(-0.027, 0, -0.023), (-0.036, 0, -0.012), (-0.042, 0, -0.008)], 0.0026, P['blued'], scale_y=2.5)
	tube('Trigger', [(0.004, 0, bottom + 0.003), (0.006, 0, -0.010), (0.002, 0, -0.019), (-0.006, 0, -0.024)],
		0.0017, P['blued'], scale_y=1.8)

	# barrel: chamber section, taper, recessed bore, brass bead
	barrel([(xb, 0.0), (xb, r_ch - 0.0012), (xb + 0.0012, r_ch), (xb + 0.074, r_ch), (xb + 0.086, 0.0102),
		(xb + 0.20, 0.0094), (xm - 0.003, r_mz), (xm, r_mz - 0.0006), (xm, r_bore), (xm - 0.015, r_bore),
		(xm - 0.015, 0.0)], P['blued'], zb)
	cyl('Muzzle', r_bore, r_bore, xm - 0.0152, xm - 0.0148, P['bore'], z=zb, seg=24)
	sphere('FrontBead', 0.0019, (xm - 0.008, 0, zb + r_mz + 0.0011), P['brass'])

	# forend: concave rear wraps the knuckle, screwed to the barrel from underneath
	arc = [(hx + (hr + 0.0006) * math.cos(math.radians(a)), hz + (hr + 0.0006) * math.sin(math.radians(a)))
		for a in range(-75, 91, 15)]
	fore = [(hx + 0.0006, zb), (0.300, zb), (0.311, zb - 0.005), (0.316, zb - 0.015), (0.312, zb - 0.025),
		(0.300, zb - 0.030), (0.20, zb - 0.036), (0.12, zb - 0.042), (0.09, bottom)]
	profile_solid('Forend', fore + arc, [0] + list(range(len(fore), len(fore) + len(arc))),
		[(0.07, 0.040), (0.20, 0.038), (0.32, 0.033)], P['wood'], 0.008, 5)
	screw = post('ForendScrew', 0.0034, 0.0022, 0.0046, 'Z', 0.19, 0, 0, P['blued'], seg=24)
	slotted(screw, 0.185, 0.195, -0.00035, 0.00035, 0.0012, 0.0028)

	# buttstock: dropped comb, semi-pistol grip, 14.4 in length of pull
	toe, heel = (-0.367, -0.142), (-0.360, -0.009)
	prof = [toe, heel, (-0.30, 0.0005), (-0.22, 0.0095), (-0.16, 0.016), (-0.12, 0.019), (-0.085, 0.026),
		(-0.055, 0.038), (-0.030, 0.0475), (xr, zb + 0.0105), (xr, bottom), (-0.030, -0.006), (-0.040, -0.012),
		(-0.046, -0.022), (-0.054, -0.040), (-0.064, -0.058), (-0.076, -0.074), (-0.088, -0.085), (-0.100, -0.089),
		(-0.116, -0.084), (-0.130, -0.074), (-0.145, -0.066), (-0.165, -0.065), (-0.21, -0.077), (-0.26, -0.095),
		(-0.31, -0.115)]
	profile_solid('Stock', prof, [0, 1, 9, 10], [(-0.37, 0.041), (-0.25, 0.038), (-0.13, 0.033),
		(-0.08, 0.030), (-0.04, 0.0285), (xr, 0.028)], P['wood'], 0.0105, 6)
	buttplate(toe, heel, 0.041, P)

	finish_photoreal('Shotgun_Pardner_410', 'Pardner_410', 'T_Pardner410', (0.157, -0.035, 1.25), {
		'detail': ((0.30, -0.46, 0.19), (0.005, 0.0, 0.025)),
		'detail_left': ((0.26, 0.44, 0.17), (0.005, 0.0, 0.025)),
	})


def buttplate(toe, heel, width, P, bt=0.0065):
	"""Ribbed plastic buttplate on the butt line toe->heel, with two slotted screws."""
	profile_solid('Buttplate', [(toe[0] + 0.0005, toe[1]), (heel[0] + 0.0005, heel[1]), (heel[0] - bt, heel[1] + 0.0005),
		(toe[0] - bt, toe[1] - 0.0005)], [0, 1, 2, 3], [(-0.5, width), (-0.3, width)], P['plastic'], 0.004, 3, smooth_iters=0)
	for nm, t in (('ButtScrewTop', 0.16), ('ButtScrewBottom', 0.84)):
		sz = heel[1] + t * (toe[1] - heel[1])
		sx = heel[0] - bt + t * (toe[0] - heel[0])
		screw = cyl(nm, 0.0030, 0.0034, sx - 0.0009, sx + 0.001, P['blued'], z=sz, seg=24)
		slotted(screw, sx - 0.002, sx - 0.0003, -0.00035, 0.00035, sz - 0.005, sz + 0.005)


# ------------------------------------------------------------- Stevens 940D .410

def build_stevens():
	"""Savage/Stevens 940-series: top lever, long hammer spur lying along the wrist, case-colored frame whose
	side walls hug the barrel, D guard with a rear tang, slim forend. .410 with a 26 in barrel."""
	reset()
	zb, r_ch, r_mz, r_bore = 0.034, 0.0118, 0.0076, 0.0052
	xr = 0.012  # stock joint at the frame bottom (the trigger sits just behind it)
	xb = xr + 0.032  # breech face
	xf = xr + 0.117  # frame front / forend joint
	xm = xb + 0.660
	bottom = -0.0035
	ztop = zb + r_ch  # barrel top at the breech
	zw = zb - r_ch + 0.0035  # side-wall top
	paddle = ((xr - 0.069, xr - 0.049), (-0.01, 0.01), (0.022, 0.042))
	lever_tip = ((xb - 0.034, xb - 0.021), (-0.01, 0.01), (0.056, 0.066))
	P = gun_mats(('#9c6b44', '#7a4c2a', '#3e2412'), wear=0.6, case_fade=0.35, checker=[paddle, lever_tip])

	# frame: rounded standing-breech hump at the rear, low side walls forward of the breech
	J = [(xr, bottom), (xr - 0.0006, 0.010), (xr - 0.0016, 0.022), (xr - 0.003, 0.032), (xr - 0.0042, 0.0385),
		(xr - 0.0036, 0.0420)]  # stock joint, bottom to top (shared with the stock)
	hump = [(xr - 0.001, 0.0438), (xr + 0.006, 0.0450), (xr + 0.016, ztop - 0.0002), (xb - 0.006, ztop + 0.0003), (xb, ztop)]
	profile_solid('Frame', J + hump + [(xb, zw), (xf, zw), (xf, bottom)], [], [(xr, 0.0295), (xf, 0.0295)],
		P['case'], 0.003, 3, smooth_iters=0)
	for nm, r, px, pz in (('HammerPin', 0.0022, xr + 0.010, 0.021), ('SearPin', 0.0022, xr + 0.005, 0.008),
			('HingePin', 0.0032, xf - 0.012, 0.005)):
		post(nm, r, -0.0150, 0.0150, 'Y', px, 0, pz, P['blued'], seg=24)
	# top lever: hook rising behind the breech, checkered tip over the joint (pushed right to open)
	profile_solid('TopLever', [(xb - 0.002, 0.044), (xb - 0.002, 0.0495), (xb - 0.007, 0.0545), (xb - 0.015, 0.0575),
		(xb - 0.024, 0.0600), (xb - 0.031, 0.0615), (xb - 0.0325, 0.0590), (xb - 0.025, 0.0565), (xb - 0.016, 0.0540),
		(xb - 0.010, 0.0500), (xb - 0.011, 0.044)], [0, 10], [(xb - 0.034, 0.0085), (xb - 0.02, 0.0075), (xb, 0.0055)],
		P['blued'], 0.0009, 2)
	# hammer: pivots in the frame, long arm lies along the wrist, checkered paddle angled up at the end
	profile_solid('Hammer', [(xr + 0.010, 0.029), (xr + 0.010, 0.037), (xr - 0.008, 0.034), (xr - 0.026, 0.0285),
		(xr - 0.042, 0.026), (xr - 0.054, 0.0295), (xr - 0.064, 0.0345), (xr - 0.067, 0.032), (xr - 0.062, 0.027),
		(xr - 0.052, 0.0215), (xr - 0.040, 0.0205), (xr - 0.024, 0.0235), (xr - 0.006, 0.029)], [0, 1],
		[(xr - 0.07, 0.0105), (xr - 0.05, 0.0100), (xr - 0.044, 0.0060), (xr + 0.01, 0.0055)], P['blued'], 0.0009, 2)
	tube('TriggerGuard', [(0.037, 0, -0.001), (0.036, 0, -0.016), (0.028, 0, -0.026), (0.014, 0, -0.030),
		(0.000, 0, -0.029), (-0.011, 0, -0.022), (-0.015, 0, -0.012), (-0.018, 0, -0.0085), (-0.030, 0, -0.0105),
		(-0.046, 0, -0.0150)], 0.0026, P['blued'], scale_y=2.6)
	screw = post('GuardScrew', 0.0026, -0.0170, -0.0150, 'Z', -0.040, 0, 0, P['blued'], seg=24)
	slotted(screw, -0.0435, -0.0365, -0.0003, 0.0003, -0.0180, -0.0162)
	tube('Trigger', [(0.003, 0, -0.001), (0.004, 0, -0.009), (0.001, 0, -0.017), (-0.005, 0, -0.022)],
		0.0016, P['blued'], scale_y=1.8)

	barrel([(xb, 0.0), (xb, r_ch - 0.001), (xb + 0.001, r_ch), (xf + 0.008, r_ch), (xf + 0.022, 0.0112),
		(xf + 0.045, 0.0102), (xb + 0.24, 0.0093), (xm - 0.003, r_mz), (xm, r_mz - 0.0006), (xm, r_bore), (xm - 0.015, r_bore),
		(xm - 0.015, 0.0)], P['blued'], zb)
	cyl('Muzzle', r_bore, r_bore, xm - 0.0152, xm - 0.0148, P['bore'], z=zb, seg=24)
	sphere('FrontBead', 0.0019, (xm - 0.008, 0, zb + r_mz + 0.0011), P['brass'])

	# forend: butts square against the frame front, tapers to a rounded tip
	profile_solid('Forend', [(xf + 0.0005, zw + 0.0015), (0.335, zw + 0.0015), (0.350, 0.0245), (0.358, 0.018),
		(0.357, 0.010), (0.349, 0.0045), (0.33, 0.0020), (0.25, -0.0015), (0.18, -0.005), (xf + 0.0005, -0.0075)],
		[0, 9], [(xf, 0.036), (0.25, 0.034), (0.36, 0.030)], P['wood'], 0.0075, 5)
	screw = post('ForendScrew', 0.0034, -0.0050, -0.0028, 'Z', 0.20, 0, 0, P['blued'], seg=24)
	slotted(screw, 0.195, 0.205, -0.00035, 0.00035, -0.0060, -0.0044)

	# buttstock: steep wrist drop under the hammer, long slanted pistol grip, 14.5 in length of pull
	toe, heel = (-0.367, -0.131), (-0.360, -0.016)
	top = [(-0.30, -0.007), (-0.22, 0.0015), (-0.14, 0.008), (-0.09, 0.011), (-0.057, 0.015), (-0.036, 0.021),
		(-0.018, 0.027), (0.000, 0.034)]
	under = [(0.000, -0.0055), (-0.020, -0.009), (-0.036, -0.012), (-0.052, -0.021), (-0.068, -0.036), (-0.084, -0.051),
		(-0.097, -0.060), (-0.109, -0.063), (-0.121, -0.0605), (-0.140, -0.059), (-0.20, -0.074), (-0.28, -0.101),
		(-0.33, -0.117)]
	joint = list(range(2 + len(top), 2 + len(top) + len(J)))
	profile_solid('Stock', [toe, heel] + top + J[::-1] + under, [0, 1] + joint, [(-0.37, 0.040), (-0.25, 0.038),
		(-0.12, 0.032), (-0.07, 0.030), (-0.03, 0.0295), (xr, 0.0295)], P['wood'], 0.0105, 6)
	buttplate(toe, heel, 0.040, P)

	finish_photoreal('Shotgun_Stevens_940D', 'Stevens_940D', 'T_Stevens940D', (0.166, -0.03, 1.26), {
		'detail': ((0.30, -0.46, 0.19), (0.015, 0.0, 0.02)),
		'detail_left': ((0.26, 0.44, 0.17), (0.015, 0.0, 0.02)),
	})


GUNS = {'pardner': build_pardner, 'stevens': build_stevens}
for key, build in GUNS.items():
	if not ONLY or key in ONLY:
		build()
print('ALL DONE')
