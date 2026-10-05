export class ForestAudio {
  constructor(){this.enabled=true;this.ctx=null;}
  start(){if(this.ctx){this.ctx.resume();return;}try{this.ctx=new AudioContext();this.master=this.ctx.createGain();this.master.gain.value=this.enabled?.17:0;this.master.connect(this.ctx.destination);const buffer=this.ctx.createBuffer(1,this.ctx.sampleRate*4,this.ctx.sampleRate),data=buffer.getChannelData(0);let brown=0;for(let i=0;i<data.length;i++){brown=(brown+(Math.random()*2-1)*.02)/1.02;data[i]=brown*3;}const noise=this.ctx.createBufferSource();noise.buffer=buffer;noise.loop=true;const filter=this.ctx.createBiquadFilter();filter.type='lowpass';filter.frequency.value=700;noise.connect(filter);const g=this.ctx.createGain();g.gain.value=.22;filter.connect(g);g.connect(this.master);noise.start();this.timer=setInterval(()=>{if(this.enabled)this.bird();},4200);}catch{}}
  setEnabled(on){this.enabled=on;if(this.master)this.master.gain.setTargetAtTime(on?.17:0,this.ctx.currentTime,.2);}
  tone(freq,duration,volume=.2,type='sine',endFreq=freq){if(!this.ctx||!this.enabled)return;const now=this.ctx.currentTime,o=this.ctx.createOscillator(),g=this.ctx.createGain();o.type=type;o.frequency.setValueAtTime(freq,now);o.frequency.exponentialRampToValueAtTime(Math.max(20,endFreq),now+duration);g.gain.setValueAtTime(volume,now);g.gain.exponentialRampToValueAtTime(.001,now+duration);o.connect(g);g.connect(this.master);o.start();o.stop(now+duration);}
  bird(){const f=2200+Math.random()*1400;this.tone(f,.14,.06,'sine',f*.7);setTimeout(()=>this.tone(f*.9,.16,.045,'sine',f*1.1),180);}
  shot(){this.tone(130,.15,1.8,'triangle',35);this.tone(700,.065,.2,'sawtooth',80);}
  acorn(){this.tone(600,.12,.25);setTimeout(()=>this.tone(900,.15,.2),100);}
  step(){this.tone(75,.04,.075,'triangle',35);}
}
