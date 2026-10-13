import sys,struct;sys.path.insert(0,'tools/lib')
from pe_image import PEImage
im=PEImage('game/Sonic Adventure DX/sonic.exe')
t=[s for s in im.sections if s['name']=='.text'][0]
text=im.slice(t['file_offset'],t['raw_size']);base=im.base+t['rva']
for g in [int(x,16) for x in sys.argv[1:]]:
  k=struct.pack('<I',g);i=0;hits=[]
  while True:
    i=text.find(k,i)
    if i<0:break
    hits.append(hex(base+i)+':'+text[max(0,i-2):i].hex());i+=1
  try: v=im.read(g,4).hex()
  except Exception as e: v='bss'
  print(hex(g),'image',v,hits)
