import { defineConfig } from '@playwright/test';
export default defineConfig({testDir:'tests/browser',timeout:90000,workers:1,use:{baseURL:'http://localhost:3000',viewport:{width:1100,height:760},launchOptions:{args:['--use-angle=swiftshader','--enable-webgl']}}});
