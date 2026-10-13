import { Client } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/index.mjs';
import { StdioClientTransport } from '/private/tmp/sadx-rea/node_modules/@modelcontextprotocol/client/dist/stdio.mjs';
import { writeFile } from 'node:fs/promises';
const client = new Client({name: 'sadx-native-investigation', version: '0.1.0'});
const transport = new StdioClientTransport({ command: process.execPath, args: ['/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs','mcp'] });
await client.connect(transport);
try {
  const result = await client.listTools();
  const selected = result.tools.filter(t=> ['open_binary','close_binary','analyze_function','procedure_pseudo_code','save_analysis','export_evidence_bundle','binary_session'].includes(t.name));
  await writeFile('_reference/sadx/rea-selected-tools.json', JSON.stringify(selected,null,2)+'\n');
  console.log(selected.map(t=>t.name).join(', '));
} finally { await client.close(); }
