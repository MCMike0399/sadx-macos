"""Compile semantic faults in isolated copies; no production source is modified."""
import hashlib,json,pathlib,subprocess,sys
root=pathlib.Path.cwd();out=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else 'build/sse-readability/faults');out.mkdir(exist_ok=False)
source=pathlib.Path('src/cpu/sse_math.cpp').read_text()
faults=[
('rounding-selection','modes[(mxcsr >> 13) & 3]','modes[((mxcsr >> 13) & 3) ^ 1]'),
('nan-operand-priority','f32_add(left, right)','f32_add(right, left)'),
('daz-signed-zero','return bits & 0x80000000U;','return 0;'),
('ftz-disabled','tiny && (mxcsr & kFlushToZero)','false && tiny && (mxcsr & kFlushToZero)'),
('exact-tiny-unmasked','const bool tiny = is_denormal32(result) || (flags & kUnderflow);','const bool tiny = bool(flags & kUnderflow);'),
('overflow-precision','if ((flags & kOverflow) && !(mxcsr & kOverflowMask))','if (false && (flags & kOverflow) && !(mxcsr & kOverflowMask))'),
('double-ftz-sign','result &= 0x8000000000000000ULL;','result = 0;'),
('denormal-priority','if (!prior_exception && (is_denormal32(destination.bits) || is_denormal32(source.bits)))','if ((prior_exception || true) && (is_denormal32(destination.bits) || is_denormal32(source.bits)))'),
('denormal-stop','if (!(mxcsr_ & kDenormalMask))','if (false && !(mxcsr_ & kDenormalMask))'),
('incoming-sticky-trap','flags & ~(mxcsr_ >> 7) & kExceptionFlags','mxcsr_ & ~(mxcsr_ >> 7) & kExceptionFlags'),
('drop-sticky-commit','mxcsr_ |= flags;','mxcsr_ = (mxcsr_ & ~kExceptionFlags) | flags;'),
('cvtt-inexact','f32_to_i32_r_minMag(float32_t{source.bits}, true)','f32_to_i32_r_minMag(float32_t{source.bits}, false)'),
('ucomisd-quiet-invalid','if (is_signaling_nan64(destination.bits) || is_signaling_nan64(source.bits))','if (is_nan64(destination.bits) || is_nan64(source.bits))'),
('minss-nan-destination','return source; // Legacy MINSS','return destination; // Legacy MINSS'),
('signed-zero-order','destination == source || !((destination | source) & ~sign_bit)','destination == source'),
('tls-flags-restore','softfloat_exceptionFlags = saved_flags;','softfloat_exceptionFlags = 0;'),
('tls-round-restore','softfloat_roundingMode = saved_rounding;','softfloat_roundingMode = softfloat_round_near_even;'),
('wrong-thread-bypass','if (owner_ != std::this_thread::get_id())','if (false && owner_ != std::this_thread::get_id())'),
]
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffp-contract=off','-fno-fast-math','-Isrc','-I.']
archives=['build/sse-readability/pinned/sse/softfloat-sse.a','build/sse-readability/pinned/base/softfloat.a']
results=[]
for name,old,new in faults:
    if len(sys.argv)>2 and name not in sys.argv[2:]:continue
    assert old in source,name
    directory=out/name;directory.mkdir()
    copy=directory/'sse_math.cpp'
    changed=source.replace(old,new,1).replace('../../third_party/',str(root/'third_party')+'/')
    copy.write_text(changed)
    command=flags+['tests/unit/sse_math_contract.cpp',str(copy),'src/cpu/x87_math.cpp']+archives+['-o',str(directory/'contract')]
    built=subprocess.run(command,capture_output=True,text=True)
    record=dict(fault=name,old=old,replacement=new,source_sha256=hashlib.sha256(copy.read_bytes()).hexdigest(),command=command,compile_exit=built.returncode,compile_stderr=built.stderr,caught=False)
    if built.returncode==0:
        run=subprocess.run([str(directory/'contract')],capture_output=True,text=True)
        record.update(run_exit=run.returncode,stdout=run.stdout,stderr=run.stderr,caught=run.returncode!=0)
    results.append(record)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(name,'INVALID' if built.returncode else 'caught' if record['caught'] else 'SURVIVED',flush=True)
