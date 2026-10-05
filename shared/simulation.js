import { RULES, trees, acorns, den, ground, clamp } from './world.js';
export function createPlayer(id,role,name,bot=false,index=0) {
  const x=role==='hunter' ? 0+index*2 : -7+index*3, z=role==='hunter'?10:-4-index*3;
  return {id,role,name,bot,x,y:ground(x,z),z,yaw:0,pitch:0,vy:0,vx:0,vz:0,tree:null,stamina:100,ammo:5,reloadUntil:0,fireAt:-10,thermalUntil:0,thermalReadyAt:0,respawnAt:0,bag:0,captures:0,collected:[],input:{},lastAction:{},moving:0};
}
export function activateThermal(p,now) {
  if(p.role!=='hunter'||now<p.thermalReadyAt||now<p.respawnAt) return false;
  p.thermalUntil=now+RULES.thermalDuration; p.thermalReadyAt=now+RULES.thermalCooldown; return true;
}
export function intersectsTrunk(origin,dir,dist) {
  return trees.some(t=>{
    const dx=t.x-origin.x,dz=t.z-origin.z,flat=dir.x*dir.x+dir.z*dir.z;
    if(flat<.00001) return false;
    const along=(dx*dir.x+dz*dir.z)/flat;
    if(along<.1||along>dist) return false;
    const y=origin.y+dir.y*along;
    return Math.hypot(origin.x+dir.x*along-t.x,origin.z+dir.z*along-t.z)<t.r && y>ground(t.x,t.z) && y<ground(t.x,t.z)+t.h;
  });
}
export function shoot(p,room,now) {
  if(p.role!=='hunter'||p.ammo===0||now<p.reloadUntil||now-p.fireAt<.75||now<p.respawnAt||room.winner) return null;
  p.fireAt=now; p.ammo--;
  const c=Math.cos(p.pitch),dir={x:-Math.sin(p.yaw)*c,y:Math.sin(p.pitch),z:-Math.cos(p.yaw)*c};
  const origin={x:p.x,y:p.y+1.65,z:p.z}; let target=null, distance=100;
  for(const animal of room.players.values()) {
    if(animal.role!=='squirrel'||now<animal.respawnAt) continue;
    const dx=animal.x-origin.x,dy=animal.y+.28-origin.y,dz=animal.z-origin.z;
    const along=dx*dir.x+dy*dir.y+dz*dir.z;
    if(along<0||along>distance) continue;
    const perpendicular=Math.hypot(dx-dir.x*along,dy-dir.y*along,dz-dir.z*along);
    if(perpendicular<.48&&!intersectsTrunk(origin,dir,along)) {target=animal;distance=along;}
  }
  room.events.push({type:'shot',id:p.id,hit:target?.id||null});
  if(target) {target.respawnAt=now+5;target.bag=0;target.tree=null;room.captures++;p.captures++;if(room.captures>=RULES.capturesToWin)room.winner='hunters';}
  return target;
}
export function stepPlayer(p,room,dt,now) {
  if(now<p.respawnAt||room.winner) return;
  if(p.respawnAt) {p.x=-7;p.z=-4;p.y=ground(p.x,p.z);p.vy=0;p.respawnAt=0;}
  const i=p.input,edge=k=>i[k]&&!p.lastAction[k];
  if(Number.isFinite(i.yaw))p.yaw=i.yaw;
  if(Number.isFinite(i.pitch))p.pitch=clamp(i.pitch,-1.4,1.4);
  if(edge('thermal')) activateThermal(p,now);
  if(edge('reload')&&p.role==='hunter'&&p.ammo<5&&!p.reloadUntil) p.reloadUntil=now+2.2;
  if(p.reloadUntil&&now>=p.reloadUntil){p.ammo=5;p.reloadUntil=0;}
  if(i.fire) shoot(p,room,now);
  const forward=(i.forward?1:0)-(i.back?1:0),side=(i.right?1:0)-(i.left?1:0);
  const mag=Math.max(1,Math.hypot(forward,side)),fx=-Math.sin(p.yaw),fz=-Math.cos(p.yaw);
  const dx=(fx*forward+Math.cos(p.yaw)*side)/mag,dz=(fz*forward-Math.sin(p.yaw)*side)/mag;
  const floor=ground(p.x,p.z),onGround=p.y<=floor+.03;
  if(p.role==='squirrel'&&edge('climb')){
    if(p.tree!==null){p.tree=null;p.vy=0;}
    else{const t=trees.find(t=>Math.hypot(t.x-p.x,t.z-p.z)<t.r+1.6&&p.y<ground(t.x,t.z)+t.h*.75);if(t){p.tree=t.id;p.y=Math.max(p.y,ground(t.x,t.z)+.5);}}
  }
  if(p.tree!==null) {
    const t=trees[p.tree],angle=Math.atan2(p.x-t.x,p.z-t.z)+side*dt*1.6;
    p.x=t.x+Math.sin(angle)*(t.r+.32);p.z=t.z+Math.cos(angle)*(t.r+.32);
    p.y=clamp(p.y+forward*4.8*dt,ground(t.x,t.z)+.35,ground(t.x,t.z)+t.h*.72);p.vy=0;
    if(edge('jump')&&p.stamina>15){p.tree=null;p.vy=7.5;p.vx=fx*17;p.vz=fz*17;p.stamina-=18;}
  }else{
    const sprint=i.sprint&&p.stamina>4&&Math.hypot(dx,dz)>.01;
    const speed=p.role==='squirrel'?(sprint?10.5:6.5):(i.crouch?1.6:sprint?5.8:3.2);
    p.stamina=clamp(p.stamina+(sprint?-12:9)*dt,0,100);
    if(onGround){p.vx=dx*speed;p.vz=dz*speed;p.vy=Math.max(0,p.vy);if(edge('jump')&&p.stamina>12){p.vy=p.role==='squirrel'?7.2:4.2;const leapForward=p.role==='squirrel'&&Math.hypot(dx,dz)<.01;p.vx=(leapForward?fx:dx)*(p.role==='squirrel'?13:speed);p.vz=(leapForward?fz:dz)*(p.role==='squirrel'?13:speed);p.stamina-=12;}}
    else {p.vx+=dx*dt*2.5;p.vz+=dz*dt*2.5;}
    p.x+=p.vx*dt;p.z+=p.vz*dt;p.vy-=17*dt;p.y+=p.vy*dt;
    const g=ground(p.x,p.z);if(p.y<g){p.y=g;p.vy=0;}
    for(const t of trees){const len=Math.hypot(p.x-t.x,p.z-t.z),r=t.r+(p.role==='hunter'?.38:.2);if(len<r&&p.y<ground(t.x,t.z)+t.h){p.x=t.x+(p.x-t.x)/Math.max(.001,len)*r;p.z=t.z+(p.z-t.z)/Math.max(.001,len)*r;}}
  }
  const d=Math.hypot(p.x,p.z);if(d>RULES.radius){p.x*=RULES.radius/d;p.z*=RULES.radius/d;}
  p.moving=Math.hypot(p.vx,p.vz)+(p.tree!==null?Math.abs(forward)*3:0);
  if(p.role==='squirrel') {
    for(const a of acorns){if(p.bag<4&&!p.collected.includes(a.id)&&Math.hypot(p.x-a.x,p.z-a.z)<1.7&&p.y-ground(a.x,a.z)<1){p.bag++;p.collected.push(a.id);room.events.push({type:'acorn',id:p.id});}}
    if(p.bag&&Math.hypot(p.x-den.x,p.z-den.z)<3){room.acorns+=p.bag;p.bag=0;room.events.push({type:'deposit',id:p.id});if(room.acorns>=RULES.acornsToWin)room.winner='squirrels';}
  }
  p.lastAction={...i};
}
export function botInput(p,room,now) {
  const foes=[...room.players.values()].filter(x=>x.role!==p.role&&now>=x.respawnAt);
  const nearest=foes.sort((a,b)=>Math.hypot(p.x-a.x,p.z-a.z)-Math.hypot(p.x-b.x,p.z-b.z))[0];
  let x,z,climb=false,jump=false,fire=false,pitch=0;
  if(p.role==='squirrel'){
    const danger=nearest&&Math.hypot(p.x-nearest.x,p.z-nearest.z)<19;
    if(p.tree!==null){x=p.x+(p.x-(nearest?.x??0))*3;z=p.z+(p.z-(nearest?.z??10))*3;jump=p.y>ground(p.x,p.z)+8;}
    else if(danger){const t=trees.filter(t=>Math.hypot(t.x-p.x,t.z-p.z)<20).sort((a,b)=>Math.hypot(a.x-p.x,a.z-p.z)-Math.hypot(b.x-p.x,b.z-p.z))[0];x=t?.x??p.x+(p.x-nearest.x);z=t?.z??p.z+(p.z-nearest.z);climb=!!t&&Math.hypot(p.x-t.x,p.z-t.z)<t.r+1.5;}
    else if(p.bag>=3){x=den.x;z=den.z;}
    else {const a=acorns.filter(a=>!p.collected.includes(a.id)).sort((a,b)=>Math.hypot(p.x-a.x,p.z-a.z)-Math.hypot(p.x-b.x,p.z-b.z))[0];x=a?.x??den.x;z=a?.z??den.z;}
    p.input={forward:true,sprint:!!danger,yaw:Math.atan2(p.x-x,p.z-z),climb,jump};
  }else if(nearest){x=nearest.x;z=nearest.z;const distance=Math.hypot(x-p.x,z-p.z);pitch=Math.atan2(nearest.y+.28-p.y-1.65,distance);fire=distance<15&&now-p.fireAt>5;p.input={forward:distance>9,yaw:Math.atan2(p.x-x,p.z-z),pitch,fire,reload:p.ammo===0};}
}
