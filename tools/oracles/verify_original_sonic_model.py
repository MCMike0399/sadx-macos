"""Standard oracle entry for the bounded Sonic model initialization prefix."""
import argparse
import importlib.util
from pathlib import Path
import sys

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    script = Path(__file__).resolve().parents[2] / 'reference/packets/sonic-model-native/verify_source.py'
    spec = importlib.util.spec_from_file_location('sonic_model_source_judge', script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.argv = [str(script), str(args.library), '--exe', str(args.exe), '--output', str(args.output)]
    return module.main()

if __name__ == '__main__':
    raise SystemExit(main())
