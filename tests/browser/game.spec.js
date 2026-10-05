import { test, expect } from '@playwright/test';
import { WebSocket } from 'ws';
test.beforeEach(async({page})=>{await page.addInitScript(()=>localStorage.setItem('wildline-quality','low'));});
function connected(){return new Promise((resolve,reject)=>{const ws=new WebSocket('ws://localhost:3000');ws.on('open',()=>resolve(ws));ws.on('error',reject);});}
function receive(ws,predicate){return new Promise((resolve,reject)=>{const timeout=setTimeout(()=>reject(Error('Missing room message')),10000);function onMessage(raw){const m=JSON.parse(raw);if(predicate(m)){clearTimeout(timeout);ws.off('message',onMessage);resolve(m);}}ws.on('message',onMessage);});}
test('forest, field guide and graphics settings load without browser errors',async({page})=>{
  const errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('/',{waitUntil:'domcontentloaded'});await expect(page.locator('#start-button')).toBeEnabled({timeout:45000});await page.screenshot({path:'test-results/menu.png'});
  await page.locator('#guide-button').click();await expect(page.locator('#guide')).toBeVisible();await page.locator('#guide [data-close]').last().click();await expect(page.locator('#guide')).toBeHidden();
  await page.locator('#settings-button').click();await expect(page.locator('#settings')).toBeVisible();await page.locator('#sensitivity').fill('1.2');await page.locator('#settings [data-close]').last().click();expect(errors).toEqual([]);
});
test('hunter thermal activates for ten seconds, expires, and refuses early reuse',async({page})=>{
  const errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('/',{waitUntil:'domcontentloaded'});await expect(page.locator('#start-button')).toBeEnabled({timeout:45000});await page.locator('#callsign').fill('Hunter tester');await page.locator('#start-button').click();await expect(page.locator('#hud-role')).toHaveText('HUNTER');await page.waitForFunction(()=>window.wildline?.state?.players.some(p=>p.name==='Hunter tester'));
  await page.locator('#world').click({position:{x:550,y:350}});await page.waitForFunction(()=>document.pointerLockElement?.id==='world');await page.keyboard.press('KeyT',{delay:150});await expect(page.locator('#thermal-label')).toHaveText('THERMAL ACTIVE',{timeout:5000});await expect(page.locator('#scope-overlay')).toBeVisible();await page.screenshot({path:'test-results/hunter-thermal.png'});
  await expect(page.locator('#thermal-label')).toContainText('RECHARGING',{timeout:15000});await expect(page.locator('#scope-overlay')).toBeHidden();await page.keyboard.press('KeyT',{delay:150});await expect(page.locator('#thermal-label')).toContainText('RECHARGING');expect(errors).toEqual([]);
});
test('a squirrel browser joins a live hunter room and responds to movement and jumping',async({page})=>{
  const ws=await connected(),errors=[];page.on('pageerror',e=>errors.push(e.message));try{
    const pending=receive(ws,m=>m.type==='joined');ws.send(JSON.stringify({type:'join',role:'hunter',name:'Co-op host'}));const host=await pending;
    await page.goto('/',{waitUntil:'domcontentloaded'});await expect(page.locator('#start-button')).toBeEnabled({timeout:45000});await page.locator('[data-role="squirrel"]').click();await page.locator('#callsign').fill('Squirrel tester');await page.locator('#room-code').fill(host.code);await page.locator('#start-button').click();await expect(page.locator('#hud-role')).toHaveText('SQUIRREL');await page.waitForFunction(()=>window.wildline?.state?.players.filter(p=>!p.bot).length===2);
    await expect(page.locator('#player-count')).toContainText('2 explorers');await page.locator('#world').click({position:{x:550,y:350}});await page.waitForFunction(()=>document.pointerLockElement?.id==='world');
    const before=await page.evaluate(()=>window.wildline.state.players.find(p=>p.id===window.wildline.id).z);await page.keyboard.down('KeyW');await page.waitForFunction(z=>window.wildline.state.players.find(p=>p.id===window.wildline.id).z<z-1,before);await page.keyboard.up('KeyW');await page.keyboard.press('Space',{delay:150});await expect(page.locator('#movement-state')).toHaveText('AIRBORNE',{timeout:5000});await page.screenshot({path:'test-results/squirrel.png'});expect(errors).toEqual([]);
  }finally{ws.close();}
});
test('an unknown room shows an actionable error',async({page})=>{await page.goto('/',{waitUntil:'domcontentloaded'});await expect(page.locator('#start-button')).toBeEnabled({timeout:45000});await page.locator('#room-code').fill('BAD123');await page.locator('#start-button').click();await expect(page.locator('#connection-message')).toContainText('not found');await expect(page.locator('#start-button')).toBeEnabled();});
