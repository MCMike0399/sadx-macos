"""The Bochs FSINCOS / FPTAN wrappers (src/cpu/x87_transcendental, fsincos.cc at the pinned Bochs revision)
against Unicorn 2.1.4 executing the FSINCOS (D9 FB) and FPTAN (D9 F2) instructions on random canonical
ext80 operands under every rounding/precision control.

Unicorn is a weak reference here: QEMU converts ST(0) to a host double, calls the host libm sin/cos/tan
and converts back, so its results carry at most 53 significant bits and its argument reduction is the
libm's exact one, while the x87 (and the Bochs model) reduces with a 66-bit approximation of pi. Bit
equality is therefore not expected and is not claimed. What is compared:

  * double_agreement_mismatches (value class): finite in-range operands of the kinds range53 / near_pi /
    tiny (double-representable, so Unicorn's argument is exact) and range64 (full 64-bit significand,
    Unicorn rounds the argument to a double): |native - unicorn| (native rounded to double) must be at
    most 2 double ulps of Unicorn's result + |f'(x)| * |x| * (2^-62 + argument rounding 2^-53 for
    range64), f' = 1 for sine/cosine and 1 + tan^2 for the tangent. The |x| term bounds the different
    argument reductions (x87: 66-bit pi; libm: exact) and the argument rounding. QEMU's libm is not
    correctly rounded, the x87 is within 1 ext80 ulp. This catches wrong signs, swapped outputs, wrong
    quadrants and reduction errors; it does not judge the low bits. Only round-to-nearest control words
    are judged: under a directed RC, QEMU converts the argument with that RC and its libm results were
    measured far less accurate (2026-10-13, 300-bit mpmath reference on the reported examples: Unicorn
    error up to 6.7e-13, native Bochs error <= 6.1e-18), so those cases are only counted
    (directed_rounding_cases, max_ulps_directed_rounding).
  * special_mismatches (value class): zeros (exact, signed), the C2 path |x| >= 2^63 (both leave ST(0)
    and push nothing), NaN operands (quietened payload). Unicorn round-trips a NaN through a double and
    drops the low 11 payload bits (the SDM propagates the ext80 QNaN): counted as
    unicorn_nan_payload_via_double when the payloads agree above those bits. Infinite operands are an SDM #IA with the
    indefinite NaN in Bochs but take the C2 path in QEMU: counted as unicorn_infinity_as_out_of_range,
    not compared.
  * Documented, not judged: ext80 bit differences in the 53-bit range, and the deviation profile for
    larger or full-64-bit operands (the pi-approximation and argument-rounding effects), per magnitude
    bucket in `profile`. Settling the low bits needs the Intel capture (roadmap M4).
"""
import argparse,ctypes,hashlib,json,math,pathlib,random,struct
from fractions import Fraction
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
sha=lambda b:hashlib.sha256(b).hexdigest()
CODE,DATA=0x1000,0x2000
def program(opcode):
    # fld tbyte [DATA]; <op>; fnstsw [DATA+0x30]; fstp tbyte [DATA+0x10]; fstp tbyte [DATA+0x20]
    return (b'\xdb\x2d'+struct.pack('<I',DATA)+opcode+b'\xdd\x3d'+struct.pack('<I',DATA+0x30)
            +b'\xdb\x3d'+struct.pack('<I',DATA+0x10)+b'\xdb\x3d'+struct.pack('<I',DATA+0x20))
OPS=(('FSINCOS',b'\xd9\xfb'),('FPTAN',b'\xd9\xf2'))
def value(se,sig):
    """Exact rational of a finite ext80, or None for inf/NaN."""
    e=se&0x7fff
    if e==0x7fff:return None
    v=Fraction(sig)*Fraction(2)**((e or 1)-16383-63)
    return -v if se&0x8000 else v
def to_double(se,sig):
    v=value(se,sig)
    if v is None:return math.nan if sig<<1&0xffffffffffffffff else math.copysign(math.inf,-1 if se&0x8000 else 1)
    try:d=float(v)
    except OverflowError:d=math.copysign(math.inf,v)
    return math.copysign(d,-1.0) if d==0 and se&0x8000 else d
def ulps(a,b):
    if a==b:return 0
    if math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b):return math.inf
    ia=struct.unpack('<q',struct.pack('<d',a))[0];ib=struct.unpack('<q',struct.pack('<d',b))[0]
    ia=ia if ia>=0 else -(ia&0x7fffffffffffffff);ib=ib if ib>=0 else -(ib&0x7fffffffffffffff)
    return abs(ia-ib)
def operands(rng):
    for n in range(12000):
        k=n%12
        if k<6:  # game range, 53-bit significands (argument exact in a double)
            d=rng.uniform(-64,64) if k<4 else rng.uniform(-7,7)
            yield 'range53',d
        elif k==6:yield 'range64',rng.uniform(-64,64)  # full 64-bit significand added below
        elif k==7:yield 'large',math.ldexp(rng.uniform(0.5,1),rng.randrange(7,63))*rng.choice([1,-1])
        elif k==8:yield 'tiny',math.ldexp(rng.uniform(0.5,1),-rng.randrange(1,1100))*rng.choice([1,-1])
        elif k==9:yield 'near_pi',rng.choice([math.pi/2,math.pi,3*math.pi/2,2*math.pi,math.pi/4])*rng.choice([1,-1,2,3])+rng.uniform(-1e-9,1e-9)
        elif k==10:yield 'c2',math.ldexp(rng.uniform(0.5,1),rng.randrange(64,200))*rng.choice([1,-1])
        else:yield 'special',rng.choice([0.0,-0.0,math.inf,-math.inf,math.nan])
def ext80(d,full,rng):
    """Canonical ext80 of a double; with `full` the low 11 significand bits are filled at random."""
    if math.isnan(d):return 0x7fff,0xc000000000000000|rng.getrandbits(40)<<8
    if math.isinf(d):return (0xffff if d<0 else 0x7fff),1<<63
    if d==0:return (0x8000 if math.copysign(1,d)<0 else 0),0
    m,e=math.frexp(abs(d));sig=int(m*(1<<53))<<11;exp=e-1+16383
    if exp<=0:sig>>=1-exp;exp=0
    if full and exp:sig|=rng.getrandbits(11)
    return (exp|(0x8000 if d<0 else 0)),sig
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn 2.1.4')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_x87_transcendental
    fn.argtypes=[ctypes.c_int,ctypes.c_uint16,ctypes.c_uint16,ctypes.c_uint64,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint64)];fn.restype=ctypes.c_int
    m=Uc(UC_ARCH_X86,UC_MODE_32);m.mem_map(CODE,0x1000);m.mem_map(DATA,0x1000)
    results=[];profile={}
    for opi,(name,opcode) in enumerate(OPS):
        code=program(opcode);m.mem_write(CODE,code)
        row=dict(operation=name,cases=0,double_agreement_cases=0,double_agreement_mismatches=0,special_cases=0,special_mismatches=0,
                 unicorn_infinity_as_out_of_range=0,unicorn_nan_payload_via_double=0,directed_rounding_cases=0,max_ulps_directed_rounding=0,ext80_bit_equal=0,max_ulps_in_agreement_range=0,examples=[])
        rng=random.Random(0xf5c05+opi)
        for kind,d in operands(rng):
            cw=rng.choice([0x37f,0x27f,0x7f,0x77f,0xb7f,0xf7f])
            se,sig=ext80(d,kind in('range64','large'),rng)
            out=(ctypes.c_uint32*4)();ns=(ctypes.c_uint64*2)()
            if not fn(opi,cw,se,sig,out,ns):continue
            m.mem_write(DATA,struct.pack('<QH',sig,se)+bytes(0x40))
            m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
            m.emu_start(CODE,CODE+len(code))
            raw=m.mem_read(DATA+0x10,0x24);st0=struct.unpack('<QH',raw[0:10]);st1=struct.unpack('<QH',raw[16:26]);fpsw=struct.unpack('<H',raw[32:34])[0]
            c2=bool(fpsw&0x400);row['cases']+=1
            # Unicorn results: FSINCOS ST0=cos ST1=sin; FPTAN ST0=1.0 ST1=tan; C2: ST0 = operand, nothing pushed.
            if opi==0:u_first,u_second=(st1[1],st1[0]),(st0[1],st0[0])
            else:u_first,u_second=(st1[1],st1[0]),(st0[1],st0[0])
            n_first=(out[1],ns[0]);n_second=(out[2],ns[1])
            infinite=(se&0x7fff)==0x7fff and sig==1<<63
            def note(why):
                if len(row['examples'])<12:row['examples'].append(dict(kind=kind,why=why,cw=hex(cw),operand=[hex(se),hex(sig)],unicorn=[[hex(x) for x in u_first],[hex(x) for x in u_second]],c2=c2,native_ok=out[0],native=[[hex(x) for x in n_first],[hex(x) for x in n_second]]))
            if infinite:
                row['special_cases']+=1
                if c2 and not out[0]:pass
                elif c2:row['unicorn_infinity_as_out_of_range']+=1
                else:row['special_mismatches']+=1;note('infinity')
                continue
            if kind in('c2','special','tiny') or (se&0x7fff)==0x7fff or not out[0] or c2:
                row['special_cases']+=1
                if bool(out[0])==c2:row['special_mismatches']+=1;note('C2 path disagrees');continue
                if c2:continue  # both leave ST(0)
                if kind!='tiny':
                    same=u_first==n_first and (opi==1 or u_second==n_second)
                    nan=(se&0x7fff)==0x7fff
                    if nan:
                        same=u_first==n_first
                        if not same and u_first[0]==n_first[0] and u_first[1]==n_first[1]&~0x7ff:row['unicorn_nan_payload_via_double']+=1;continue
                    if not same:row['special_mismatches']+=1;note('special value')
                    continue
            nd=[to_double(*n_first),to_double(*n_second)];ud=[to_double(*u_first),to_double(*u_second)]
            if opi==1:ud[1]=nd[1]=1.0
            if (u_first,u_second)==(n_first,n_second) or (opi==1 and u_first==n_first):row['ext80_bit_equal']+=1
            mag=abs(d);bucket=f"{name}:{kind}:2^{max(-1,int(math.log2(mag))) if mag else 'zero'}"
            worst=max(ulps(nd[0],ud[0]),ulps(nd[1],ud[1]) if opi==0 else 0)
            b=profile.setdefault(bucket,dict(cases=0,max_double_ulps=0,within_2=0));b['cases']+=1;b['max_double_ulps']=max(b['max_double_ulps'],worst if worst!=math.inf else 1e300);b['within_2']+=worst<=2
            if kind in('range53','near_pi','tiny','range64') and cw&0xc00:
                row['directed_rounding_cases']+=1;row['max_ulps_directed_rounding']=max(row['max_ulps_directed_rounding'],worst)
            elif kind in('range53','near_pi','tiny','range64'):
                row['double_agreement_cases']+=1;row['max_ulps_in_agreement_range']=max(row['max_ulps_in_agreement_range'],worst)
                reduction=2.0**-62+(2.0**-53 if kind=='range64' else 0)
                derivs=[1.0,1.0] if opi==0 else [1.0+ud[0]*ud[0],0.0]
                ok=all(abs(nd[i]-ud[i])<=2*math.ulp(ud[i])+derivs[i]*mag*reduction for i in range(2 if opi==0 else 1))
                if not ok:row['double_agreement_mismatches']+=1;note(f'{worst} double ulps beyond the reduction bound')
        results.append(row);print(json.dumps({k:v for k,v in row.items() if k!='examples'}),flush=True)
    root=pathlib.Path(__file__).resolve().parents[2]
    names=['src/cpu/x87_transcendental.hpp','src/cpu/x87_transcendental.cpp','third_party/bochs_fpatan/upstream.json','tests/probes/native_x87_transcendental_probe.cpp','tools/oracles/verify_original_x87_transcendental.py']
    report=dict(native_library_sha256=sha(a.native_library.read_bytes()),unicorn_version=unicorn.__version__,source_files={x:sha((root/x).read_bytes()) for x in names if (root/x).exists()},
        results=results,profile=profile,
        values_match=all(r['double_agreement_mismatches']==0 and r['special_mismatches']==0 for r in results),
        passed=False,
        limits=[__doc__.strip(),'passed is false by construction: Unicorn is not bit-exact for FSINCOS/FPTAN (host double libm), so only the double-precision agreement and the special cases are claimed; ext80 low bits and large-operand reduction are unverified until the Intel capture (M4)'])
    a.output.write_text(json.dumps(report,indent=1)+'\n')
    return int(not report['values_match'])
if __name__=='__main__':raise SystemExit(main())
