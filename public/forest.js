import * as THREE from 'three';
import { Sky } from 'three/addons/objects/Sky.js';
import { RULES, trees, acorns, den, ground, streamX, seeded } from '/shared/world.js';
const random=seeded(5791), dummy=new THREE.Object3D();
const color=new THREE.Color();
export async function makeForest(scene,renderer){
  const loader=new THREE.TextureLoader();
  const load=async(name,srgb=false)=>{const t=await loader.loadAsync('/assets/'+name);t.wrapS=t.wrapT=THREE.RepeatWrapping;t.anisotropy=Math.min(8,renderer.capabilities.getMaxAnisotropy());if(srgb)t.colorSpace=THREE.SRGBColorSpace;return t;};
  const [bark,barkNormal,earth,earthNormal]=await Promise.all([load('bark-color.jpg',true),load('bark-normal.jpg'),load('ground-color.jpg',true),load('ground-normal.jpg')]);
  bark.repeat.set(2,5);barkNormal.repeat.set(2,5);earth.repeat.set(36,36);earthNormal.repeat.set(36,36);
  scene.background=new THREE.Color('#b7c3b7');scene.fog=new THREE.FogExp2('#b8c4b3',.0075);
  const sky=new Sky();sky.scale.setScalar(450000);scene.add(sky);
  const su=sky.material.uniforms;su.turbidity.value=5;su.rayleigh.value=1.8;su.mieCoefficient.value=.006;su.mieDirectionalG.value=.83;
  const sunDirection=new THREE.Vector3(.65,.34,-.48).normalize();su.sunPosition.value.copy(sunDirection);
  const pmrem=new THREE.PMREMGenerator(renderer);scene.environment=pmrem.fromScene(sky,.08,.1,1000).texture;pmrem.dispose();scene.environmentIntensity=.45;
  const sun=new THREE.DirectionalLight('#ffe5b0',3.6);sun.position.copy(sunDirection).multiplyScalar(70);sun.castShadow=true;sun.shadow.mapSize.set(2048,2048);Object.assign(sun.shadow.camera,{left:-65,right:65,top:65,bottom:-65,near:.5,far:190});sun.shadow.bias=-.0003;sun.shadow.normalBias=.03;scene.add(sun);scene.add(sun.target);
  scene.add(new THREE.HemisphereLight('#b2ccd3','#4b432a',1.55));
  const terrainGeo=new THREE.PlaneGeometry(420,420,180,180);terrainGeo.rotateX(-Math.PI/2);
  const pos=terrainGeo.attributes.position,colors=[];
  for(let i=0;i<pos.count;i++){const x=pos.getX(i),z=pos.getZ(i);pos.setY(i,ground(x,z));const trail=Math.abs(x-(Math.sin(z*.022)*7-2));const meadow=Math.sin(x*.065)*Math.cos(z*.075);color.set(trail<2.5?'#c3b591':meadow>.2?'#8c9f6b':'#a4a388');colors.push(color.r,color.g,color.b);}
  terrainGeo.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));terrainGeo.computeVertexNormals();
  const terrain=new THREE.Mesh(terrainGeo,new THREE.MeshStandardMaterial({map:earth,normalMap:earthNormal,normalScale:new THREE.Vector2(.65,.65),roughness:1,vertexColors:true}));terrain.receiveShadow=true;scene.add(terrain);
  const barkMat=new THREE.MeshStandardMaterial({map:bark,normalMap:barkNormal,normalScale:new THREE.Vector2(.8,.8),color:'#bcb8a2',roughness:.97});
  const trunkGeo=new THREE.CylinderGeometry(.09,1,1,9,5);trunkGeo.translate(0,.5,0);
  const trunks=new THREE.InstancedMesh(trunkGeo,barkMat,trees.length);trunks.castShadow=true;trunks.receiveShadow=true;
  const branchGeo=new THREE.CylinderGeometry(.3,1,1,6);branchGeo.translate(0,.5,0);
  const branches=new THREE.InstancedMesh(branchGeo,barkMat,trees.length*6);branches.castShadow=true;branches.receiveShadow=true;
  const leafTexture=makeNeedles();const leafMat=new THREE.MeshStandardMaterial({map:leafTexture,alphaTest:.36,side:THREE.DoubleSide,color:'#ffffff',roughness:1});
  const foliage=new THREE.InstancedMesh(new THREE.PlaneGeometry(1,1),leafMat,trees.length*32);foliage.castShadow=true;foliage.receiveShadow=true;
  let bi=0,li=0;
  for(let ti=0;ti<trees.length;ti++){
    const t=trees[ti],base=ground(t.x,t.z);dummy.position.set(t.x,base,t.z);dummy.rotation.set(0,random()*6.28,0);dummy.scale.set(t.r,t.h,t.r);dummy.updateMatrix();trunks.setMatrixAt(ti,dummy.matrix);
    for(let b=0;b<6;b++){
      const angle=b*2.4+ti,at=t.h*(.34+b*.083),length=2.8+(5-b)*.36;
      const direction=new THREE.Vector3(Math.sin(angle),.17,Math.cos(angle)).normalize();
      dummy.position.set(t.x,base+at,t.z);dummy.quaternion.setFromUnitVectors(new THREE.Vector3(0,1,0),direction);dummy.scale.set(.14,length,.14);dummy.updateMatrix();branches.setMatrixAt(bi++,dummy.matrix);
    }
    for(let layer=0;layer<8;layer++)for(let p=0;p<4;p++){
      const angle=p*Math.PI/2+layer*.7+random()*.4,spread=(8-layer)*.48+.5;
      dummy.position.set(t.x+Math.sin(angle)*spread*.47,base+t.h*(.35+layer*.089),t.z+Math.cos(angle)*spread*.47);
      dummy.rotation.set(-.25+random()*.28,angle,random()*.2-.1);dummy.scale.set(spread*2.7,2.3+spread*.22,1);dummy.updateMatrix();foliage.setMatrixAt(li++,dummy.matrix);foliage.setColorAt(li-1,color.setHSL(.23+random()*.03,.12+random()*.1,.55+random()*.2));
    }
  }
  scene.add(trunks,branches,foliage);
  // Ferns and grass are instanced alpha-cut vegetation, rather than solid polygon cones.
  const grassTexture=makeGrass();const grassMat=new THREE.MeshStandardMaterial({map:grassTexture,alphaTest:.4,side:THREE.DoubleSide,color:'#ffffff',roughness:1});
  const grassGeo=new THREE.PlaneGeometry(1,1);grassGeo.translate(0,.5,0);
  const grass=new THREE.InstancedMesh(grassGeo,grassMat,7000);grass.receiveShadow=true;
  for(let i=0;i<7000;i++){
    const x=(random()-.5)*250,z=(random()-.5)*250,trail=Math.abs(x-(Math.sin(z*.022)*7-2));
    dummy.position.set(x,ground(x,z)-.04,z);dummy.rotation.set(0,random()*6.28,0);const height=trail<2.4?.12:.25+random()*.6;dummy.scale.set(.5+random()*.85,height,1);dummy.updateMatrix();grass.setMatrixAt(i,dummy.matrix);grass.setColorAt(i,color.setHSL(.19+random()*.09,.15+random()*.15,.5+random()*.2));
  }scene.add(grass);
  const fernMat=new THREE.MeshStandardMaterial({map:makeFern(),alphaTest:.35,side:THREE.DoubleSide,color:'#9dae7a',roughness:.95});
  const ferns=new THREE.InstancedMesh(grassGeo,fernMat,800);ferns.receiveShadow=true;
  for(let i=0;i<800;i++){const t=trees[i%trees.length],x=t.x+(random()-.5)*8,z=t.z+(random()-.5)*8;dummy.position.set(x,ground(x,z),z);dummy.rotation.set(0,random()*6.28,0);dummy.scale.set(1.5+random(),.8+random()*.5,1);dummy.updateMatrix();ferns.setMatrixAt(i,dummy.matrix);}scene.add(ferns);
  const rockGeo=new THREE.IcosahedronGeometry(1,1),rockMat=new THREE.MeshStandardMaterial({color:'#85897b',map:earth,roughness:1});
  const rocks=new THREE.InstancedMesh(rockGeo,rockMat,170);rocks.castShadow=true;rocks.receiveShadow=true;
  for(let i=0;i<170;i++){const z=(random()-.5)*220,x=i<85?streamX(z)+(random()-.5)*9:(random()-.5)*220;dummy.position.set(x,ground(x,z)-.2,z);dummy.rotation.set(random(),random()*6.28,random());const s=.3+random()*1.2;dummy.scale.set(s,s*.5,s*.7);dummy.updateMatrix();rocks.setMatrixAt(i,dummy.matrix);rocks.setColorAt(i,color.setHSL(.16,.06,.28+random()*.25));}scene.add(rocks);
  // Fallen logs add cover and visual landmarks.
  for(let i=0;i<14;i++){const x=(random()-.5)*170,z=(random()-.5)*170;const log=new THREE.Mesh(new THREE.CylinderGeometry(.27,.4,6+random()*4,10),barkMat);log.position.set(x,ground(x,z)+.35,z);log.rotation.set(Math.PI/2,0,random()*6.28);log.castShadow=true;log.receiveShadow=true;scene.add(log);}
  const waterPositions=[],waterUV=[],indices=[];
  for(let i=0;i<=110;i++){const z=-150+i*300/110,x=streamX(z);for(const side of[-1,1]){waterPositions.push(x+side*2.7,ground(x+side*2.7,z)+.07,z);waterUV.push(side===-1?0:1,i/8);}if(i<110){const k=i*2;indices.push(k,k+2,k+1,k+1,k+2,k+3);}}
  const waterGeo=new THREE.BufferGeometry();waterGeo.setAttribute('position',new THREE.Float32BufferAttribute(waterPositions,3));waterGeo.setAttribute('uv',new THREE.Float32BufferAttribute(waterUV,2));waterGeo.setIndex(indices);waterGeo.computeVertexNormals();
  const water=new THREE.Mesh(waterGeo,new THREE.MeshPhysicalMaterial({color:'#536f63',metalness:.35,roughness:.16,transparent:true,opacity:.75,normalMap:earthNormal,normalScale:new THREE.Vector2(.09,.09),clearcoat:1}));scene.add(water);
  // A mountain silhouette beyond the playable reserve.
  const mountains=new THREE.Group();
  for(let i=0;i<28;i++){const angle=i/28*Math.PI*2,radius=320;const m=new THREE.Mesh(new THREE.ConeGeometry(55+random()*65,45+random()*80,7),new THREE.MeshStandardMaterial({color:color.setHSL(.23,.08,.34+random()*.15).clone(),roughness:1}));m.position.set(Math.sin(angle)*radius,8,Math.cos(angle)*radius);mountains.add(m);}scene.add(mountains);
  const acornObjects=new Map(),acornGeo=new THREE.SphereGeometry(.15,8,6),acornMat=new THREE.MeshStandardMaterial({color:'#d4a761',roughness:.8,emissive:'#6b3e09',emissiveIntensity:.3});
  for(const a of acorns){const group=new THREE.Group();const nut=new THREE.Mesh(acornGeo,acornMat);nut.scale.y=1.35;group.add(nut);const cap=new THREE.Mesh(new THREE.SphereGeometry(.17,8,6,0,Math.PI*2,0,Math.PI/2),new THREE.MeshStandardMaterial({color:'#5c462c',roughness:1}));cap.position.y=.13;group.add(cap);group.position.set(a.x,ground(a.x,a.z)+.3,a.z);scene.add(group);acornObjects.set(a.id,group);}
  const denGroup=new THREE.Group();const burrow=new THREE.Mesh(new THREE.SphereGeometry(1.8,16,8),new THREE.MeshStandardMaterial({color:'#3d3524',map:earth,roughness:1}));burrow.scale.set(1,.42,.8);burrow.position.y=-.05;denGroup.add(burrow);const hole=new THREE.Mesh(new THREE.CircleGeometry(.55,24),new THREE.MeshBasicMaterial({color:'#0b1009'}));hole.position.set(0,.36,1.4);denGroup.add(hole);const ring=new THREE.Mesh(new THREE.RingGeometry(2.4,2.47,64),new THREE.MeshBasicMaterial({color:'#dce9a6',transparent:true,opacity:.55,side:THREE.DoubleSide}));ring.rotation.x=-Math.PI/2;ring.position.y=.05;denGroup.add(ring);denGroup.position.set(den.x,ground(den.x,den.z),den.z);scene.add(denGroup);
  const motesGeo=new THREE.BufferGeometry(),motes=[];for(let i=0;i<180;i++)motes.push((random()-.5)*100,2+random()*18,(random()-.5)*100);motesGeo.setAttribute('position',new THREE.Float32BufferAttribute(motes,3));const particles=new THREE.Points(motesGeo,new THREE.PointsMaterial({color:'#e5d7a9',size:.035,transparent:true,opacity:.6}));scene.add(particles);
  return {sun,grass,ferns,foliage,particles,water,acornObjects,denGroup,update(time,focus){particles.rotation.y=time*.005;water.material.normalMap.offset.x=time*.005;if(focus){sun.position.copy(focus).addScaledVector(sunDirection,70);sun.target.position.copy(focus);}},setQuality(q){renderer.shadowMap.enabled=q!=='low';grass.count=q==='low'?1800:q==='medium'?4000:7000;ferns.count=q==='low'?200:800;renderer.setPixelRatio(Math.min(devicePixelRatio,q==='high'?1.6:q==='medium'?1.2:1));}};
}
function canvasTexture(draw,size=512){const canvas=document.createElement('canvas');canvas.width=canvas.height=size;draw(canvas.getContext('2d'),size);const t=new THREE.CanvasTexture(canvas);t.colorSpace=THREE.SRGBColorSpace;return t;}
function makeNeedles(){return canvasTexture((ctx,s)=>{
  const rand=seeded(778);ctx.lineCap='round';
  const branch=(x,y,length,angle,depth)=>{
    const ex=x+Math.cos(angle)*length,ey=y+Math.sin(angle)*length;
    ctx.strokeStyle=depth>1?'#655b3c':'#777858';ctx.lineWidth=depth*1.4;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(ex,ey);ctx.stroke();
    for(let j=0;j<length/3;j++){const f=j/(length/3),px=x+(ex-x)*f,py=y+(ey-y)*f;for(const sign of[-1,1]){const a=angle+sign*(.6+rand()*.7),l=9+rand()*18;ctx.strokeStyle=['#738963','#889966','#566c49','#a2a37a','#6e804f'][Math.floor(rand()*5)];ctx.lineWidth=.8+rand()*.9;ctx.beginPath();ctx.moveTo(px,py);ctx.lineTo(px+Math.cos(a)*l,py+Math.sin(a)*l);ctx.stroke();}}
    if(depth>0)for(let j=1;j<7;j++){const f=j/7,px=x+(ex-x)*f,py=y+(ey-y)*f;for(const sign of[-1,1])branch(px,py,length*(.34+rand()*.12),angle+sign*(.5+rand()*.5),depth-1);}
  };
  branch(s*.5,s*.7,s*.43,-Math.PI/2,2);branch(s*.5,s*.7,s*.39,-2.55,2);branch(s*.5,s*.7,s*.39,-.59,2);
});}
function makeGrass(){return canvasTexture((ctx,s)=>{const rand=seeded(924);for(let i=0;i<75;i++){const x=s*.1+rand()*s*.8,h=s*(.2+rand()*.65),tip=x+(rand()-.5)*100;ctx.fillStyle=['#8e9c59','#839451','#bac28a','#697746','#a6aa73'][i%5];ctx.beginPath();ctx.moveTo(x,s);ctx.quadraticCurveTo(x+15,s-h*.45,tip,s-h);ctx.quadraticCurveTo(x+6,s-h*.4,x+3+rand()*5,s);ctx.fill();}for(let i=0;i<12;i++){const x=rand()*s,h=s*(.35+rand()*.4);ctx.strokeStyle='#a9a475';ctx.lineWidth=1.5;ctx.beginPath();ctx.moveTo(x,s);ctx.quadraticCurveTo(x-20,s-h*.4,x-30,s-h);ctx.stroke();for(let j=0;j<7;j++){ctx.fillStyle='#b5ad7b';ctx.beginPath();ctx.ellipse(x-30+j*1.5,s-h+j*4,3,5,-.4,0,Math.PI*2);ctx.fill();}}});}
function makeFern(){return canvasTexture((ctx,s)=>{for(let f=0;f<9;f++){const angle=-Math.PI/2+(f-4)*.22,length=s*(.52+Math.sin(f)*.1),sx=s/2,sy=s*.97,ex=sx+Math.cos(angle)*length,ey=sy+Math.sin(angle)*length;ctx.strokeStyle='#697c42';ctx.lineWidth=3;ctx.beginPath();ctx.moveTo(sx,sy);ctx.lineTo(ex,ey);ctx.stroke();for(let j=1;j<22;j++){const t=j/22,px=sx+(ex-sx)*t,py=sy+(ey-sy)*t,len=35*Math.sin(t*Math.PI);for(const side of[-1,1]){const a=angle+side*1.1;ctx.fillStyle=j%2?'#8a9c61':'#6b894d';ctx.beginPath();ctx.moveTo(px,py);ctx.quadraticCurveTo(px+Math.cos(a)*len*.6,py+Math.sin(a)*len*.7,px+Math.cos(a)*len,py+Math.sin(a)*len);ctx.quadraticCurveTo(px+Math.cos(a)*len*.5,py+Math.sin(a)*len*.4,px,py+4);ctx.fill();}}}});}
export function makeSquirrel(){
  const group=new THREE.Group(),fur=new THREE.MeshStandardMaterial({color:'#8a5c34',map:canvasTexture((ctx,s)=>{const rand=seeded(41);ctx.fillStyle='#b29a7b';ctx.fillRect(0,0,s,s);for(let i=0;i<16000;i++){const x=rand()*s,y=rand()*s;ctx.strokeStyle=rand()>.5?'#dbc8a5':'#64523d';ctx.globalAlpha=.2+rand()*.3;ctx.lineWidth=.5;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(x+(rand()-.5)*4,y+3+rand()*15);ctx.stroke();}}),roughness:1});
  const cream=new THREE.MeshStandardMaterial({color:'#c8b393',roughness:1}),black=new THREE.MeshStandardMaterial({color:'#0a0c08',roughness:.2});
  const sphere=(material,x,y,z,sx,sy,sz)=>{const m=new THREE.Mesh(new THREE.SphereGeometry(1,14,12),material);m.position.set(x,y,z);m.scale.set(sx,sy,sz);m.castShadow=true;group.add(m);return m;};
  sphere(fur,0,.28,0,.23,.24,.38);sphere(cream,0,.21,-.04,.18,.16,.31);sphere(fur,0,.46,-.33,.19,.17,.2);sphere(cream,0,.4,-.49,.12,.08,.1);sphere(black,0,.43,-.57,.047,.035,.027);
  for(const side of[-1,1]){sphere(fur,side*.12,.64,-.29,.075,.135,.063);sphere(cream,side*.12,.65,-.335,.045,.085,.023);sphere(black,side*.155,.49,-.432,.035,.034,.024);sphere(fur,side*.19,.12,.17,.14,.13,.18);sphere(fur,side*.14,.12,-.24,.06,.13,.065);sphere(fur,side*.17,.025,-.27,.055,.03,.1);sphere(fur,side*.23,.035,.24,.09,.03,.16);}
  const curve=new THREE.CatmullRomCurve3([new THREE.Vector3(0,.26,.28),new THREE.Vector3(0,.48,.57),new THREE.Vector3(0,.85,.66),new THREE.Vector3(0,1.1,.5),new THREE.Vector3(0,1.06,.22)]);
  const tail=new THREE.Mesh(new THREE.TubeGeometry(curve,22,.19,10,false),fur);tail.castShadow=true;group.add(tail);
  // Fine fur around the tail silhouette.
  const hairs=[],rand=seeded(74);for(let i=0;i<480;i++){const t=rand(),p=curve.getPoint(t),angle=rand()*Math.PI*2,r=.17+rand()*.07;hairs.push(p.x+Math.cos(angle)*r,p.y,p.z+Math.sin(angle)*r,p.x+Math.cos(angle)*(r+.03),p.y+.045,p.z+Math.sin(angle)*(r+.03));}const hg=new THREE.BufferGeometry();hg.setAttribute('position',new THREE.Float32BufferAttribute(hairs,3));group.add(new THREE.LineSegments(hg,new THREE.LineBasicMaterial({color:'#a48052',transparent:true,opacity:.4})));
  group.userData.tail=tail;return group;
}
export function makeHunter(){
  const group=new THREE.Group();const jacket=new THREE.MeshStandardMaterial({color:'#596044',roughness:1}),pants=new THREE.MeshStandardMaterial({color:'#434a36',roughness:1}),skin=new THREE.MeshStandardMaterial({color:'#b29472',roughness:.9});
  function mesh(g,m,x,y,z){const o=new THREE.Mesh(g,m);o.position.set(x,y,z);o.castShadow=true;group.add(o);return o;}
  mesh(new THREE.CylinderGeometry(.25,.21,.65,10),jacket,0,1.15,0);mesh(new THREE.SphereGeometry(.16,12,10),skin,0,1.72,0);mesh(new THREE.CylinderGeometry(.19,.19,.1,12),pants,0,1.84,0);mesh(new THREE.BoxGeometry(.26,.025,.25),pants,0,1.8,-.15);
  for(const side of[-1,1]){mesh(new THREE.CylinderGeometry(.105,.09,.72,8),pants,side*.12,.47,0);mesh(new THREE.BoxGeometry(.16,.15,.29),pants,side*.12,.08,-.06);const arm=mesh(new THREE.CylinderGeometry(.085,.07,.62,8),jacket,side*.3,1.2,-.1);arm.rotation.x=-.6;}
  const rifle=makeRifle();rifle.scale.setScalar(.85);rifle.position.set(.13,1.15,-.3);group.add(rifle);return group;
}
export function makeRifle(){
  const group=new THREE.Group(),metal=new THREE.MeshStandardMaterial({color:'#262d29',metalness:.75,roughness:.35}),wood=new THREE.MeshStandardMaterial({color:'#635440',roughness:.75});
  const box=(m,x,y,z,sx,sy,sz)=>{const b=new THREE.Mesh(new THREE.BoxGeometry(sx,sy,sz),m);b.position.set(x,y,z);group.add(b);};
  box(wood,0,0,.18,.075,.12,.35);box(metal,0,.01,-.1,.07,.095,.32);box(wood,0,-.09,.13,.055,.2,.1);
  const barrel=new THREE.Mesh(new THREE.CylinderGeometry(.018,.022,.55,12),metal);barrel.rotation.x=Math.PI/2;barrel.position.set(0,.033,-.5);group.add(barrel);
  const scope=new THREE.Mesh(new THREE.CylinderGeometry(.036,.043,.25,16),metal);scope.rotation.x=Math.PI/2;scope.position.set(0,.14,-.12);group.add(scope);box(metal,0,.08,-.11,.03,.07,.1);
  const lens=new THREE.Mesh(new THREE.CircleGeometry(.032,16),new THREE.MeshStandardMaterial({color:'#546e67',metalness:.9,roughness:.05}));lens.position.set(0,.14,.007);lens.rotation.y=Math.PI;group.add(lens);return group;
}
