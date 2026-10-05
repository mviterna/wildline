import * as THREE from 'three';
import { makeForest, makeSquirrel, makeHunter, makeRifle } from './forest.js';
import { ForestAudio } from './audio.js';
import { RULES, trees, acorns, den, ground, clamp } from '/shared/world.js';
const $=id=>document.getElementById(id),canvas=$('world'),scene=new THREE.Scene();
let renderer,camera,forest;
const audio=new ForestAudio(),clock=new THREE.Clock(),actors=new Map(),keys=new Set();
const pendingKeys=new Map();
let socket,joined=false,active=false,menuOpen=true,selectedRole='hunter',myId=null,roomCode='',state=null,yaw=0,pitch=0,aim=false,firing=false,thermal=false,elapsed=0,lastSent=0,recoil=0,lastFootstep=0,toastUntil=0,lastFrame=0;
let sensitivity=Number(localStorage.getItem('wildline-sensitivity')||1);
let quality=localStorage.getItem('wildline-quality')||'high';
let pageFocused=true;
const position=new THREE.Vector3(),target=new THREE.Vector3(),temp=new THREE.Vector3();
const status=(message,error=false)=>{const el=$('connection-message');el.textContent=message;el.classList.toggle('error',error);};
function toast(message){$('toast').textContent=message;$('toast').classList.add('visible');toastUntil=performance.now()+3300;}
const mm=$('minimap').getContext('2d');
const rifle=makeRifle();
function openMenu(){menuOpen=true;keys.clear();firing=false;aim=false;$('menu').hidden=false;$('hud').hidden=true;$('start-button').firstElementChild.textContent=active?'RESUME EXPEDITION':'ENTER THE WILD';if(active)$('room-code').value=roomCode;if(document.pointerLockElement)document.exitPointerLock();}
function closeMenu(){menuOpen=false;$('menu').hidden=true;$('hud').hidden=false;audio.start();lockMouse();}
function lockMouse(){if(document.pointerLockElement===canvas)return;try{const result=canvas.requestPointerLock();result?.catch(()=>toast('Click the forest to capture your mouse. Esc opens camp.'));}catch{toast('Click the forest to capture your mouse.');}}
function connect(){
  socket=new WebSocket(`${location.protocol==='https:'?'wss':'ws'}://${location.host}`);
  socket.addEventListener('open',()=>{status('Play solo with AI, or invite up to 7 friends.');$('start-button').disabled=false;});
  socket.addEventListener('message',event=>{
    const m=JSON.parse(event.data);
    if(m.type==='error'){status(m.message,true);$('start-button').disabled=false;openMenu();return;}
    if(m.type==='joined'){
      myId=m.id;roomCode=m.code;joined=true;active=true;state=null;yaw=0;pitch=0;position.set(selectedRole==='hunter'?0:-7,ground(0,10),selectedRole==='hunter'?10:-4);$('copy-room').innerHTML=`${roomCode} <span>⧉</span>`; $('start-button').disabled=false;closeMenu();
      $('hud-role').textContent=selectedRole.toUpperCase();$('controls-hint').innerHTML=selectedRole==='hunter'?'WASD <span>Move</span> SHIFT <span>Sprint</span> RMB <span>Aim</span> R <span>Reload</span>':'WASD <span>Move</span> SHIFT <span>Scamper</span> E <span>Climb</span> SPACE <span>Long jump</span>';
      $('objective-text').textContent=selectedRole==='hunter'?'Track and tag 8 squirrels.':'Bring 12 acorns back to the den.';$('objective-detail').textContent=selectedRole==='hunter'?'Work together. Listen for movement in the canopy.':'Carry up to 4 acorns. Climb trees to evade the hunters.';toast(selectedRole==='hunter'?'Expedition started. T activates your thermal scope.':'Welcome to the canopy. E grabs a tree; Space launches you.');
    }else if(m.type==='state'){state=m;for(const e of m.events){if(e.type==='shot'){audio.shot();if(e.id===myId){recoil=.05;if(e.hit)toast('Squirrel tagged. +1 for the hunter team.');}}if(e.id===myId&&e.type==='acorn'){audio.acorn();toast('Acorn collected. Bring your stash to the den.');}if(e.id===myId&&e.type==='deposit'){audio.acorn();toast('Stash delivered. Team acorns stored.');}}
      if(m.winner&&document.pointerLockElement)document.exitPointerLock();
    }
  });
  socket.addEventListener('close',()=>{joined=false;active=false;openMenu();status('Connection lost. Start again when the host is available.',true);$('start-button').disabled=false;});
  socket.addEventListener('error',()=>status('Cannot reach the expedition server. Run npm start and reload.',true));
}
function sendInput(now,force=false){
  if(!joined||socket.readyState!==WebSocket.OPEN||(!force&&now-lastSent<32))return;lastSent=now;
  const playable=!menuOpen&&!state?.winner&&document.pointerLockElement===canvas;
  const held=k=>playable&&(keys.has(k)||(pendingKeys.get(k)||0)>now);
  socket.send(JSON.stringify({type:'input',input:{forward:held('KeyW')||held('ArrowUp'),back:held('KeyS')||held('ArrowDown'),left:held('KeyA')||held('ArrowLeft'),right:held('KeyD')||held('ArrowRight'),sprint:held('ShiftLeft')||held('ShiftRight'),crouch:held('KeyC'),jump:held('Space'),climb:held('KeyE'),thermal:held('KeyT'),reload:held('KeyR'),fire:playable&&firing,yaw,pitch}}));
}
function selectRole(role){selectedRole=role;document.querySelectorAll('.role').forEach(b=>{const s=b.dataset.role===role;b.classList.toggle('selected',s);b.setAttribute('aria-pressed',String(s));});$('role-summary').innerHTML=role==='hunter'?'<span class="tiny-icon">⌖</span><p>Track squirrels with your team. A thermal scope gives you <strong>10 seconds of sight</strong> every 3 minutes.</p>':'<span class="tiny-icon">♧</span><p>Forage acorns, <strong>climb into the canopy</strong>, and leap between trees. Outrun the hunters. Bring your stash home.</p>';}
document.querySelectorAll('[data-role]').forEach(b=>b.addEventListener('click',()=>selectRole(b.dataset.role)));
$('start-button').addEventListener('click',()=>{
  if(!renderer||!forest){status('Preparing the forest. Please wait.');return;}
  if(active&&state?.players.find(p=>p.id===myId)?.role===selectedRole&&$('room-code').value.trim().toUpperCase()===roomCode){closeMenu();return;}
  if(socket?.readyState!==WebSocket.OPEN){status('Reconnecting to the host…');connect();return;}
  $('start-button').disabled=true;status('Joining the expedition…');audio.start();
  socket.send(JSON.stringify({type:'join',role:selectedRole,name:$('callsign').value||'Explorer',code:$('room-code').value}));
});
$('guide-button').addEventListener('click',()=>$('guide').showModal());$('settings-button').addEventListener('click',()=>$('settings').showModal());$('expedition-nav').addEventListener('click',()=>{$('guide').close();$('settings').close();});
document.querySelectorAll('[data-close]').forEach(b=>b.addEventListener('click',()=>b.closest('dialog').close()));
$('quality').value=quality;$('sensitivity').value=sensitivity;
$('quality').addEventListener('change',event=>{quality=event.target.value;localStorage.setItem('wildline-quality',quality);forest?.setQuality(quality);});
$('sensitivity').addEventListener('input',event=>{sensitivity=Number(event.target.value);localStorage.setItem('wildline-sensitivity',sensitivity);});
$('audio-toggle').addEventListener('change',event=>audio.setEnabled(event.target.checked));
$('pause-button').addEventListener('click',openMenu);
$('copy-room').addEventListener('click',async()=>{try{await navigator.clipboard.writeText(roomCode);toast(`Room ${roomCode} copied. Share the host address too.`);}catch{toast(`Your expedition code is ${roomCode}.`);}});
$('restart-button').addEventListener('click',()=>{socket.send(JSON.stringify({type:'restart'}));$('round-result').hidden=true;lockMouse();});
$('return-menu').addEventListener('click',()=>{socket.send(JSON.stringify({type:'leave'}));active=joined=false;state=null;roomCode='';$('room-code').value='';openMenu();});
document.addEventListener('keydown',e=>{if(menuOpen||e.target.closest('dialog,input,select'))return;if(['Space','ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.code))e.preventDefault();keys.add(e.code);if(!e.repeat&&['Space','KeyE','KeyT','KeyR'].includes(e.code))pendingKeys.set(e.code,performance.now()+180);if(e.code==='Escape')openMenu();sendInput(performance.now(),true);});
document.addEventListener('keyup',e=>{keys.delete(e.code);sendInput(performance.now(),true);});
document.addEventListener('mousemove',e=>{if(document.pointerLockElement!==canvas||menuOpen)return;const rate=(aim?.00065:.0017)*sensitivity;yaw-=e.movementX*rate;pitch=clamp(pitch-e.movementY*rate,-1.3,1.3);});
canvas.addEventListener('mousedown',e=>{if(!active||menuOpen||state?.winner)return;audio.start();if(document.pointerLockElement!==canvas){lockMouse();return;}if(e.button===0)firing=true;if(e.button===2)aim=true;sendInput(performance.now(),true);});
document.addEventListener('mouseup',e=>{if(e.button===0)firing=false;if(e.button===2)aim=false;});
canvas.addEventListener('contextmenu',e=>e.preventDefault());
document.addEventListener('pointerlockchange',()=>{if(document.pointerLockElement!==canvas){keys.clear();firing=false;aim=false;}});
window.addEventListener('blur',()=>{pageFocused=false;keys.clear();firing=false;aim=false;});
window.addEventListener('focus',()=>{pageFocused=true;});
document.addEventListener('visibilitychange',()=>{if(document.hidden){keys.clear();firing=false;aim=false;sendInput(performance.now());}});
window.addEventListener('resize',()=>{if(!renderer)return;renderer.setSize(innerWidth,innerHeight);camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();});
function updateActors(dt,time){
  const ids=new Set(state?.players.map(p=>p.id)||[]);
  for(const [id,actor]of actors)if(!ids.has(id)){scene.remove(actor);actors.delete(id);}
  for(const p of state?.players||[]){
    let actor=actors.get(p.id);
    if(!actor||actor.userData.role!==p.role){if(actor)scene.remove(actor);actor=p.role==='squirrel'?makeSquirrel():makeHunter();actor.userData.role=p.role;actor.position.set(p.x,p.y,p.z);actors.set(p.id,actor);scene.add(actor);}
    actor.visible=time>=p.respawnAt&&(p.id!==myId||p.role==='squirrel');
    temp.set(p.x,p.y,p.z);actor.position.lerp(temp,1-Math.exp(-dt*16));actor.rotation.y=p.yaw;
    if(p.role==='squirrel'){const stride=p.tree!==null?.04:p.moving>.1?Math.sin(elapsed*17)*.035:0;actor.position.y+=stride;actor.rotation.z=p.tree!==null?.22:Math.sin(elapsed*12)*Math.min(.05,p.moving*.004);if(actor.userData.tail)actor.userData.tail.rotation.z=Math.sin(elapsed*4)*.06;}
    if(actor.userData.thermal!==thermal){actor.userData.thermal=thermal;actor.traverse(child=>{if(!child.isMesh)return;const m=child.material;if(!child.userData.original)child.userData.original=m;child.material=thermal&&p.role==='squirrel'?heatMaterial:child.userData.original;});}
  }
}
const heatMaterial=new THREE.MeshBasicMaterial({color:'#fff5db'});
function updateHUD(p,time){
  $('round-time').textContent=formatTime(Math.max(0,RULES.round-time));$('hunter-score').textContent=`${state.captures} / ${RULES.capturesToWin}`;$('squirrel-score').textContent=`${state.acorns} / ${RULES.acornsToWin}`;
  const humans=state.players.filter(x=>!x.bot).length,bots=state.players.length-humans;$('player-count').textContent=`${humans} explorer${humans===1?'':'s'} · ${bots} AI`;
  $('stamina-fill').style.width=`${p.stamina}%`;$('movement-state').textContent=p.tree!==null?'CLIMBING':p.y>ground(p.x,p.z)+.2?'AIRBORNE':p.moving>.1?(keys.has('ShiftLeft')?'SPRINTING':'MOVING'):'STANDING';
  const bearing=(Math.round(-yaw*180/Math.PI)%360+360)%360,heading=['N','NE','E','SE','S','SW','W','NW'][Math.round(bearing/45)%8];$('compass-text').textContent=`·  ${heading}  ·  ${String(bearing).padStart(3,'0')}°  ·`;
  const available=time>=p.thermalReadyAt,remaining=Math.max(0,p.thermalReadyAt-time);
  $('thermal-label').textContent=thermal?'THERMAL ACTIVE':available?'THERMAL READY':`RECHARGING ${formatTime(remaining)}`;$('thermal-ring').style.strokeDashoffset=88*(available?0:remaining/RULES.thermalCooldown);$('thermal-seconds').textContent=thermal?`${Math.max(0,p.thermalUntil-time).toFixed(1)}s`:'OPTICAL MODE';
  $('thermal-description').textContent=thermal?'HEAT SIGNATURES DETECTED':'10 SEC ACTIVE / 3 MIN RECHARGE';
  $('ammo-count').textContent=p.reloadUntil?'↻':String(p.ammo).padStart(2,'0');
  $('ammo-display').hidden=p.role!=='hunter';document.querySelector('.thermal-indicator').hidden=p.role!=='hunter';
  if(p.role==='squirrel'){$('ammo-display').hidden=false;$('ammo-count').textContent=String(p.bag).padStart(2,'0');$('ammo-display').lastElementChild.innerHTML='/ 04<small>ACORNS IN YOUR STASH</small>';}
  else $('ammo-display').lastElementChild.innerHTML='/ 05<small>TRANQUILIZER RIFLE</small>';
  let context='';
  if(!document.pointerLockElement&&!state.winner)context='<kbd>CLICK</kbd> Return to the wild &nbsp; · &nbsp; <kbd>ESC</kbd> Camp';
  else if(p.role==='squirrel'){
    const near=trees.find(t=>Math.hypot(p.x-t.x,p.z-t.z)<t.r+1.6);
    if(p.tree!==null)context='<kbd>W / S</kbd> Climb &nbsp; <kbd>SPACE</kbd> Launch &nbsp; <kbd>E</kbd> Let go';
    else if(near)context='<kbd>E</kbd> Climb tree';
    else if(p.bag&&Math.hypot(p.x-den.x,p.z-den.z)<8)context='Return to the glowing den to store your acorns';
  }else if(p.reloadUntil)context=`Reloading · ${Math.max(0,p.reloadUntil-time).toFixed(1)}s`;
  else if(!p.ammo)context='<kbd>R</kbd> Reload your rifle';
  $('context-action').innerHTML=context;
  $('respawn').hidden=time>=p.respawnAt;$('respawn-text').textContent=`Returning in ${Math.ceil(p.respawnAt-time)} seconds`;
  $('round-result').hidden=!state.winner;
  if(state.winner){$('winner-text').textContent=state.winner==='hunters'?'The hunters prevail.':'The canopy wins.';$('result-detail').textContent=`${state.captures} squirrels tagged · ${state.acorns} acorns stored · ${formatTime(time)} in the wild`;}
  for(const [id,obj]of forest.acornObjects){obj.visible=p.role==='squirrel'&&!p.collected.includes(id);if(obj.visible)obj.rotation.y=elapsed*.8;}
  drawMap(p);
}
function drawMap(p){
  mm.clearRect(0,0,180,180);mm.save();mm.beginPath();mm.arc(90,90,87,0,Math.PI*2);mm.clip();mm.fillStyle='#12251ecc';mm.fillRect(0,0,180,180);mm.strokeStyle='#adc09c12';mm.lineWidth=1;for(let i=0;i<180;i+=30){mm.beginPath();mm.moveTo(i,0);mm.lineTo(i,180);mm.moveTo(0,i);mm.lineTo(180,i);mm.stroke();}
  const project=(x,z)=>[90+(x-p.x)*1.3,90+(z-p.z)*1.3];mm.fillStyle='#79966c50';for(const t of trees){const[x,y]=project(t.x,t.z);mm.beginPath();mm.arc(x,y,2.1,0,Math.PI*2);mm.fill();}
  mm.fillStyle='#d6dca3';const[dx,dy]=project(den.x,den.z);mm.fillRect(dx-3,dy-3,6,6);
  if(p.role==='squirrel'){mm.fillStyle='#d1a555';for(const a of acorns)if(!p.collected.includes(a.id)){const[x,y]=project(a.x,a.z);mm.beginPath();mm.arc(x,y,1.6,0,6.28);mm.fill();}}
  for(const other of state.players){if(other.id===p.id||state.time<other.respawnAt)continue;const friendly=other.role===p.role;if(!friendly&&!thermal)continue;const[x,y]=project(other.x,other.z);mm.fillStyle=friendly?'#b4caa1':'#fff1bf';mm.beginPath();mm.arc(x,y,3,0,6.28);mm.fill();}
  mm.translate(90,90);mm.rotate(-p.yaw);mm.fillStyle='#edf1ce';mm.beginPath();mm.moveTo(0,-7);mm.lineTo(4,5);mm.lineTo(0,2);mm.lineTo(-4,5);mm.closePath();mm.fill();mm.restore();mm.fillStyle='#c2ccae';mm.font='8px sans-serif';mm.textAlign='center';mm.fillText('N',90,12);
}
function formatTime(t){return `${String(Math.floor(t/60)).padStart(2,'0')}:${String(Math.floor(t%60)).padStart(2,'0')}`;}
function frame(){
  requestAnimationFrame(frame);const now=performance.now();sendInput(now);
  if(document.hidden||(!pageFocused&&active)||document.querySelector('dialog[open]')||now-lastFrame<(menuOpen?100:quality==='low'?33:16))return;
  lastFrame=now;const dt=Math.min(clock.getDelta(),.1);elapsed+=dt;
  const p=state?.players.find(p=>p.id===myId),time=state?.time||0;
  thermal=!!p&&p.role==='hunter'&&time<p.thermalUntil&&!menuOpen;
  const scoped=!!p&&p.role==='hunter'&&(aim||thermal)&&!menuOpen&&!state.winner;
  document.body.classList.toggle('thermal',thermal);$('scope-overlay').hidden=!scoped;$('crosshair').hidden=scoped||p?.role==='squirrel';
  camera.fov=THREE.MathUtils.lerp(camera.fov,scoped?24:65,1-Math.exp(-dt*12));camera.updateProjectionMatrix();
  updateActors(dt,time);
  if(p&&active&&!menuOpen){
    temp.set(p.x,p.y,p.z);position.lerp(temp,1-Math.exp(-dt*18));
    if(p.role==='hunter'){
      const bob=p.moving>.1?Math.sin(elapsed*(keys.has('ShiftLeft')?13:8))*.018:Math.sin(elapsed*1.5)*.003;
      camera.position.set(position.x,position.y+(keys.has('KeyC')?1.1:1.65)+bob,position.z);camera.rotation.set(pitch+recoil,yaw,0,'YXZ');recoil=THREE.MathUtils.lerp(recoil,0,dt*12);rifle.visible=!scoped&&time>=p.respawnAt;rifle.position.set(.23,-.26+bob*.3,-.38);rifle.rotation.set(0,-.06,0);
    }else{
      const distance=p.tree!==null?3.6:4.5,targetPos=new THREE.Vector3(position.x,position.y+.55,position.z);
      const desired=new THREE.Vector3(position.x+Math.sin(yaw)*distance*Math.cos(pitch),position.y+1.8-Math.sin(pitch)*distance,position.z+Math.cos(yaw)*distance*Math.cos(pitch));
      desired.y=Math.max(desired.y,ground(desired.x,desired.z)+.6);
      // Keep the chase camera in front of tree trunks instead of burying it inside the bark.
      const ray=new THREE.Vector3().subVectors(desired,targetPos),length=ray.length(),direction=ray.clone().normalize();
      for(const t of trees){const relative=new THREE.Vector3(t.x,desired.y,t.z).sub(targetPos),along=relative.dot(direction);if(along>.3&&along<length&&desired.y<ground(t.x,t.z)+t.h){const point=targetPos.clone().addScaledVector(direction,along);if(Math.hypot(point.x-t.x,point.z-t.z)<t.r+.3)desired.copy(targetPos).addScaledVector(direction,Math.max(.8,along-.7));}}
      camera.position.lerp(desired,1-Math.exp(-dt*9));camera.lookAt(targetPos);rifle.visible=false;
    }
    updateHUD(p,time);if(p.moving>1&&p.tree===null&&p.y<=ground(p.x,p.z)+.03&&elapsed-lastFootstep>.42){audio.step();lastFootstep=elapsed;}
  }else{
    const t=elapsed*.015;camera.position.set(-19+Math.sin(t)*2,ground(-19,24)+3.6,24+Math.cos(t)*2);camera.lookAt(-1,8,-28);rifle.visible=false;
  }
  if(forest)forest.update(elapsed,active&&p?position:undefined);
  if(now>toastUntil)$('toast').classList.remove('visible');
  renderer.render(scene,camera);
}
try{
  $('start-button').disabled=true;
  renderer=new THREE.WebGLRenderer({canvas,antialias:true,powerPreference:'high-performance'});renderer.setSize(innerWidth,innerHeight);renderer.setPixelRatio(Math.min(devicePixelRatio,1.6));renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.PCFSoftShadowMap;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.05;renderer.outputColorSpace=THREE.SRGBColorSpace;
  camera=new THREE.PerspectiveCamera(65,innerWidth/innerHeight,.08,900);scene.add(camera);camera.add(rifle);rifle.visible=false;
  status('Preparing the Blackwood forest…');forest=await makeForest(scene,renderer);forest.setQuality(quality);connect();frame();
  // Read-only diagnostics for development and automated verification.
  window.wildline={get state(){return state;},get role(){return selectedRole;},get roomCode(){return roomCode;},get id(){return myId;},get rendering(){return renderer.info.render;},get position(){return position.clone();}};
}catch(error){$('fatal').hidden=false;$('fatal-detail').textContent=`${error.message}. Make sure WebGL 2 and hardware acceleration are enabled, then reload the page.`;console.error(error);}
