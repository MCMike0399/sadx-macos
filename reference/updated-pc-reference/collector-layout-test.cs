using System; using SadxReferenceRuntimeV1;
public class Program { public static void Main() {
 Native.CheckLayout();
 byte[] bytes = new byte[5]; bytes[0]=0xe9;
 Array.Copy(BitConverter.GetBytes(unchecked((int)(0xd0000000u-(0x00431517u+5)))),0,bytes,1,4);
 if(Native.DecodeX86RelativeJump(0x00431517u,bytes)!=0xd0000000u) throw new Exception("High-address x86 rel32 wrap failure");
 Array.Copy(BitConverter.GetBytes(-0x105),0,bytes,1,4);
 if(Native.DecodeX86RelativeJump(0x401000u,bytes)!=0x400f00u) throw new Exception("Negative displacement failure");
 CheckPath(@"\Device\HarddiskVolume3\Windows\System32\DriverStore\FileRepository\igd.dll", "C:", @"\Device\HarddiskVolume3", @"C:\Windows\System32\DriverStore\FileRepository\igd.dll");
 CheckPath(@"\device\harddiskvolume3\Windows\System32\a.dll", "D:", @"\Device\HarddiskVolume3", @"D:\Windows\System32\a.dll");
 CheckPath(@"\Device\HarddiskVolume30\a.dll", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3other\a.dll", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3/a.dll", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume4\a.dll", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3\Case Preserved\A.DLL", "C:", @"\Device\HarddiskVolume3", @"C:\Case Preserved\A.DLL");
 CheckPath(null, "C:", @"\Device\HarddiskVolume3", null);
 CheckPath("", "C:", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3\a.dll", null, @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3\a.dll", "", @"\Device\HarddiskVolume3", null);
 CheckPath(@"\Device\HarddiskVolume3\a.dll", "C:", null, null);
 CheckPath(@"\Device\HarddiskVolume3\a.dll", "C:", "", null);
 Console.WriteLine("C# layout/rel32/device-path boundary tests passed; no Windows APIs called");
}
static void CheckPath(string path,string drive,string device,string expected) {
 string actual=Native.DevicePathToDrivePath(path,drive,device);
 if(!String.Equals(actual,expected,StringComparison.Ordinal)) throw new Exception("Device path mismatch: "+path+" => "+actual);
} }