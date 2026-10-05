"""Procedurally build three .22 rifles in Blender and export UE-ready FBX.

Run: blender -b -P build_rifles.py -- <out_dir>
Conventions: meters in Blender (exported as cm), +X = muzzle, +Z = up,
Blender -Y = rifle's right side (becomes UE +Y). Origin at the trigger.
"""
import bpy
import bmesh
import math
import os
import sys
from mathutils import Vector

OUT = sys.argv[sys.argv.index('--') + 1]
TEX = os.path.join(OUT, 'Textures', 'T_Kryptek_Camo_D.png')


# ---------------------------------------------------------------- helpers

def reset():
	bpy.ops.wm.read_factory_settings(use_empty=True)
	bpy.context.scene.unit_settings.system = 'METRIC'
	bpy.context.scene.unit_settings.scale_length = 1.0


def make_mat(name, color, metal=0.0, rough=0.5, image=None):
	m = bpy.data.materials.new(name)
	try:
		m.use_nodes = True
	except Exception:
		pass
	nt = m.node_tree
	bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
	bsdf.inputs['Base Color'].default_value = (*color, 1)
	bsdf.inputs['Metallic'].default_value = metal
	bsdf.inputs['Roughness'].default_value = rough
	m.diffuse_color = (*color, 1)
	if image:
		tex = nt.nodes.new('ShaderNodeTexImage')
		tex.image = bpy.data.images.load(image)
		nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
	return m


def link(obj, mat):
	bpy.context.collection.objects.link(obj) if obj.name not in bpy.context.collection.objects else None
	obj.data.materials.clear()
	obj.data.materials.append(mat)
	return obj


def activate(obj):
	bpy.ops.object.select_all(action='DESELECT')
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)


def apply_mods(obj):
	activate(obj)
	for m in list(obj.modifiers):
		bpy.ops.object.modifier_apply(modifier=m.name)


def bevel(obj, width, segs=3, angle=35, harden=True):
	m = obj.modifiers.new('Bevel', 'BEVEL')
	m.width = width
	m.segments = segs
	m.limit_method = 'ANGLE'
	m.angle_limit = math.radians(angle)
	m.use_clamp_overlap = True
	m.harden_normals = harden
	apply_mods(obj)
	for p in obj.data.polygons:
		p.use_smooth = True
	return obj


def smooth_by_angle(obj, angle=40):
	bm = bmesh.new()
	bm.from_mesh(obj.data)
	for f in bm.faces:
		f.smooth = True
	for e in bm.edges:
		if not e.is_manifold or e.calc_face_angle(0) > math.radians(angle):
			e.smooth = False
	bm.to_mesh(obj.data)
	bm.free()


def chaikin(pts, sharp, iters=3):
	sharp = set(sharp)
	for _ in range(iters):
		new, nsharp = [], set()
		n = len(pts)
		for i in range(n):
			p, q = pts[i], pts[(i + 1) % n]
			if i in sharp:
				nsharp.add(len(new))
				new.append(p)
			else:
				new.append((0.75 * p[0] + 0.25 * q[0], 0.75 * p[1] + 0.25 * q[1]))
			if (i + 1) % n not in sharp:
				new.append((0.25 * p[0] + 0.75 * q[0], 0.25 * p[1] + 0.75 * q[1]))
		pts, sharp = new, nsharp
	return pts


def lerp_table(table, x):
	if x <= table[0][0]:
		return table[0][1]
	for (x0, v0), (x1, v1) in zip(table, table[1:]):
		if x <= x1:
			t = (x - x0) / (x1 - x0)
			t = t * t * (3 - 2 * t)
			return v0 + (v1 - v0) * t
	return table[-1][1]


def profile_solid(name, pts, sharp, widths, mat, bev, segs=4, smooth_iters=3):
	"""Side profile (x, z) extruded symmetrically in Y with an x-dependent width."""
	pts = chaikin(pts, sharp, smooth_iters) if smooth_iters else pts
	bm = bmesh.new()
	L = [bm.verts.new((x, lerp_table(widths, x) / 2, z)) for x, z in pts]
	R = [bm.verts.new((x, -lerp_table(widths, x) / 2, z)) for x, z in pts]
	bm.faces.new(L)
	bm.faces.new(list(reversed(R)))
	n = len(pts)
	for i in range(n):
		j = (i + 1) % n
		bm.faces.new((L[i], R[i], R[j], L[j]))
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	obj = bpy.data.objects.new(name, me)
	link(obj, mat)
	return bevel(obj, bev, segs, angle=30)


def cyl(name, r0, r1, x0, x1, mat, y=0.0, z=0.0, seg=32):
	bpy.ops.mesh.primitive_cone_add(vertices=seg, radius1=r0, radius2=r1, depth=x1 - x0, location=((x0 + x1) / 2, y, z), rotation=(0, math.pi / 2, 0))
	obj = bpy.context.active_object
	obj.name = name
	link(obj, mat)
	smooth_by_angle(obj)
	return obj


def box(name, x0, x1, y0, y1, z0, z1, mat, bev=0.0008):
	bpy.ops.mesh.primitive_cube_add(size=1, location=((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = (x1 - x0, y1 - y0, z1 - z0)
	activate(obj)
	bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
	link(obj, mat)
	if bev:
		bevel(obj, bev, 2, angle=60)
	else:
		smooth_by_angle(obj)
	return obj


def sphere(name, r, loc, mat):
	bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, radius=r, location=loc)
	obj = bpy.context.active_object
	obj.name = name
	link(obj, mat)
	smooth_by_angle(obj, 80)
	return obj


def torus(name, major, minor, loc, mat, axis='Z'):
	rot = {'X': (0, math.pi / 2, 0), 'Y': (math.pi / 2, 0, 0), 'Z': (0, 0, 0)}[axis]
	bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=24, minor_segments=8, location=loc, rotation=rot)
	obj = bpy.context.active_object
	obj.name = name
	link(obj, mat)
	smooth_by_angle(obj, 80)
	return obj


def tube(name, pts, radius, mat, scale_y=1.0, smooth=2):
	"""Swept round tube along (x, y, z) points; scale_y flattens it into a strap/blade."""
	for _ in range(smooth):
		out = [pts[0]]
		for p, q in zip(pts, pts[1:]):
			out.append(tuple(0.75 * a + 0.25 * b for a, b in zip(p, q)))
			out.append(tuple(0.25 * a + 0.75 * b for a, b in zip(p, q)))
		out.append(pts[-1])
		pts = out
	cu = bpy.data.curves.new(name, 'CURVE')
	cu.dimensions = '3D'
	cu.bevel_depth = radius
	cu.bevel_resolution = 3
	cu.use_fill_caps = True
	sp = cu.splines.new('POLY')
	sp.points.add(len(pts) - 1)
	for p, c in zip(sp.points, pts):
		p.co = (*c, 1)
	obj = bpy.data.objects.new(name, cu)
	bpy.context.collection.objects.link(obj)
	activate(obj)
	bpy.ops.object.convert(target='MESH')
	obj = bpy.context.active_object
	obj.scale = (1, scale_y, 1)
	bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
	link(obj, mat)
	smooth_by_angle(obj, 60)
	return obj


def boolean_cut(target, cutter):
	m = target.modifiers.new('Bool', 'BOOLEAN')
	m.operation = 'DIFFERENCE'
	m.object = cutter
	m.solver = 'EXACT'
	apply_mods(target)
	bpy.data.objects.remove(cutter, do_unlink=True)


def sling_stud(prefix, x, z, mat):
	bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.0025, depth=0.009, location=(x, 0, z - 0.0035))
	o = bpy.context.active_object
	o.name = prefix + '_Post'
	link(o, mat)
	smooth_by_angle(o)
	torus(prefix + '_Ring', 0.0035, 0.0012, (x, 0, z - 0.009), mat, axis='Y')


def lathe(name, prof, mat, z=0.0, steps=48, closed=False):
	"""Revolve an (x, r) profile around the X axis at height z."""
	bm = bmesh.new()
	verts = [bm.verts.new((x, 0.0, r)) for x, r in prof]
	edges = [bm.edges.new((a, b)) for a, b in zip(verts, verts[1:])]
	if closed:
		edges.append(bm.edges.new((verts[-1], verts[0])))
	bmesh.ops.spin(bm, geom=verts + edges, cent=(0, 0, 0), axis=(1, 0, 0), angle=2 * math.pi, steps=steps, use_merge=True)
	bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	for v in bm.verts:
		v.co.z += z
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	obj = bpy.data.objects.new(name, me)
	link(obj, mat)
	smooth_by_angle(obj, 40)
	return obj


def post(name, r, a, b, axis, x, y, z, mat, seg=32):
	"""Cylinder of radius r spanning a..b along the Y or Z axis, centered at (x, y, z) on the other axes."""
	if axis == 'Y':
		loc, rot = (x, (a + b) / 2, z), (math.pi / 2, 0, 0)
	else:
		loc, rot = (x, y, (a + b) / 2), (0, 0, 0)
	bpy.ops.mesh.primitive_cylinder_add(vertices=seg, radius=r, depth=abs(b - a), location=loc, rotation=rot)
	obj = bpy.context.active_object
	obj.name = name
	link(obj, mat)
	smooth_by_angle(obj)
	return obj


def export_joined(objs, sm_name, pivot=(0.0, 0.0, 0.0), sockets=None):
	"""Join copies of objs into one triangulated static mesh with its origin at pivot, and export it as FBX.

	sockets maps a name to a world position; each becomes a SOCKET_<name> empty that Unreal imports as a mesh socket.
	"""
	bpy.ops.object.select_all(action='DESELECT')
	for o in objs:
		o.select_set(True)
	bpy.context.view_layer.objects.active = objs[0]
	bpy.ops.object.duplicate()
	bpy.ops.object.join()
	sm = bpy.context.active_object
	sm.name = sm_name
	sm.data.name = sm_name
	bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
	for v in sm.data.vertices:
		v.co -= Vector(pivot)
	sm.modifiers.new('Tri', 'TRIANGULATE').keep_custom_normals = True
	apply_mods(sm)
	for socket_name, loc in (sockets or {}).items():
		e = bpy.data.objects.new('SOCKET_' + socket_name, None)
		bpy.context.collection.objects.link(e)
		e.empty_display_type = 'ARROWS'
		e.empty_display_size = 0.03
		e.parent = sm
		e.location = Vector(loc) - Vector(pivot)
		e.select_set(True)
	bpy.ops.export_scene.fbx(
		filepath=os.path.join(OUT, sm_name + '.fbx'),
		use_selection=True, object_types={'MESH', 'EMPTY'},
		apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
		mesh_smooth_type='FACE', use_tspace=True,
		path_mode='COPY', embed_textures=True,
		add_leaf_bones=False, bake_anim=False)
	tris = sum(len(p.vertices) - 2 for p in sm.data.polygons)
	print(f'EXPORTED {sm_name}: {tris} tris, {len(sm.data.materials)} materials')
	return sm


def finish(name, render_name, optics=None, socket=None):
	"""Export the rifle, and each optic ({name: part objects}) as its own mesh pivoted on the rifle's optic socket."""
	objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
	for o in objs:
		activate(o)
		bpy.ops.object.mode_set(mode='EDIT')
		bpy.ops.mesh.select_all(action='SELECT')
		bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.01)
		bpy.ops.object.mode_set(mode='OBJECT')
		for uv in list(o.data.uv_layers)[1:]:
			o.data.uv_layers.remove(uv)
		if o.data.uv_layers:
			o.data.uv_layers[0].name = 'UVMap'
	optics = optics or {}
	optic_parts = {o for parts in optics.values() for o in parts}
	sm = export_joined([o for o in objs if o not in optic_parts], 'SM_' + name, sockets={'Optic': socket} if socket else None)
	optic_sms = {}
	for optic_name, parts in optics.items():
		osm = export_joined(parts, 'SM_' + optic_name, pivot=socket)
		osm.location = socket
		col = bpy.data.collections.new('Optic_' + optic_name)
		bpy.context.scene.collection.children.link(col)
		bpy.context.scene.collection.objects.unlink(osm)
		col.objects.link(osm)
		optic_sms[optic_name] = (osm, col)
	# keep a parts collection in the .blend for later rigging
	parts = bpy.data.collections.new('Parts_' + name)
	bpy.context.scene.collection.children.link(parts)
	for o in objs:
		bpy.context.scene.collection.objects.unlink(o)
		parts.objects.link(o)
	parts.hide_render = True
	bpy.context.view_layer.layer_collection.children[parts.name].exclude = True

	def show_only(visible):
		for o in [sm] + [osm for osm, _ in optic_sms.values()]:
			o.hide_render = o not in visible

	show_only([sm])
	render(render_name)
	for optic_name, (osm, _) in optic_sms.items():
		show_only([sm, osm])
		render(f'{render_name}_{optic_name}')
		show_only([osm])
		c = Vector(socket) + Vector((0.0, 0.0, 0.035))
		render(optic_name, ortho=(c.x, c.z, 0.32), details={
			'front': (c + Vector((0.26, 0.24, 0.12)), c),
			'rear': (c + Vector((-0.30, -0.20, 0.12)), c),
		})
	# saved file: rifle plus the first optic visible; other optics toggle with their collection's eye icon
	show_only([sm] + [osm for osm, _ in optic_sms.values()])
	for i, (_, col) in enumerate(optic_sms.values()):
		bpy.context.view_layer.layer_collection.children[col.name].hide_viewport = i > 0
		col.hide_render = i > 0
	bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'src', name + '.blend'), compress=False)


def render(render_name, ortho=(0.12, 0.0, 0.98), details=None):
	"""Render orthographic left/right views plus perspective views ({tag: (camera location, look-at point)})."""
	sc = bpy.context.scene
	for eng in ('BLENDER_EEVEE', 'BLENDER_EEVEE_NEXT', 'BLENDER_WORKBENCH'):
		try:
			sc.render.engine = eng
			break
		except TypeError:
			continue
	sc.render.resolution_x, sc.render.resolution_y = 1600, 700
	sc.render.film_transparent = False
	if not sc.world:
		world = bpy.data.worlds.new('W')
		world.use_nodes = True
		world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.82, 0.83, 0.85, 1)
		world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.9
		sc.world = world
	sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
	sun.data.energy = 3.5
	sun.rotation_euler = (math.radians(50), math.radians(-20), math.radians(-30))
	sc.collection.objects.link(sun)
	cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
	sc.collection.objects.link(cam)
	sc.camera = cam
	tgt = bpy.data.objects.new('T', None)
	sc.collection.objects.link(tgt)
	cx, cz, scale = ortho
	cam.data.type = 'ORTHO'
	cam.data.ortho_scale = scale
	views = {
		'right': ((cx, -2.0, cz), (math.radians(90), 0, 0)),
		'left': ((cx, 2.0, cz), (math.radians(90), 0, math.radians(180))),
	}
	for tag, (loc, rot) in views.items():
		cam.location = loc
		cam.rotation_euler = rot
		sc.render.filepath = os.path.join(OUT, 'Previews', f'{render_name}_{tag}.png')
		bpy.ops.render.render(write_still=True)
	cam.data.type = 'PERSP'
	cam.data.lens = 50
	c = cam.constraints.new('TRACK_TO')
	c.target = tgt
	c.track_axis = 'TRACK_NEGATIVE_Z'
	c.up_axis = 'UP_Y'
	for tag, (loc, target) in (details or {'detail': ((0.35, -0.55, 0.22), (0.0, 0.0, 0.02))}).items():
		cam.location = loc
		tgt.location = target
		sc.render.filepath = os.path.join(OUT, 'Previews', f'{render_name}_{tag}.png')
		bpy.ops.render.render(write_still=True)
	for o in (sun, cam, tgt):
		bpy.data.objects.remove(o, do_unlink=True)


def ribbed(name, r, depth, x0, x1, mat, ribs=24, y=0.0, z=0.0):
	"""Cylinder along X with lengthwise grip ribs (focus rings, knurled caps)."""
	bm = bmesh.new()
	n = ribs * 4
	ends = []
	for x in (x0, x1):
		ring = []
		for i in range(n):
			a = 2 * math.pi * i / n
			rr = r if i % 4 < 2 else r - depth
			ring.append(bm.verts.new((x, y + rr * math.cos(a), z + rr * math.sin(a))))
		ends.append(ring)
	a, b = ends
	for i in range(n):
		j = (i + 1) % n
		bm.faces.new((a[i], a[j], b[j], b[i]))
	bm.faces.new(list(reversed(a)))
	bm.faces.new(b)
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	me = bpy.data.meshes.new(name)
	bm.to_mesh(me)
	bm.free()
	obj = bpy.data.objects.new(name, me)
	link(obj, mat)
	smooth_by_angle(obj, 30)
	return obj


# ------------------------------------------------------------- materials

def common_mats():
	return {
		'blued': make_mat('M_Steel_Blued', (0.025, 0.026, 0.03), 1.0, 0.32),
		'bright': make_mat('M_Steel_Bright', (0.62, 0.62, 0.64), 1.0, 0.28),
		'black': make_mat('M_Black_Matte', (0.012, 0.012, 0.013), 0.0, 0.85),
		'rubber': make_mat('M_Rubber_Black', (0.015, 0.015, 0.015), 0.0, 0.9),
		'bore': make_mat('M_Bore', (0.0, 0.0, 0.0), 0.0, 1.0),
	}


# ---------------------------------------------------------------- Rascal

def build_rascal(variant, color):
	reset()
	M = common_mats()
	stock = make_mat(f'M_Polymer_{variant}', color, 0.0, 0.48)
	zb = 0.052  # bore axis

	prof = [
		(-0.278, -0.118), (-0.272, 0.030), (-0.20, 0.036), (-0.12, 0.038), (-0.085, 0.034),
		(-0.055, 0.040), (0.0, 0.042), (0.20, 0.042), (0.295, 0.041), (0.31, 0.034), (0.312, 0.022),
		(0.305, 0.012), (0.25, 0.008), (0.12, 0.004), (0.04, 0.0), (0.0, -0.002),
		(-0.03, -0.004), (-0.038, -0.015), (-0.045, -0.05), (-0.05, -0.078), (-0.058, -0.088),
		(-0.075, -0.09), (-0.085, -0.082), (-0.11, -0.083), (-0.18, -0.098),
	]
	widths = [(-0.28, 0.036), (-0.12, 0.031), (-0.06, 0.026), (0.0, 0.029), (0.2, 0.029), (0.31, 0.025)]
	profile_solid('Stock', prof, [0, 1], widths, stock, 0.009)
	profile_solid('Buttpad', [(-0.2775, -0.118), (-0.2715, 0.030), (-0.284, 0.0315), (-0.2905, -0.1185)],
		[0, 1, 2, 3], [(-0.3, 0.036), (-0.27, 0.036)], M['rubber'], 0.004, 2, smooth_iters=0)
	tube('TriggerGuard', [(0.032, 0, 0.002), (0.031, 0, -0.022), (0.018, 0, -0.033), (-0.014, 0, -0.033),
		(-0.028, 0, -0.022), (-0.036, 0, -0.004)], 0.0032, stock, scale_y=2.4)
	tube('Trigger', [(0.003, 0, 0.004), (0.006, 0, -0.008), (0.003, 0, -0.019), (-0.005, 0, -0.025)],
		0.0017, M['bright'], scale_y=1.7)

	cyl('Receiver', 0.0135, 0.0135, -0.075, 0.14, M['blued'], z=zb)
	cyl('BreechRing', 0.0118, 0.0118, 0.135, 0.152, M['blued'], z=zb)
	cyl('Barrel', 0.0098, 0.0086, 0.15, 0.55, M['blued'], z=zb)
	cyl('Muzzle', 0.0028, 0.0028, 0.546, 0.5508, M['bore'], z=zb, seg=12)
	box('EjectionPort', 0.0, 0.042, -0.0142, -0.0118, zb - 0.004, zb + 0.007, M['black'], bev=0.0004)
	# front sight
	box('FrontSightBase', 0.524, 0.545, -0.0045, 0.0045, zb + 0.006, zb + 0.0125, M['blued'])
	box('FrontSightBlade', 0.532, 0.540, -0.0011, 0.0011, zb + 0.012, zb + 0.0195, M['blued'], bev=0.0003)
	# rear peep sight on receiver
	box('RearSightBase', -0.07, -0.045, -0.006, 0.006, zb + 0.011, zb + 0.017, M['blued'])
	box('RearSightPost', -0.068, -0.061, -0.003, 0.003, zb + 0.016, zb + 0.022, M['blued'], bev=0.0004)
	torus('RearSightAperture', 0.0042, 0.0016, (-0.0645, 0, zb + 0.026), M['blued'], axis='X')
	# bolt: cocking piece out the back, handle on right
	cyl('BoltBody', 0.0095, 0.0095, -0.098, -0.075, M['bright'], z=zb)
	cyl('CockingKnob', 0.0085, 0.010, -0.112, -0.098, M['blued'], z=zb)
	tube('BoltHandle', [(-0.03, -0.010, zb + 0.004), (-0.034, -0.018, zb + 0.003), (-0.04, -0.026, zb - 0.004),
		(-0.044, -0.030, zb - 0.012)], 0.0022, M['bright'], smooth=0)
	sphere('BoltKnob', 0.0052, (-0.045, -0.031, zb - 0.015), M['bright'])
	sling_stud('SlingFront', 0.26, 0.0075, M['blued'])
	sling_stud('SlingRear', -0.235, -0.108, M['blued'])
	finish(f'Rifle_Rascal_{variant}', f'Rascal_{variant}')


def build_scope(M, socket):
	"""3-9x40-style rimfire scope on two Picatinny ring mounts centered on the optic socket; returns the created objects."""
	before = set(bpy.context.scene.objects)
	rail_top = socket[2]
	body = make_mat('M_Scope_Anodized', (0.018, 0.018, 0.02), 0.35, 0.42)
	glass = make_mat('M_Scope_Lens', (0.015, 0.045, 0.06), 0.0, 0.04)
	zs = rail_top + 0.0242  # scope axis height
	tube_r = 0.0127  # 1 inch main tube
	prof = [(-0.110, 0.0), (-0.110, 0.015), (-0.115, 0.015), (-0.115, 0.0182), (-0.112, 0.0195), (-0.090, 0.0195)]
	# ribbed power ring
	x = -0.088
	while x < -0.064:
		prof += [(x, 0.0189), (x + 0.0012, 0.0198), (x + 0.0025, 0.0198), (x + 0.0037, 0.0189)]
		x += 0.004
	prof += [(-0.062, 0.0186), (-0.052, 0.0172), (-0.042, tube_r), (0.128, tube_r), (0.158, 0.0212),
		(0.170, 0.0225), (0.195, 0.0225), (0.195, 0.0192), (0.190, 0.0192), (0.190, 0.0)]
	lathe('ScopeBody', prof, body, z=zs, steps=48)
	for nm, xa, xb, r in (('ScopeLensRear', -0.1106, -0.1100, 0.0148), ('ScopeLensFront', 0.1900, 0.1906, 0.0190)):
		cyl(nm, r, r, xa, xb, glass, z=zs, seg=32)
	# turret saddle + elevation (top), windage (right), side focus (left)
	box('TurretSaddle', 0.017, 0.053, -0.0158, 0.0158, zs - 0.0115, zs + 0.0158, body, bev=0.0055)
	post('ElevationBase', 0.0092, zs, zs + 0.021, 'Z', 0.035, 0, 0, body)
	post('ElevationCap', 0.0112, zs + 0.017, zs + 0.0275, 'Z', 0.035, 0, 0, body, seg=24)
	post('WindageBase', 0.0092, 0, -0.021, 'Y', 0.035, 0, zs, body)
	post('WindageCap', 0.0112, -0.017, -0.0275, 'Y', 0.035, 0, zs, body, seg=24)
	post('FocusBase', 0.0085, 0, 0.020, 'Y', 0.035, 0, zs, body)
	post('FocusWheel', 0.0135, 0.0185, 0.025, 'Y', 0.035, 0, zs, body, seg=36)
	# rings: clamp on the rail, stand, band around the tube, cross bolt on the right
	for i, rx in enumerate((-0.025, 0.075)):
		lathe(f'Ring{i}', [(rx - 0.006, tube_r), (rx + 0.006, tube_r), (rx + 0.006, 0.0165), (rx - 0.006, 0.0165)],
			M['black'], z=zs, steps=40, closed=True)
		box(f'RingStand{i}', rx - 0.006, rx + 0.006, -0.0085, 0.0085, rail_top + 0.002, zs - 0.012, M['black'], bev=0.0015)
		box(f'RingClamp{i}', rx - 0.008, rx + 0.008, -0.0135, 0.0135, rail_top - 0.0075, rail_top + 0.003, M['black'], bev=0.0012)
		post(f'RingBolt{i}', 0.0032, -0.0135, -0.0175, 'Y', rx, 0, rail_top - 0.003, M['black'], seg=6)
	objs = [o for o in bpy.context.scene.objects if o not in before and o.type == 'MESH']
	# laid out centered on x = 0.025; slide it onto the socket
	for o in objs:
		o.location.x += socket[0] - 0.025
	return objs


def build_hydra(M, socket):
	"""Unbranded likeness of a DNT Hydra HS219 thermal scope on a QDM150-style QD mount; returns the created objects."""
	before = set(bpy.context.scene.objects)
	housing = make_mat('M_Thermal_Housing', (0.022, 0.023, 0.025), 0.2, 0.5)
	germanium = make_mat('M_Thermal_Lens', (0.035, 0.03, 0.045), 0.7, 0.08)
	sx, _, rail_top = socket
	x0 = sx + 0.0685  # objective face
	z0 = rail_top + 0.0412  # optical axis

	def X(v):
		return x0 + v

	def Z(v):
		return z0 + v

	# objective: hood with recessed germanium lens, ribbed focus ring
	lathe('ThermalHood', [(X(-0.0095), 0.0), (X(-0.0095), 0.0198), (X(-0.001), 0.0198), (X(0.0), 0.0191),
		(X(0.0), 0.0165), (X(-0.004), 0.0158), (X(-0.005), 0.0)], M['rubber'], z=z0)
	cyl('ThermalObjectiveLens', 0.0128, 0.0128, X(-0.0052), X(-0.0046), germanium, z=z0)
	ribbed('ThermalFocusRing', 0.0208, 0.0012, X(-0.0325), X(-0.0091), M['rubber'], ribs=30, z=z0)
	# main housing with sloped lower front
	hp = [(-0.0345, 0.0223), (-0.1137, 0.0223), (-0.1147, 0.020), (-0.1147, -0.024), (-0.1086, -0.026),
		(-0.0497, -0.026), (-0.0386, -0.0199), (-0.0335, -0.0057), (-0.0325, 0.0187)]
	profile_solid('ThermalHousing', [(X(a), Z(b)) for a, b in hp], [1, 2, 3],
		[(X(-0.3), 0.046), (X(0.1), 0.046)], housing, 0.005)
	# top deck: three rocker buttons and a round power button
	box('ThermalTopDeck', X(-0.112), X(-0.037), -0.016, 0.016, Z(0.020), Z(0.0245), housing, bev=0.0018)
	for i, bx in enumerate((-0.061, -0.080, -0.098)):
		box(f'ThermalButton{i}', X(bx - 0.0045), X(bx + 0.0045), -0.0075, 0.0075, Z(0.0235), Z(0.0272), M['rubber'], bev=0.0014)
	post('ThermalPowerButton', 0.0048, Z(0.0235), Z(0.0272), 'Z', X(-0.044), 0, 0, M['rubber'], seg=20)
	# battery tube with ribbed cap along the lower left side
	cyl('ThermalBatteryTube', 0.0128, 0.0128, X(-0.112), X(-0.042), housing, y=0.024, z=Z(-0.012))
	cyl('ThermalBatteryNose', 0.0128, 0.007, X(-0.042), X(-0.0345), housing, y=0.024, z=Z(-0.012))
	ribbed('ThermalBatteryCap', 0.0136, 0.0010, X(-0.1265), X(-0.112), M['rubber'], ribs=18, y=0.024, z=Z(-0.012))
	# right side: port door and control knob; left side: blank badge plate
	box('ThermalPortDoor', X(-0.098), X(-0.068), -0.0242, -0.0222, Z(-0.013), Z(0.013), M['rubber'], bev=0.0012)
	post('ThermalKnob', 0.0085, -0.022, -0.0305, 'Y', X(-0.052), 0, Z(0.002), housing, seg=28)
	box('ThermalBadge', X(-0.100), X(-0.082), 0.0222, 0.0238, Z(0.006), Z(0.017), M['black'], bev=0.0006)
	# eyepiece: neck, ribbed diopter ring, bellows eyecup around the display lens
	cyl('ThermalEyepieceNeck', 0.0173, 0.0173, X(-0.1285), X(-0.1140), housing, z=z0)
	ribbed('ThermalDiopterRing', 0.0203, 0.0011, X(-0.1475), X(-0.1279), M['rubber'], ribs=28, z=z0)
	cup = [(X(-0.1470), 0.0), (X(-0.1470), 0.0185)]
	x = -0.1495
	for k in range(5):
		lo, hi = 0.0188 + k * 0.0004, 0.0212 + k * 0.0006
		cup += [(X(x), lo), (X(x - 0.0035), hi), (X(x - 0.0055), hi), (X(x - 0.009), lo)]
		x -= 0.009
	cup += [(X(-0.1965), 0.0222), (X(-0.2000), 0.0255), (X(-0.2025), 0.0293), (X(-0.2032), 0.0293),
		(X(-0.2032), 0.0270), (X(-0.1995), 0.0238), (X(-0.1930), 0.0175), (X(-0.1720), 0.0150),
		(X(-0.1680), 0.0118), (X(-0.1680), 0.0)]
	lathe('ThermalEyecup', cup, M['rubber'], z=z0)
	cyl('ThermalEyepieceLens', 0.0116, 0.0116, X(-0.1684), X(-0.1679), germanium, z=z0)
	# QD mount: saddle, clamp block with three cross screws, rail jaws, throw lever on the left
	mx0, mx1 = sx - 0.0279, sx + 0.0279
	box('QDSaddle', mx0 + 0.003, mx1 - 0.003, -0.016, 0.016, Z(-0.0285), Z(-0.0245), M['black'], bev=0.0012)
	box('QDBlock', mx0, mx1, -0.0145, 0.0145, rail_top + 0.003, Z(-0.0275), M['black'], bev=0.0018)
	box('QDJaws', mx0 + 0.002, mx1 - 0.002, -0.0158, 0.0158, rail_top - 0.0075, rail_top + 0.0035, M['black'], bev=0.0012)
	for i, dx in enumerate((-0.0105, 0.0, 0.0105)):
		for side in (-1, 1):
			post(f'QDScrew{i}{side}', 0.0027, side * 0.0142, side * 0.0158, 'Y', sx + dx, 0, rail_top + 0.0085, M['blued'], seg=16)
	post('QDLeverPivot', 0.0038, 0.0145, 0.0195, 'Y', mx1 - 0.004, 0, Z(-0.0275), M['black'], seg=16)
	box('QDLever', mx1 - 0.004, X(-0.0101), 0.0150, 0.0188, Z(-0.0300), Z(-0.0255), M['black'], bev=0.0008)
	box('QDLeverTip', X(-0.0140), X(-0.0101), 0.0150, 0.0188, Z(-0.0340), Z(-0.0255), M['black'], bev=0.0008)
	return [o for o in bpy.context.scene.objects if o not in before and o.type == 'MESH']


# ---------------------------------------------------------------- Model 64 camo

def build_m64():
	reset()
	M = common_mats()
	camo = make_mat('M_Stock_Kryptek', (0.6, 0.55, 0.45), 0.0, 0.62, image=TEX)
	bronze = make_mat('M_Cerakote_Bronze', (0.30, 0.17, 0.08), 0.6, 0.4)
	zb = 0.056

	# sporter stock traced from a side photo of the real rifle
	prof = [
		(-0.3259, -0.1074), (-0.3362, 0.0118),
		# comb rising from the heel to a notched nose, then the wrist up to the receiver tang
		(-0.300, 0.0151), (-0.2487, 0.0192), (-0.1972, 0.0233), (-0.1458, 0.0274), (-0.1235, 0.0294), (-0.1160, 0.0294),
		(-0.1100, 0.0262), (-0.1040, 0.0225), (-0.0975, 0.0210), (-0.0890, 0.0218), (-0.0814, 0.0249), (-0.0685, 0.0310),
		(-0.0557, 0.0418), (-0.0479, 0.0485), (-0.0420, 0.0530), (-0.0340, 0.0553), (-0.0250, 0.0555),
		# long forend with a rounded tip; the barrel sits down in it
		(0.200, 0.0555), (0.365, 0.0555), (0.3844, 0.0510), (0.3973, 0.0440), (0.4035, 0.0367), (0.4040, 0.0300),
		(0.4005, 0.0238), (0.3922, 0.0197), (0.369, 0.0190), (0.3175, 0.0182), (0.266, 0.0171), (0.2403, 0.0161),
		(0.163, 0.0141), (0.1116, 0.0141), (0.086, 0.0145), (0.0391, 0.0120), (0.0288, 0.0104),
		# inside the trigger guard, then the swept front of the pistol grip down to the cap
		(0.0100, 0.0092), (-0.0100, 0.0082), (-0.0222, 0.0058), (-0.0325, 0.0032), (-0.0428, 0.0001),
		(-0.0557, -0.0071), (-0.0634, -0.0148), (-0.0685, -0.0199), (-0.0737, -0.0287), (-0.0789, -0.0380),
		(-0.0830, -0.0467), (-0.0862, -0.0512), (-0.0900, -0.0520), (-0.1195, -0.0420), (-0.1215, -0.0390),
		(-0.1260, -0.0330), (-0.1340, -0.0285),
		# straight butt line sloping down to the toe
		(-0.1458, -0.0333), (-0.1972, -0.0539), (-0.2487, -0.0755), (-0.3002, -0.0961), (-0.3200, -0.1042),
	]
	widths = [(-0.34, 0.040), (-0.13, 0.034), (-0.075, 0.029), (0.0, 0.034), (0.25, 0.036), (0.40, 0.032)]
	profile_solid('Stock', prof, [0, 1], widths, camo, 0.011)
	# butt pad pitched with the toe forward
	profile_solid('Buttpad', [(-0.3254, -0.1080), (-0.3357, 0.0122), (-0.3450, 0.0116), (-0.3347, -0.1086)],
		[0, 1, 2, 3], [(-0.35, 0.040), (-0.32, 0.040)], M['rubber'], 0.004, 2, smooth_iters=0)
	profile_solid('GripCap', [(-0.1210, -0.0420), (-0.0870, -0.0535), (-0.0856, -0.0492), (-0.1196, -0.0377)],
		[0, 1, 2, 3], [(-0.13, 0.026), (-0.08, 0.026)], M['black'], 0.0015, 2, smooth_iters=0)
	tube('TriggerGuard', [(0.0252, 0, 0.0125), (0.0257, 0, -0.0029), (0.0205, 0, -0.0150), (0.0061, 0, -0.0192),
		(-0.0105, 0, -0.0165), (-0.0181, 0, -0.0045), (-0.0225, 0, 0.0085)], 0.0021, camo, scale_y=2.4)
	tube('Trigger', [(0.0066, 0, 0.0115), (0.0020, 0, 0.0035), (-0.0010, 0, -0.0029), (0.0000, 0, -0.0091),
		(0.0056, 0, -0.0132)], 0.0017, M['bright'], scale_y=1.7)

	# receiver: rounded rear that sweeps down onto the wrist
	rz = 0.0675  # receiver top
	profile_solid('Receiver', [(-0.056, 0.038), (-0.054, 0.046), (-0.049, 0.0525), (-0.042, 0.0585), (-0.033, 0.0632),
		(-0.022, 0.0662), (-0.010, rz), (0.152, rz), (0.152, 0.038)], [0, 7, 8], [(-0.1, 0.027), (0.2, 0.027)], bronze, 0.0065)
	post('BoltRelease', 0.0032, -0.0130, -0.0152, 'Y', 0.0145, 0, 0.0582, M['black'], seg=20)
	# bolt handle slot, charging handle, and the bolt showing through the ejection port
	box('BoltSlot', 0.086, 0.112, -0.0141, -0.0125, 0.0548, 0.0574, M['black'], bev=0.0003)
	post('ChargingHandle', 0.0028, -0.012, -0.025, 'Y', 0.113, 0, zb, M['bright'], seg=16)
	sphere('ChargingKnob', 0.0048, (0.113, -0.0265, zb), M['bright'])
	box('EjectionPort', 0.124, 0.152, -0.0141, -0.0125, 0.0545, 0.0615, M['black'], bev=0.0004)
	box('BoltFace', 0.128, 0.149, -0.0144, -0.0139, 0.0555, 0.0603, M['bright'], bev=0.0003)
	# picatinny rail over the front of the receiver
	box('RailBase', 0.047, 0.195, -0.0105, 0.0105, rz - 0.0005, rz + 0.0072, M['black'], bev=0.0005)
	for i in range(14):
		x = 0.0534 + i * 0.01
		box(f'RailTooth{i:02d}', x, x + 0.0053, -0.0105, 0.0105, rz + 0.0068, rz + 0.011, M['black'], bev=0.0005)
	# fluted barrel + knurled thread protector
	barrel = cyl('Barrel', 0.0103, 0.0099, 0.15, 0.546, bronze, z=zb, seg=48)
	for k in range(6):
		a = k * math.pi / 3
		bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.0035, depth=0.25,
			location=(0.395, 0.0127 * math.cos(a), zb + 0.0127 * math.sin(a)), rotation=(0, math.pi / 2, 0))
		boolean_cut(barrel, bpy.context.active_object)
	smooth_by_angle(barrel, 50)
	ribbed('ThreadProtector', 0.0108, 0.0005, 0.545, 0.566, M['black'], ribs=28, z=zb)
	cyl('Muzzle', 0.003, 0.003, 0.5655, 0.5668, M['bore'], z=zb, seg=12)
	# magazine raked back at the bottom, ahead of the trigger guard under the ejection port
	profile_solid('Magazine', [(0.1153, 0.030), (0.1015, -0.0255), (0.1296, -0.0255), (0.1435, 0.030)],
		[0, 1, 2, 3], [(0.09, 0.017), (0.15, 0.017)], M['black'], 0.0015, 2, smooth_iters=0)
	profile_solid('MagBase', [(0.1000, -0.0245), (0.0988, -0.0295), (0.1318, -0.0295), (0.1310, -0.0245)],
		[0, 1, 2, 3], [(0.09, 0.0195), (0.14, 0.0195)], M['black'], 0.0012, 2, smooth_iters=0)
	box('MagRelease', 0.086, 0.097, -0.0045, 0.0045, 0.0095, 0.0165, M['black'], bev=0.001)
	post('ActionScrew', 0.0042, 0.0105, 0.016, 'Z', 0.166, 0, 0, M['blued'], seg=20)
	sling_stud('SlingFront', 0.357, 0.0215, M['blued'])
	sling_stud('SlingRear', -0.291, -0.0895, M['blued'])
	# optics are separate meshes mounted at the rifle's SOCKET_Optic (rail top, centered on the rail)
	socket = (0.121, 0.0, rz + 0.011)
	optics = {
		'Scope_3-9x40': build_scope(M, socket),
		'Scope_HS219_Hydra': build_hydra(M, socket),
	}
	finish('Rifle_M64_Kryptek', 'M64_Kryptek', optics=optics, socket=socket)


build_rascal('Blue', (0.02, 0.10, 0.52))
build_rascal('Red', (0.55, 0.018, 0.025))
build_m64()
print('ALL DONE')
