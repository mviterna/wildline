import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createPlayer, activateThermal, stepPlayer, shoot } from '../shared/simulation.js';
import { trees, ground, acorns, den, RULES } from '../shared/world.js';
const roomFor=(...players)=>({players:new Map(players.map(p=>[p.id,p])),events:[],captures:0,acorns:0,winner:null});
test('thermal lasts exactly ten seconds and cannot be activated again for three minutes',()=>{
  const p=createPlayer('h','hunter','Hunter');assert.equal(activateThermal(p,0),true);assert.equal(p.thermalUntil,10);assert.equal(p.thermalReadyAt,180);assert.equal(activateThermal(p,10),false);assert.equal(activateThermal(p,179.99),false);assert.equal(activateThermal(p,180),true);assert.equal(p.thermalUntil,190);assert.equal(activateThermal(createPlayer('s','squirrel','Squirrel'),180),false);
});
test('squirrel can grab a trunk, climb and launch over fifteen metres',()=>{
  const p=createPlayer('s','squirrel','Squirrel'),t=trees[0],room=roomFor(p);p.x=t.x+t.r+.5;p.z=t.z;p.y=ground(p.x,p.z);p.input={climb:true};stepPlayer(p,room,1/30,1);assert.equal(p.tree,t.id);
  p.input={forward:true};for(let i=0;i<70;i++)stepPlayer(p,room,1/30,1+i/30);assert.ok(p.y-ground(t.x,t.z)>9);
  const start=p.z;p.input={jump:true,yaw:0};stepPlayer(p,room,1/30,4);assert.equal(p.tree,null);assert.equal(p.vz,-17);p.input={yaw:0};for(let i=0;i<36;i++)stepPlayer(p,room,1/30,4+i/30);assert.ok(start-p.z>19);
});
test('hunters cannot climb trees',()=>{
  const p=createPlayer('h','hunter','Hunter'),t=trees[0];p.x=t.x+t.r+.5;p.z=t.z;p.y=ground(p.x,p.z);p.input={climb:true};stepPlayer(p,roomFor(p),1/30,1);assert.equal(p.tree,null);
});
test('aimed shots tag squirrels, consume ammo, share score, and respect fire rate',()=>{
  const p=createPlayer('h','hunter','Hunter'),s=createPlayer('s','squirrel','Squirrel');p.x=0;p.z=0;p.y=ground(0,0);s.x=0;s.z=-5;s.y=ground(0,-5);p.pitch=Math.atan2(s.y+.28-p.y-1.65,5);const room=roomFor(p,s);assert.equal(shoot(p,room,1),s);assert.equal(p.ammo,4);assert.equal(s.respawnAt,6);assert.equal(room.captures,1);assert.equal(shoot(p,room,1.2),null);assert.equal(p.ammo,4);
});
test('a solid tree trunk blocks the rifle ray',()=>{
  const p=createPlayer('h','hunter','Hunter'),s=createPlayer('s','squirrel','Squirrel'),t=trees[0];p.x=t.x;p.z=t.z+3;p.y=ground(t.x,t.z);s.x=t.x;s.z=t.z-3;s.y=ground(t.x,t.z);p.pitch=Math.atan2(.28-1.65,6);const room=roomFor(p,s);assert.equal(shoot(p,room,1),null);assert.equal(room.captures,0);
});
test('foraging is per squirrel and depositing increments the shared team objective',()=>{
  const p=createPlayer('s','squirrel','Squirrel'),a=acorns[0],room=roomFor(p);p.x=a.x;p.z=a.z;p.y=ground(a.x,a.z);stepPlayer(p,room,1/30,1);assert.ok(p.bag>=1);assert.ok(p.collected.includes(a.id));const bag=p.bag;stepPlayer(p,room,1/30,2);assert.equal(p.bag,bag);p.x=den.x;p.z=den.z;p.y=ground(den.x,den.z);stepPlayer(p,room,1/30,3);assert.equal(p.bag,0);assert.equal(room.acorns,bag);
});
test('the twelfth stored acorn ends the round for squirrels',()=>{
  const p=createPlayer('s','squirrel','Squirrel'),room=roomFor(p);room.acorns=11;p.bag=1;p.x=den.x;p.z=den.z;p.y=ground(den.x,den.z);stepPlayer(p,room,1/30,1);assert.equal(room.winner,'squirrels');
});
test('reload takes 2.2 seconds and restores a five-round magazine',()=>{
  const p=createPlayer('h','hunter','Hunter'),room=roomFor(p);p.ammo=0;p.input={reload:true};stepPlayer(p,room,1/30,1);assert.equal(p.reloadUntil,3.2);p.input={};stepPlayer(p,room,1/30,3);assert.equal(p.ammo,0);stepPlayer(p,room,1/30,3.3);assert.equal(p.ammo,5);assert.equal(p.reloadUntil,0);
});
test('player movement is clamped to the reserve boundary',()=>{
  const p=createPlayer('s','squirrel','Squirrel'),room=roomFor(p);p.x=109;p.z=0;p.y=ground(p.x,p.z);p.input={forward:true,sprint:true,yaw:-Math.PI/2};for(let i=0;i<100;i++)stepPlayer(p,room,1/30,i/30);assert.ok(Math.hypot(p.x,p.z)<=RULES.radius+.00001);
});
