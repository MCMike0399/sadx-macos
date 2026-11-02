"""Build-only current continuation hunk, preserving the accepted model binding."""
from pathlib import Path
import runpy
ROOT = Path(__file__).resolve().parents[3]
prepared = runpy.run_path(str(ROOT / 'reference/packets/sonic-initialization-native/prepare_wiring.py'))
(ROOT / 'build/sonic-model-selection-preview.cpp').write_text(prepared['after'])
