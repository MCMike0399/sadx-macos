import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { mkdir, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
const suite=process.argv[2] ?? 'gameplay';
if(!['gameplay','movement','coordinates','coordinate-dependencies','crt-dependencies','ground-dependencies','heading-dependencies','physics-dependencies','acos-dependencies','position-dependencies','collision-normal','collision-primitives','task-lifecycle','path-dependencies','scene-dependencies','floor-dependencies','scene-mesh-dependencies','scene-producers','mesh-math-dependencies','mesh-resource-dependencies','task-cleanup-dependencies','mesh-producers','object-cleanup-dependencies','mesh-transform-dependencies','plane-dependencies'].includes(suite)) throw new Error('Unknown suite');
const root=resolve('_reference/sadx/'+suite);
await mkdir(root,{recursive:true});
const client=new Client({name:'sadx-native-gameplay-recovery',version:'0.1.0'});
const transport=new StdioClientTransport({command:process.execPath,args:['/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp'],env:{...process.env,HOPPER_LAUNCHER_PATH:'/Users/burbujamc/Applications/Hopper Disassembler.app/Contents/MacOS/hopper'}});
const manifest=[];
let opened=false;
async function call(name,args,label) {
 const result=await client.callTool({name,arguments:args},{timeout:300000});
 await writeFile(resolve(root,label+'.json'),JSON.stringify(result)+'\n');
 if(result.isError || result.structuredContent?.error) throw new Error('REA rejected '+label+'; retained complete error output');
 console.log('Retained '+label);
 return result;
}
await client.connect(transport);
try {
 await call('open_binary',{path:resolve('Sonic Adventure DX/sonic.exe'),provider_id:'hopper',snapshot_path:resolve('_reference/sadx/sonic-snapshot.json')},'open');
 opened=true;
 // One owned session; no concurrent Hopper work or repeated Sonic_Main.
 const suites={'plane-dependencies':[['plane-parallel-classifier','0x795820']],'mesh-transform-dependencies':[['translate-vector-entry','0x785070'],['scale-vector-entry','0x788a50'],['plane-intersection','0x7887d0']],'object-cleanup-dependencies':[['object-unlink-417ea0','0x417ea0'],['object-unlink-418330','0x418330']],'mesh-producers':[['mesh-registry-refresh','0x4511d0'],['mesh-group-create','0x451790'],['mesh-node-4518d0','0x4518d0'],['mesh-node-order','0x451910'],['mesh-object-traverse','0x451a00'],['meshset-traverse','0x451ad0'],['mesh-bound-452160','0x452160'],['mesh-bound-4521c0','0x4521c0'],['mesh-bound-452230','0x452230']],'task-cleanup-dependencies':[['task-membership-recursive','0x40af70'],['work-arena-initialize','0x40b1a0'],['collision-work-free','0x41c4e0'],['object-work-free','0x4187a0']],'mesh-resource-dependencies':[['sqrt-classifier','0x648bd5'],['math-dispatch-entry','0x64b3a4']],'mesh-math-dependencies':[['matrix-determinant','0x795000'],['crt-square-root-body','0x6452a4'],['crt-entry-6451bb','0x6451bb'],['crt-entry-645270','0x645270']],'scene-producers':[['land-registration','0x43aef0'],['land-stage-binding','0x43b700'],['land-stage-initialize','0x43a260']],'scene-mesh-dependencies':[['scale-matrix','0x784c70'],['mesh-test-450810','0x450810'],['mesh-test-450920','0x450920'],['mesh-test-4509d0','0x4509d0'],['point-787630','0x787630']],'floor-dependencies':[['floor-fallback','0x64917a'],['control-fpu','0x64b1fd'],['round-integer','0x64dbc9'],['fp-classify','0x64b0cd'],['floor-error-return','0x648ee8'],['floor-error','0x64af93'],['floor-error2','0x64afe5']],'collision-primitives':[['translate-matrix','0x781c10'],['calculate-point','0x781670'],['push-matrix','0x7850f0'],['pop-matrix-ex','0x7816e0'],['shape-419030','0x419030'],['shape-419270','0x419270'],['shape-4193a0','0x4193a0'],['shape-419900','0x419900']],'task-lifecycle':[['allocate-task','0x40b030'],['allocate-work','0x40b220'],['free-work','0x40b310'],['destroy-task','0x40b570'],['free-children','0x40b7e0'],['create-elemental-task','0x40b860'],['create-child-task','0x40b940']],'scene-dependencies':[['scene-land-list','0x43acd0'],['scene-broadphase','0x450c10'],['object-query','0x453820'],['contact-initialize','0x43c7d0'],['contact-43cb90','0x43cb90'],['contact-43ca40','0x43ca40'],['contact-43cce0','0x43cce0'],['contact-43d180','0x43d180'],['contact-43cf90','0x43cf90'],['contact-43d8b0','0x43d8b0']],'path-dependencies':[['crt-floor','0x6449f0'],['path-segment','0x49c210'],['path-angle','0x4393c0'],['adjust-angle2','0x4383a0']],'collision-normal':[['normal-query','0x41bc00']],'position-dependencies':[['scene-position-449500','0x449500'],['scene-position-445d10','0x445d10'],['wall-normal','0x41bed0'],['hit-kind','0x41c1c0'],['speed-response','0x43c580'],['scalar-squared','0x787cf0']],'acos-dependencies':[['crt-acos','0x6450f0'],['crt-acos-body','0x64510d']],'physics-dependencies':[['matrix-787610','0x787610'],['vector-squared-length','0x787cc0'],['vector-add','0x785170'],['axis-rotation','0x788df0'],['vector-angle','0x438410'],['normalize-vector','0x418ac0'],['reset-collision-query','0x49c330'],['normalize-cross-vector','0x788d40'],['event-air-acceleration','0x447b90']],'heading-dependencies':[['adjust-angle','0x438350']],'ground-dependencies':[['ground-event-acceleration','0x447510'],['ground-attribute-20000','0x44b0e0'],['ground-attribute-4000','0x447010'],['cross-product-length','0x7889f0'],['player-direction-reset','0x43c130'],['passive-rotation','0x4491e0'],['attribute-rotation','0x449380'],['angle-distance','0x4383b0'],['ground-yaw-443df0','0x443df0'],['ground-yaw-443e60','0x443e60'],['ground-yaw-443c50','0x443c50'],['input-producer','0x40f170']],'crt-dependencies':[['matrix-stack-initialize','0x780180'],['crt-asin-body','0x64503d'],['crt-asin-classify','0x648c18'],['crt-control-nearest','0x648ba5'],['crt-asin-return','0x648c3b'],['crt-matherr-dispatch','0x648b47'],['crt-matherr-handler','0x64dacb']], 'coordinate-dependencies':[['matrix-push-ex','0x781c80'],['matrix-unit','0x7875f0'],['matrix-initialize','0x787590'],['normal-angles','0x4b8e80'],['crt-asin','0x645020'],['crt-ftol','0x644340'],['current-character','0x4144e0']],coordinates:[['world-to-player','0x43ec00'],['task-to-world','0x43ec90'],['floor-to-world','0x43edd0'],['input-query','0x43bdc0'],['matrix-init','0x7850f0'],['matrix-copy','0x785160'],['matrix-push','0x785120'],['matrix-pop','0x785140'],['matrix-vector','0x784d60'],['rotate-x','0x784ee0'],['rotate-y','0x784e20'],['rotate-z','0x784fb0']],gameplay:[['task-list','0x40b0c0'],['task-children','0x40b420'],['task-free','0x40b6c0'],['sonic-action-checks','0x496f50'],['player-initialize','0x442750'],['sonic-post-update','0x4915f0'],['held-object-update','0x4955e0'],['character-shadow-candidate','0x49f1a0'],['hold-object','0x43b9c0'],['underwater','0x446f10'],['ninja-rotate-object','0x7931a0']],movement:[['gravity','0x443800'],['clear-speed','0x43c550'],['get-speed','0x443f50'],['ground-acceleration','0x44c270'],['air-acceleration','0x44b9c0'],['set-position','0x44cdf0'],['friction','0x4432e0'],['inertia','0x443650'],['rotation','0x44bb60'],['player-start-position','0x417e80'],['jump-check','0x491660']]};
 if(suite==='mesh-resource-dependencies') await call('xrefs',{address:'0x3c48550'},'mesh-registry-xrefs');
 if(suite==='scene-mesh-dependencies') {
  for(const address of ['0x3b2f718','0x3b36d38','0x3b36d3c']) await call('xrefs',{address},'xrefs-'+address);
 }
 for(const [name,procedure] of suites[suite]) {
  await call('analyze_function',{procedure},name);
  manifest.push({name,procedure,artifact:name+'.json'});
  await writeFile(resolve(root,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
 }
 await call('export_evidence_bundle',{path:resolve(root,'evidence-bundle.json'),overwrite:false},'export');
} finally {
 if(opened) {
  try { await call('close_binary',{snapshot_path:resolve('_reference/sadx/sonic-snapshot.json'),overwrite:true},'close'); }
  catch(error) { console.error(error.message); process.exitCode=1; }
 }
 await client.close();
}
