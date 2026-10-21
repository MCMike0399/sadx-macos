import sys,struct,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/import'))
import font_assets
class Image:
    sha256=font_assets.SOURCE_SHA256
    def __init__(self):
        self.data={font_assets.FONT:struct.pack('<4f6I',16,16,256,128,0x1000,0x2000,1,2,3,4),0x1000:struct.pack('<2I',0x3000,1),0x2000:bytes(range(128)),0x3000:struct.pack('<3I',0x4000,0,0),0x4000:b'abc_txt\0'}
    def read(self,address,size):return self.data[address][:size]
    def unpack(self,fmt,address):return struct.unpack(fmt,self.read(address,struct.calcsize(fmt)))
    def string(self,address):return self.data[address].rstrip(b'\0').decode()
class FontAssets(unittest.TestCase):
    def test_inverse(self):
        result,proof=font_assets.declarations(Image());self.assertTrue(proof['round_trip_exact']);self.assertEqual(result['ascii']['proportions'],list(range(128)));self.assertEqual(result['ascii']['colors'],[1,2,3,4]);self.assertNotIn('address',str(result));self.assertEqual(result['ascii']['texture_list']['filenames'],['abc_txt'])
    def test_wrong_source(self):
        image=Image();image.sha256='bad'
        with self.assertRaises(ValueError):font_assets.declarations(image)
    def test_texture_owner(self):
        image=Image();image.data[0x3000]=struct.pack('<3I',0x4000,0,7)
        with self.assertRaises(ValueError):font_assets.declarations(image)
if __name__=='__main__':unittest.main()
