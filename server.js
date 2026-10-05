import http from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { networkInterfaces } from 'node:os';
import { randomBytes } from 'node:crypto';
import { WebSocketServer } from 'ws';
import { RULES } from './shared/world.js';
import { createPlayer, stepPlayer, botInput } from './shared/simulation.js';
const root=path.dirname(fileURLToPath(import.meta.url));
const types={'.html':'text/html','.css':'text/css','.js':'text/javascript','.jpg':'image/jpeg','.png':'image/png','.svg':'image/svg+xml','.ico':'image/x-icon','.ttf':'font/ttf'};
const server=http.createServer(async(req,res)=>{
  try{
    const url=new URL(req.url,'http://localhost'),pathname=decodeURIComponent(url.pathname);
    if(pathname==='/health'){res.writeHead(200,{'Content-Type':'application/json'});res.end(JSON.stringify({ok:true,rooms:rooms.size}));return;}
    const base=pathname.startsWith('/shared/')?root:pathname.startsWith('/vendor/')?path.join(root,'node_modules/three'):path.join(root,'public');
    const relative=pathname.startsWith('/vendor/')?pathname.slice(8):pathname==='/'?'index.html':pathname.slice(1);
    const target=path.resolve(base,relative);
    if(!target.startsWith(base+path.sep)){res.writeHead(403);res.end();return;}
    if(!(await stat(target)).isFile())throw Error('Not found');
    res.writeHead(200,{'Content-Type':types[path.extname(target)]||'application/octet-stream','Cache-Control':'no-cache'});res.end(await readFile(target));
  }catch{res.writeHead(404);res.end('Not found');}
});
const wss=new WebSocketServer({server,maxPayload:4096});
const rooms=new Map();
const send=(ws,data)=>{if(ws.readyState===1&&ws.bufferedAmount<200000)ws.send(JSON.stringify(data));};
function fillBots(room){
  for(const [id,p] of room.players)if(p.bot)room.players.delete(id);
  const humans=[...room.players.values()];
  let index=0;
  if(!humans.some(p=>p.role==='hunter')) room.players.set('ranger',createPlayer('ranger','hunter','Ranger · AI',true));
  const animalCount=humans.filter(p=>p.role==='squirrel').length;
  for(let i=animalCount;i<3;i++){const id='squirrel-'+i;room.players.set(id,createPlayer(id,'squirrel',['Hazel','Juniper','Pip'][i]+' · AI',true,index++));}
}
function leave(ws){if(!ws.room)return;const r=ws.room;r.players.delete(ws.playerId);r.clients.delete(ws);if(r.clients.size)fillBots(r);else rooms.delete(r.code);ws.room=null;}
wss.on('connection',ws=>{
  ws.playerId=randomBytes(6).toString('hex');ws.alive=true;ws.messages=0;
  ws.on('pong',()=>ws.alive=true);
  ws.on('message',raw=>{
    if(++ws.messages>150)return;
    let m;try{m=JSON.parse(raw);}catch{return;}
    if(m.type==='join'){
      if(!['hunter','squirrel'].includes(m.role))return;
      const requested=String(m.code||'').trim().toUpperCase();
      if(requested&&!rooms.has(requested)){send(ws,{type:'error',message:'That expedition was not found. Check the room code.'});return;}
      const existing=requested?rooms.get(requested):null;
      if(existing&&existing.clients.size>=RULES.maxPlayers){send(ws,{type:'error',message:'This expedition is full (8 players).'});return;}
      leave(ws);
      let room=existing;
      if(!room){let code;do{code=randomBytes(3).toString('hex').toUpperCase();}while(rooms.has(code));room={code,players:new Map(),clients:new Set(),time:0,startedAt:performance.now(),captures:0,acorns:0,winner:null,events:[]};rooms.set(code,room);}
      ws.room=room;room.clients.add(ws);
      const name=String(m.name||'Explorer').trim().slice(0,20)||'Explorer';
      room.players.set(ws.playerId,createPlayer(ws.playerId,m.role,name,false,room.clients.size-1));fillBots(room);
      send(ws,{type:'joined',id:ws.playerId,code:room.code,rules:RULES});
    }else if(m.type==='input'&&ws.room){
      const p=ws.room.players.get(ws.playerId),input=m.input||{},clean={};
      for(const k of ['forward','back','left','right','sprint','crouch','jump','climb','thermal','reload','fire'])clean[k]=input[k]===true;
      if(Number.isFinite(input.yaw))clean.yaw=input.yaw%(Math.PI*2);
      if(Number.isFinite(input.pitch))clean.pitch=input.pitch;
      for(const k of ['jump','climb','thermal','reload','fire'])if(clean[k]&&!p.input[k]){p.pendingActions??={};p.pendingActions[k]=true;}
      p.input=clean;
    }else if(m.type==='leave')leave(ws);
    else if(m.type==='restart'&&ws.room&&ws.room.winner){const r=ws.room;r.time=0;r.startedAt=performance.now();r.winner=null;r.captures=0;r.acorns=0;for(const [id,p]of r.players)r.players.set(id,createPlayer(id,p.role,p.name,p.bot));}
  });
  ws.on('close',()=>leave(ws));ws.on('error',()=>leave(ws));
});
let last=performance.now(),tick=0;
const timer=setInterval(()=>{
  const current=performance.now(),dt=Math.min((current-last)/1000,.07);last=current;tick++;
  for(const room of rooms.values()){
    if(!room.winner)room.time=(current-room.startedAt)/1000;
    for(const p of room.players.values()){
      if(p.bot)botInput(p,room,room.time);
      const held=p.input;if(p.pendingActions)p.input={...held,...p.pendingActions};
      stepPlayer(p,room,dt,room.time);p.pendingActions=null;p.input=held;
    }
    if(room.time>=RULES.round&&!room.winner)room.winner='squirrels';
    if(tick%2===0){const players=[...room.players.values()].map(({input,lastAction,pendingActions,...p})=>p);const state={type:'state',time:room.time,players,captures:room.captures,acorns:room.acorns,winner:room.winner,events:room.events.splice(0)};for(const client of room.clients)send(client,state);}
  }
  if(tick%30===0)for(const client of wss.clients)client.messages=0;
},1000/30);
const heartbeat=setInterval(()=>{for(const ws of wss.clients){if(!ws.alive){ws.terminate();continue;}ws.alive=false;ws.ping();}},30000);
const port=Number(process.env.PORT)||3000;
server.listen(port,'0.0.0.0',()=>{
  console.log(`WILDLINE is running at http://localhost:${port}`);
  for(const nets of Object.values(networkInterfaces()))for(const net of nets||[])if(net.family==='IPv4'&&!net.internal)console.log(`Co-op on your network: http://${net.address}:${port}`);
});
function shutdown(){clearInterval(timer);clearInterval(heartbeat);for(const ws of wss.clients)ws.terminate();wss.close();server.close();}
process.on('SIGTERM',shutdown);process.on('SIGINT',shutdown);
