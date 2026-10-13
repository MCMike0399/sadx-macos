// Run from the sadx-macos root (paths are repo-relative: reference/, game/). One owned Ghidra import for successive source-recovery questions. Commands
// arrive over this process's stdin; no original game or runtime is executed.
import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { readFile,writeFile,mkdir,copyFile,access } from 'node:fs/promises';
import { createInterface } from 'node:readline';
import { resolve } from 'node:path';

const config=process.argv[2]?JSON.parse(await readFile(process.argv[2],'utf8')):{};
const root=resolve('reference/'+(config.directory??'ghidra-live-recovery'));
const snapshot=resolve('reference/'+(config.snapshot_file??'sonic-ghidra-shadow-snapshot.json'));
const env=JSON.parse(await readFile('reference/'+
  (config.environment_file??'tools/ghidra-shadow-environment.json'),'utf8'));
const client=new Client({name:'sadx-owned-live-ghidra',version:'1.0.0'});
const transport=new StdioClientTransport({command:process.execPath,args:[
  '/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp'],env:{...process.env,...env}});
const lines=createInterface({input:process.stdin,crlfDelay:Infinity});
const allowed=new Set(['analyze_function','inspect_native_instruction','resolve_native_call_targets',
  'trace_native_values','xrefs','binary_session','list_procedures','procedure_callers','procedure_callees',
  'procedure_references','procedure_info','batch_decompile']);
let opened=false,ordinal=0;
class RejectedAnalysis extends Error {}
await mkdir(root,{recursive:true});
const call=async(name,args,label)=>{
  const result=await client.callTool({name,arguments:args},{timeout:900000,
    onprogress:({progress,total,message})=>console.log(JSON.stringify({label,progress,total,message}))});
  await writeFile(resolve(root,label+'.json'),JSON.stringify(result)+'\n');
  if(result.isError)throw new RejectedAnalysis('REA rejected '+label+'; complete result retained');
  const e=result.structuredContent??JSON.parse(result.content.find(x=>x.type==='text').text);
  if(e.normalized_result?.pseudocode!==undefined)
    await writeFile(resolve(root,label+'.pseudocode.txt'),e.normalized_result.pseudocode);
  console.log(JSON.stringify({retained:label,evidence_id:e.evidence_id}));
};
await client.connect(transport);
const execute=async command=>{
  const label=String(command.label??('request-'+(++ordinal)));
  if(!/^[a-z0-9-]+$/.test(label))throw new Error('Invalid retention label');
  if(command.tool==='export'){
    await call('export_evidence_bundle',{path:resolve(root,label+'-bundle.json'),overwrite:false},label);
    return;
  }
  if(!allowed.has(command.tool))throw new Error('Unsupported static analysis command');
  try{await call(command.tool,command.args??{},label);}
  catch(e){if(!(e instanceof RejectedAnalysis))throw e;console.error(e.message);}
};
try{
  let exists=false;try{await access(snapshot);exists=true;}catch(e){if(e.code!=='ENOENT')throw e;}
  await call('open_binary',{path:resolve('game/Sonic Adventure DX/sonic.exe'),provider_id:'ghidra',
    ...(exists?{snapshot_path:snapshot}:{})},'open');opened=true;
  console.log('READY: send JSON {tool,args,label}; {tool:"export"} retains a bundle; {tool:"close"} closes the owned provider.');
  for await(const line of lines){
    if(!line.trim())continue;
    let command;
    try{command=JSON.parse(line);}
    catch(e){
      await writeFile(resolve(root,'invalid-command-'+(++ordinal)+'.json'),JSON.stringify({input:line,error:e.message})+'\n');
      console.error('Invalid JSON command retained; owned session remains open.');continue;
    }
    if(command.tool==='close')break;
    if(command.tool==='batch'){
      const batch=JSON.parse(await readFile(command.requests_file,'utf8'));
      for(const request of batch)await execute(request);
      continue;
    }
    await execute(command);
  }
}finally{
  lines.close();
  if(opened){
    try{
      try{await copyFile(snapshot,resolve(root,'snapshot-before-close.json'));}catch(e){if(e.code!=='ENOENT')throw e;}
      await call('close_binary',{snapshot_path:snapshot,overwrite:true},'close');
    }catch(e){console.error(e.message);process.exitCode=1;}
  }
  await client.close();
}
