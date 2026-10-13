import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { mkdir,readFile,writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const root=resolve('_reference/sadx/rea-capability-audit');
await mkdir(root,{recursive:true});
const client=new Client({name:'sadx-rea-capability-audit',version:'1.0.0'});
const transport=new StdioClientTransport({command:process.execPath,args:[
  '/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp'],
  env:{...process.env,HOPPER_LAUNCHER_PATH:'/Users/burbujamc/Applications/Hopper Disassembler.app/Contents/MacOS/hopper'}});
const readEvidence=async path=>{
  const record=JSON.parse(await readFile(resolve(path),'utf8'));
  if(record.structuredContent?.evidence_id)return record.structuredContent;
  for(const item of record.content??[]){
    if(item.type==='text'){
      const value=JSON.parse(item.text);
      if(value.evidence_id)return value;
    }
  }
  throw new Error('No complete Evidence in '+path);
};
const call=async(name,args,label)=>{
  const result=await client.callTool({name,arguments:args},{timeout:120000});
  await writeFile(resolve(root,label+'.json'),JSON.stringify(result)+'\n');
  if(result.isError)throw new Error('REA rejected '+label+'; full error retained');
  return result;
};
await client.connect(transport);
try{
  const catalog=await client.listTools();
  await writeFile(resolve(root,'catalog.json'),JSON.stringify(catalog)+'\n');
  const selected=['analyze_function','batch_decompile','get_call_graph','trace_call_path',
    'build_call_path','read_function_instructions','inspect_native_instruction',
    'resolve_native_call_targets','trace_native_values','inspect_native_api',
    'compare_functions','compare_bundles','capture_process_scenario',
    'compare_process_captures','build_reconstruction_obligation_ledger',
    'evaluate_reconstruction_coverage','verify_reconstruction','record_unknown','list_unknowns'];
  console.log(JSON.stringify({catalog_count:catalog.tools.length,selected:selected.map(name=>({
    name,advertised:catalog.tools.some(tool=>tool.name===name)}))}));
  await call('binary_session',{},'session');
  // Dossiers can cite sibling observations through their typed result fields.
  // Import the authenticated closure before composing those records.
  await call('import_evidence_bundle',{
    path:resolve('_reference/sadx/collision-primitives/evidence-bundle.json')},'import');
  const functions=await Promise.all([
    readEvidence('_reference/sadx/movement/set-position.json'),
    readEvidence('_reference/sadx/position-dependencies/speed-response.json'),
    readEvidence('_reference/sadx/physics-dependencies/vector-angle.json')]);
  await call('build_call_path',{functions,start:{address:'0x44cdf0'},goal:{address:'0x438410'}},'contact-call-path');
  const left=await readEvidence('_reference/sadx/collision-primitives/shape-419030.json');
  const right=await readEvidence('_reference/sadx/collision-primitives/shape-419270.json');
  await call('compare_functions',{left,right},'shape-function-comparison');
  await call('export_evidence_bundle',{path:resolve(root,'evidence-bundle.json'),overwrite:false},'export');
  console.log('Retained live catalog, session and two native Evidence workflows; no target execution or provider launched.');
}finally{await client.close();}
