"""Compile four wrong-owner/store bindings against the actual Gamma caller."""
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from verify_packet import ROOT, OUT, run, unit_command

def main():
    source=(ROOT/'src/runtime/native_gamma_binding.cpp').read_text()
    faults=[('wrong-player','player(player), work(work)','player(context.players.registry.players[0]), work(work)'),
            ('wrong-extra','player(player), work(work)','player(player), work(context.players.gamma.current_extra)'),
            ('wrong-phase-field','kFanPhase = 28','kFanPhase = 32'),
            ('wrong-height-field','kFanHeight = 24','kFanHeight = 36')]
    rows=[]
    for label,old,new in faults:
        assert old in source,label
        path=OUT/('binding-'+label+'.cpp');path.write_text(source.replace(old,new))
        binary=str((OUT/('binding-'+label)).relative_to(ROOT))
        run('build-binding-fault-'+label,unit_command('tests/unit/native_gamma_fan.mm',binary,
            ('-Wno-unused-parameter',str(path.relative_to(ROOT)))))
        rc=run('binding-fault-'+label,[binary,'game/imported'],(-6,1))
        rows.append({'fault':label,'exit_status':rc,'log':str(OUT/('binding-fault-'+label+'.log'))})
    (ROOT/'reference/packets/gamma-fan-native/BINDING-FAULTS.json').write_text(json.dumps(rows,indent=2)+'\n')
if __name__=='__main__':main()
