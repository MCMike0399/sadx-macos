import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { mkdir,readFile,writeFile,copyFile,access } from 'node:fs/promises';
import { resolve } from 'node:path';

let request;
try{request=JSON.parse(await readFile(resolve('_reference/sadx/ghidra-recovery-request.json'),'utf8'));}
catch(error){if(error.code!=='ENOENT')throw error;}
const root=resolve('_reference/sadx/'+(request?.directory??'ghidra-session'));
await mkdir(root,{recursive:true});
const configured=JSON.parse(await readFile(resolve('_reference/sadx/'+
  (request?.environment_file??'tools/ghidra-session-environment.json')),'utf8'));
const snapshot=resolve('_reference/sadx/'+(request?.snapshot_file??'sonic-ghidra-seeded-snapshot.json'));
const client=new Client({name:'sadx-ghidra-source-recovery',version:'1.0.0'});
const transport=new StdioClientTransport({command:process.execPath,args:[
  '/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp'],
  env:{...process.env,...configured}});
let opened=false;
const call=async(name,args,label)=>{
  console.log('Starting '+label);
  const result=await client.callTool({name,arguments:args},{timeout:900000,
    onprogress:({progress,total,message})=>console.log(JSON.stringify({label,progress,total,message}))});
  await writeFile(resolve(root,label+'.json'),JSON.stringify(result)+'\n');
  if(result.isError)throw new Error('REA rejected '+label+'; full error retained');
  console.log('Retained '+label);
  return result;
};
await client.connect(transport);
try{
  await call('binary_session',{},'initial-session');
  let hasSnapshot=false;try{await access(snapshot);hasSnapshot=true;}catch(error){if(error.code!=='ENOENT')throw error;}
  await call('open_binary',{path:resolve('Sonic Adventure DX/sonic.exe'),provider_id:'ghidra',
    ...(hasSnapshot?{snapshot_path:snapshot}:{})},'open');
  opened=true;
  // Separate Ghidra profile, never reuse the Hopper snapshot as this provider.
  if(request){
    for(const entry of request.functions){
      await call('analyze_function',{procedure:entry.procedure},entry.name);
      const record=JSON.parse(await readFile(resolve(root,entry.name+'.json'),'utf8'));
      const evidence=record.structuredContent?.evidence_id?record.structuredContent:
        JSON.parse(record.content.find(item=>item.type==='text').text);
      await writeFile(resolve(root,entry.name+'.pseudocode.txt'),evidence.normalized_result.pseudocode??'');
    }
    for(const address of request.xrefs??[])await call('xrefs',{address},'xrefs-'+address);
  }else{
  await call('analyze_function',{procedure:'0x43c580'},'contact-reaction');
  await call('inspect_native_instruction',{address:'0x43c5aa'},'speed-threshold-branch');
  await call('resolve_native_call_targets',{address:'0x43c6a2'},'plane-call-target');
  await call('trace_native_values',{procedure:'0x43c580',max_depth:1,max_functions:4,
    max_call_sites:24,max_nodes:1000,max_edges:3000,offset:0,limit:1000},'contact-value-flow');
  }
  await call('binary_session',{},'verified-session');
  await call('export_evidence_bundle',{path:resolve(root,'evidence-bundle.json'),overwrite:false},'export');
}finally{
  if(opened){
    try{
      try{await copyFile(snapshot,resolve(root,'snapshot-before.json'));}catch(error){if(error.code!=='ENOENT')throw error;}
      try{await call('close_binary',{snapshot_path:snapshot,overwrite:true},'close');}
      catch(error){
        const result=JSON.parse(await readFile(resolve(root,'close.json'),'utf8'));
        const failure=JSON.parse(result.content.find(item=>item.type==='text').text).error;
        // Retry only after the owned process reports a terminal exit, allowing
        // its already-exited PID to leave the process group enumeration.
        const diagnostics=failure?.details?.diagnostics?.diagnostics;
        if(failure?.code!=='cleanup_incomplete'||diagnostics?.exit_code!==0)throw error;
        await new Promise(resolve=>setTimeout(resolve,1000));
        await call('close_binary',{snapshot_path:snapshot,overwrite:true},'close-after-exit');
      }
    }
    catch(error){console.error(error.message);process.exitCode=1;}
  }
  await client.close();
}
