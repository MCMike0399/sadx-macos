"""Read only the selected Switch2 USB descriptor/property tree, no device open."""
import argparse,json,plistlib,subprocess,pathlib
FIELDS={'IORegistryEntryID','IORegistryEntryName','IOClass','IOObjectClass','IOProviderClass',
 'idVendor','idProduct','bcdDevice','bNumConfigurations','bConfigurationValue','bInterfaceNumber',
 'bAlternateSetting','bNumEndpoints','bInterfaceClass','bInterfaceSubClass','bInterfaceProtocol',
 'bEndpointAddress','bmAttributes','wMaxPacketSize','bInterval','locationID','USB Product Name',
 'MaxPacketSize','PipeType','EndpointAddress','Direction','InterfaceNumber'}
def convert(value):
 if isinstance(value,bytes):return {'hex':value.hex(),'bytes':len(value)}
 if isinstance(value,list):return [convert(x) for x in value]
 if isinstance(value,dict):return {str(k):convert(v) for k,v in value.items()}
 return value
def selected(node):
 values={k:convert(v) for k,v in node.items() if k in FIELDS or 'Descriptor' in k or 'Endpoint' in k}
 return {'properties':values,'property_names':sorted(node),
         'children':[selected(x) for x in node.get('IORegistryEntryChildren',[])]}
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
 result=subprocess.run(['/usr/sbin/ioreg','-a','-r','-c','IOUSBHostDevice'],stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
 if result.returncode:raise RuntimeError(result.stderr.decode(errors='replace'))
 tree=plistlib.loads(result.stdout);found=[]
 def walk(node):
  if isinstance(node,dict):
   if node.get('idVendor')==0x057e and node.get('idProduct')==0x2069:found.append(selected(node));return
   for x in node.values():walk(x)
  elif isinstance(node,list):
   for x in node:walk(x)
 walk(tree)
 a.output.write_text(json.dumps({'operation':'read-onlyIORegistryUSBtree','vendor_id':0x057e,'product_id':0x2069,'selected_devices':found},indent=2)+'\n')
 print('Selected Switch2 USB trees:',len(found))
if __name__=='__main__':main()
