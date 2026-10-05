import bpy,math,json
from pathlib import Path
root=Path(r'C:/Users/mvite/OneDrive/Documents/ChatGPT/PhotoRealistic Multiplater/native/Unreal/Wildline/ArtSource/Forest/Export')
profiles={}
for name in ['SM_OakA','SM_OakB','SM_OakC','SM_Pine']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(root/(name+'.fbx')))
 o=next(o for o in bpy.context.selected_objects if o.type=='MESH');pts=[o.matrix_world@v.co for v in o.data.vertices]
 polys=[p for p in o.data.polygons if 'Bark' in o.data.materials[p.material_index].name]
 rows=[];previous=(0,0)
 for z in [0.2,1,2,3,4,5,6,8,10,12,14,16,18]:
  segments=[]
  for p in polys:
   pp=[pts[i] for i in p.vertices];cross=[]
   for a,b in zip(pp,pp[1:]+pp[:1]):
    if (a.z<z<=b.z) or (b.z<z<=a.z):
     t=(z-a.z)/(b.z-a.z);c=a.lerp(b,t);cross.append((round(c.x,4),round(c.y,4)))
   if len(cross)==2:segments.append(cross)
  graph={}
  for a,b in segments:graph.setdefault(a,set()).add(b);graph.setdefault(b,set()).add(a)
  groups=[];unseen=set(graph)
  while unseen:
   start=unseen.pop();group={start};todo=[start]
   while todo:
    for p in graph[todo.pop()]:
     if p in unseen:unseen.remove(p);group.add(p);todo.append(p)
   if len(group)>=5:groups.append(group)
  if not groups:continue
  candidates=[]
  for g in groups:
   x=sum(p[0] for p in g)/len(g);y=sum(p[1] for p in g)/len(g);r=max(math.hypot(p[0]-x,p[1]-y) for p in g)
   candidates.append((math.hypot(x-previous[0],y-previous[1]),x,y,r,len(g)))
  near=[c for c in candidates if c[0]<1.5]; _,x,y,r,n=max(near or candidates,key=lambda c:c[3]);previous=(x,y);rows.append([z,x,y,r]);print('TRUNK',name,z,round(x,2),round(y,2),round(r,2),n)
 profiles[name]=rows
Path(root.parent/'trunk-profiles.json').write_text(json.dumps(profiles,indent=2))
