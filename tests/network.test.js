import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { WebSocket } from 'ws';
let processHandle;
const port=3198;
before(async()=>{processHandle=spawn(process.execPath,['server.js'],{env:{...process.env,PORT:String(port)},stdio:['ignore','pipe','pipe']});await new Promise((resolve,reject)=>{const timeout=setTimeout(()=>reject(Error('Test server did not start')),10000);processHandle.stdout.on('data',()=>{clearTimeout(timeout);resolve();});processHandle.on('error',reject);});});
after(()=>processHandle?.kill());
function client(){return new Promise((resolve,reject)=>{const ws=new WebSocket(`ws://localhost:${port}`);ws.on('open',()=>resolve(ws));ws.on('error',reject);});}
function message(ws,predicate){return new Promise((resolve,reject)=>{const timeout=setTimeout(()=>{ws.off('message',onMessage);reject(Error('Expected server message not received'));},5000);function onMessage(raw){const m=JSON.parse(raw);if(predicate(m)){clearTimeout(timeout);ws.off('message',onMessage);resolve(m);}}ws.on('message',onMessage);});}
test('opposing human roles join the same authoritative room and share positions and thermal state',async()=>{
  const h=await client(),s=await client();try{
    const joinedH=message(h,m=>m.type==='joined');h.send(JSON.stringify({type:'join',role:'hunter',name:'Host'}));const host=await joinedH;
    const joinedS=message(s,m=>m.type==='joined');s.send(JSON.stringify({type:'join',role:'squirrel',name:'Friend',code:host.code}));const friend=await joinedS;assert.equal(host.code,friend.code);
    const state=await message(h,m=>m.type==='state'&&m.players.filter(p=>!p.bot).length===2);assert.equal(state.players.find(p=>p.id===friend.id).role,'squirrel');assert.equal(state.players.find(p=>p.id===host.id).role,'hunter');
    const thermal=message(s,m=>m.type==='state'&&m.players.find(p=>p.id===host.id).thermalUntil>0);h.send(JSON.stringify({type:'input',input:{thermal:true}}));const visible=await thermal,p=visible.players.find(p=>p.id===host.id);assert.ok(Math.abs(p.thermalReadyAt-p.thermalUntil-170)<.0001);
    const start=state.players.find(p=>p.id===friend.id).z;const moved=message(h,m=>m.type==='state'&&m.players.find(p=>p.id===friend.id).z<start-.8);s.send(JSON.stringify({type:'input',input:{forward:true,yaw:0}}));await moved;
  }finally{h.close();s.close();}
});
test('joining a nonexistent room returns a clear error',async()=>{const ws=await client();try{const error=message(ws,m=>m.type==='error');ws.send(JSON.stringify({type:'join',role:'hunter',code:'BAD123'}));assert.match((await error).message,/not found/);}finally{ws.close();}});
test('clients cannot submit arbitrary player coordinates',async()=>{const ws=await client();try{const joined=message(ws,m=>m.type==='joined');ws.send(JSON.stringify({type:'join',role:'hunter',name:'Tester'}));const {id}=await joined;const snapshot=message(ws,m=>m.type==='state');ws.send(JSON.stringify({type:'input',input:{x:9999,y:9999,z:9999}}));const state=await snapshot,p=state.players.find(p=>p.id===id);assert.ok(Math.abs(p.x)<10);assert.ok(Math.abs(p.z)<20);}finally{ws.close();}});
