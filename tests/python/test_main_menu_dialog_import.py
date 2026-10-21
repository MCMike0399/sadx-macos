import importlib.util,struct,unittest
from pathlib import Path
SPEC=importlib.util.spec_from_file_location('main_menu_dialog',Path(__file__).resolve().parents[2]/'tools/import/main_menu_dialog.py')
m=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(m)
class Changed:
 def __init__(self,image,address,replacement):self.image=image;self.address=address;self.replacement=replacement;self.sha256=image.sha256
 def read(self,address,size):
  out=bytearray(self.image.read(address,size));a=max(address,self.address);b=min(address+size,self.address+len(self.replacement))
  if a<b:out[a-address:b-address]=self.replacement[a-self.address:b-self.address]
  return bytes(out)
 def unpack(self,fmt,address):return struct.unpack(fmt,self.read(address,struct.calcsize(fmt)))
class Tests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.image=m.PEImage('reference/updated-pc-reference/game/sonic.exe')
 def test_source_and_all_records(self):
  data,proof=m.declarations(self.image);self.assertTrue(proof['round_trip_exact']);self.assertFalse(proof['runtime_ready']);self.assertEqual([x['dialog'] for x in data['definitions']],[2,3,4]);self.assertEqual([x['choice_count'] for x in data['definitions']],[6,5,3]);self.assertEqual(data['variant_mapping'],[0,1,2,3,4,5,25])
  for row in proof['records']:self.assertEqual(bytes.fromhex(row['bytes']),self.image.read(int(row['address'],16),len(bytes.fromhex(row['bytes']))))
 def test_hash_rejected(self):
  changed=Changed(self.image,0,b'');changed.sha256='0'*64
  with self.assertRaises(ValueError):m.declarations(changed)
 def test_style_rejected(self):
  with self.assertRaises(ValueError):m.declarations(Changed(self.image,0x7ee328+2*60,struct.pack('<I',5)))
 def test_callback_rejected(self):
  with self.assertRaises(ValueError):m.declarations(Changed(self.image,0x7ee328+2*60+4,struct.pack('<I',0)))
 def test_choice_count_rejected(self):
  with self.assertRaises(ValueError):m.declarations(Changed(self.image,0x7ee328+2*60+56,b'\x05'))
 def test_nonfinite_panel_rejected(self):
  pointer=self.image.unpack('<I',0x7ee328+2*60+12)[0]
  with self.assertRaises(ValueError):m.declarations(Changed(self.image,pointer,struct.pack('<I',0x7fc00000)))
 def test_adjacent_cancel_alias_rejected(self):
  with self.assertRaises(ValueError):m.declarations(Changed(self.image,0x7ef900,struct.pack('<I',0x3f800000)))
if __name__=='__main__':unittest.main()
