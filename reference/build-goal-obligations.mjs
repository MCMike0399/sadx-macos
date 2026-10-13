import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const root=resolve('_reference/sadx/goal-obligations');
await mkdir(root,{recursive:true});
const client=new Client({name:'sadx-goal-obligation-review',version:'1.0.0'});
const transport=new StdioClientTransport({command:process.execPath,args:[
  '/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp']});
const evidence=result=>result.structuredContent??
  JSON.parse(result.content.find(item=>item.type==='text').text);
const call=async(name,args,label)=>{
  const result=await client.callTool({name,arguments:args},{timeout:120000});
  await writeFile(resolve(root,label+'.json'),JSON.stringify(result)+'\n');
  if(result.isError)throw new Error('REA rejected '+label+'; full result retained');
  return evidence(result);
};
const scopes=[
  ['playable','Complete native SADX player actions, stages and transitions','application',
   'End-to-end playable Mac game with real task, camera and stage ownership.'],
  ['mods','Updated speedrun mod behavior integrated in the native game','runtime',
   'Compare actually loaded intended PC mod behavior with native integrated behavior.'],
  ['precision','Speedrun physics, timing and tricks match the modded Intel reference','native-abi',
   'Observed Windows reference traces and Mac traces, including finite/nonfinite arithmetic and timing.'],
  ['input','Keyboard, Xbox and Switch 2 Pro work in the native game','runtime',
   'Actual physical input delivery and gameplay on Mac; keyboard focus and disconnect/reconnect cases.'],
  ['presentation','Native Metal rendering, camera, audio and sustained performance','runtime',
   'Real gameplay presentation and frame-time measurements using final native resources.'],
  ['package','Complete installable native app, assets, UI, saves and lifecycle','packaging',
   'Clean app installation, source-bound asset decoding, real saves and shutdown/restart validation.'],
];
await client.connect(transport);
try{
  const reviews=[];
  for(const [id,title,layer,next] of scopes){
    const windows=id==='precision'||id==='mods';
    const record=await call('record_unknown',{
      question:title+' remains unverified as a complete deliverable.',severity:'high',domain:'sadx.'+id,
      required_authority:'controlled-replay',required_confidence:'observed',
      required_environment:{id:windows?'user-intel-ultra-nvidia-atlasos':null,
        platform:windows?'Windows 11 AtlasOS':'macOS',architecture:windows?'x86_64':'arm64',isolation:'none'},
      recommended_probes:[{operation:'capture_process_scenario',rationale:next}],relationships:[],
      supporting_evidence_ids:[],contradicting_evidence_ids:[],
    },'unknown-'+id);
    reviews.push({obligation_id:'sadx.'+id,obligation_version:1,title,application_layer:layer,
      family:'complete-native-game',target:{artifact_sha256:'6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74',
        application_node_id:null,semantic_node_id:null,location:'User goal: '+title},
      required:true,required_case_kinds:id==='package'?['positive','negative','teardown']:['positive','negative'],
      required_original_authority:'runtime',required_fixture_authority:'packaged-process',
      required_verifier_authority:'packaged-process',requires_parser_type:id==='package',
      dependency_obligation_ids:id==='playable'?['sadx.mods','sadx.precision','sadx.input','sadx.presentation','sadx.package']:[],
      residual_unknown_ids:[record.result.unknown_id],
      unavailable_authority:windows?['Actual modded Windows hardware execution has not been observed.']:[],
      required_next_evidence:[next],disposition:'active',review_evidence_ids:record.result.mutation_evidence_ids});
  }
  await call('export_evidence_bundle',{path:resolve(root,'unknown-evidence-bundle.json'),overwrite:false},'export-unknowns');
  const bundle=JSON.parse(await readFile(resolve(root,'unknown-evidence-bundle.json'),'utf8'));
  await writeFile(resolve(root,'reviewed-obligations.json'),JSON.stringify(reviews,null,2)+'\n');
  const ledger=await call('build_reconstruction_obligation_ledger',{
    evidence_bundle:bundle,reviewed_obligations:reviews,manifest:{bindings:[],contradictions:[]}},'ledger');
  await call('export_evidence_bundle',{path:resolve(root,'evidence-bundle.json'),overwrite:false},'export');
  console.log(JSON.stringify({evidence_id:ledger.evidence_id,status:ledger.normalized_result.status,
    obligations:ledger.normalized_result.obligations.map(x=>({id:x.obligation_id,status:x.status}))}));
}finally{await client.close();}
