import { spawn } from 'node:child_process';
import { readFile, writeFile, open } from 'node:fs/promises';
const requests=JSON.parse(await readFile('_reference/sadx/refresh-audit-reports.json','utf8'));
let index=0;const results=[];
await Promise.all(Array.from({length:4},async()=>{
  while(index<requests.length){
    const request=requests[index++];
    const log='/private/tmp/sadx-ghidra-audit-'+request.key+'.log';
    const file=await open(log,'w');
    try{
      const result=await new Promise((resolve,reject)=>{
        const child=spawn('_reference/sadx/python/bin/python',request.args,{stdio:['ignore',file.fd,file.fd]});
        child.once('error',reject);
        child.once('exit',(code,signal)=>resolve({key:request.key,code,signal,log}));
      });
      results.push(result);console.log(JSON.stringify(result));
    }finally{await file.close();}
  }
}));
await writeFile('_reference/sadx/refresh-audit-results.json',JSON.stringify(results,null,2)+'\n');
if(results.some(x=>x.signal||![0,1].includes(x.code)))process.exitCode=1;
