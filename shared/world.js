export const RULES = Object.freeze({ radius: 110, round: 360, thermalDuration: 10, thermalCooldown: 180, capturesToWin: 8, acornsToWin: 12, maxPlayers: 8 });
export function seeded(seed) { return () => { seed |= 0; seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }
export function ground(x, z) { return Math.sin(x * .034) * 2.8 + Math.cos(z * .041) * 2.1 + Math.sin((x + z) * .066) * .8; }
export const streamX = z => 31 + Math.sin(z * .027) * 12;
const rand = seeded(84672);
export const trees = [];
// A few accessible trees beside the spawn clearings, then the same seeded forest on every client.
for (const [x,z] of [[-7,3],[7,-9],[-12,-15],[13,8],[-17,9],[2,-22]]) trees.push({id:trees.length,x,z,r:.55,h:17+rand()*8});
for (let i=0;i<245;i++) {
  const x=(rand()-.5)*220,z=(rand()-.5)*220;
  if (Math.hypot(x,z)>108 || Math.hypot(x,z)<16 || Math.abs(x-streamX(z))<5 || trees.some(t=>Math.hypot(x-t.x,z-t.z)<5)) continue;
  trees.push({id:trees.length,x,z,r:.28+rand()*.35,h:15+rand()*15});
}
export const acorns = Array.from({length:28},(_,i)=>{
  const t=trees[(i*7+2)%trees.length];
  const angle=rand()*Math.PI*2;
  return {id:i,x:t.x+Math.sin(angle)*2.1,z:t.z+Math.cos(angle)*2.1};
});
export const den={x:0,z:12};
export function clamp(n,min,max) {return Math.max(min,Math.min(max,n));}
