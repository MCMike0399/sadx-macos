#!/usr/bin/env python3
"""Normal-input native save confirmation, MainMenu navigation, cancel and reentry.

This functional checkpoint does not certify CharacterSelect, fresh-save creation,
the animated selection highlight, floating-point status parity, gameplay or performance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def check_reference_regions(captures):
    fixture = Path(__file__).resolve().parents[2] / 'reference/frozen-corpora/native-main-menu-regions.json'
    reference = json.loads(fixture.read_text())
    for frame in reference['native_frames']:
        data = (captures / f'{frame}.ppm').read_bytes()
        header = b'P6\n640 480\n255\n'
        check(data.startswith(header) and len(data) == 921615,
              f'{frame}: actual native capture missing or malformed')
        pixels = data[len(header):]
        for region in reference['regions']:
            left, top, right, bottom = region['rectangle']
            rgb = b''.join(pixels[3 * (y * 640 + left):3 * (y * 640 + right)]
                           for y in range(top, bottom))
            check(hashlib.sha256(rgb).hexdigest() == region['rgb_sha256'],
                  f"{frame}: MainMenu source image region {region['name']} differs")


def run(application, assets, output):
    application, assets, output = application.resolve(), assets.resolve(), output.resolve()
    check(application.is_file(), f'required native application missing: {application}')
    for name in ('main-menu-dialog.json', 'music-assets.json', 'menu-dialog.json'):
        check((assets / name).is_file(), f'required native MainMenu asset missing: {assets / name}')
    output.mkdir(parents=True, exist_ok=True)
    captures = output / 'captures'
    check(not captures.exists(), 'choose a fresh output directory; stale captures cannot verify this run')
    imported = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (assets / 'saves').glob('*') if path.is_file()}
    frames = (899, 902, 970, 1100, 1250, 1301, 1390, 1450, 1690, 1850)
    with tempfile.TemporaryDirectory(prefix='sadx-native-main-menu-') as temporary:
        profile = Path(temporary) / 'savedata'
        command = [str(application), '--assets', str(assets), '--offscreen', '--paced',
                   '--exit-after', '1850', '--save-directory', str(profile),
                   '--key-events', '500:13:down,501:13:up,750:13:down,751:13:up,'
                   '900:40:down,901:40:up,950:13:down,951:13:up,'
                   '1050:13:down,1051:13:up,1300:40:down,1301:40:up,'
                   '1370:90:down,1371:90:up,1650:13:down,1651:13:up',
                   '--capture-directory', str(captures), '--capture-at', ','.join(map(str, frames))]
        with (output / 'application.log').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=120)
        check(result.returncode == 0, f'native MainMenu application exit {result.returncode}; see application.log')
        states = {frame: json.loads((captures / f'{frame}.json').read_text()) for frame in frames}
        chosen = states[902]
        check(chosen['selected_save_name'] == 'SonicDX01.snc',
              'MainMenu qualification needs the pinned SonicDX01 snapshot reached by normal Down input')
        check(chosen['selected_save_name'] != states[899]['selected_save_name'],
              'normal Down did not change the selected save')
        check(states[970]['file_select_submode'] == 2 and states[970]['dialog_phase'] == 2 and
              states[970]['dialog_cursor'] == 0, 'normal Enter did not open selected-save confirmation')
        for frame in (1100, 1250, 1690, 1850):
            state = states[frame]
            check(state['execution'] == 'native' and state['game_mode'] == 12 and
                  state['sequence_mode'] == state['sequence_fixed_mode'] == 6 and
                  state['sequence_waiting'] == 0 and state['main_menu_phase'] == 2 and
                  state['main_menu_submode'] == 1 and state['main_menu_selected'] == -1,
                  f'{frame}: canonical MainMenu did not reach its source Keep/Decide state')
            check(state['active_save_name'] == chosen['selected_save_name'] and
                  state['fallback_save_name'] is None and state['menu_voice_cursor'] == 1,
                  f'{frame}: confirmation did not retain the real selected Sonic save/cursor binding')
            check(state['dialog_phase'] == 2 and state['dialog_cursor'] == 0,
                  f'{frame}: actual MainMenu dialog was not opened at Adventure')
            check(not state['caption_buffer_live'] and state['caption_timer'] == 0 and
                  state['message_texture_release_count'] == 0 and
                  state['caption_buffers_allocated'] == state['caption_buffers_freed'],
                  f'{frame}: caption resources survived their real frame tail')
        check(states[1301]['dialog_cursor'] == 1 and states[1301]['dialog_phase'] == 2,
              'normal Down did not move from Adventure to Trial')
        check(states[1390]['dialog_cursor'] == 5 and states[1390]['dialog_phase'] == 5,
              'normal B/Z did not close the actual MainMenu cancel choice')
        check(states[1450]['sequence_mode'] == 5 and states[1450]['main_menu_phase'] == 0 and
              states[1450]['main_menu_submode'] == 6 and states[1450]['main_menu_selected'] == 6,
              'cancel did not preserve the source optional-entry remap and return to Title')
        check(states[1250]['music_song'] == states[1850]['music_song'] == 101 and
              states[1250]['music_start_events'] == 3 and states[1850]['music_start_events'] == 5,
              'confirmation/reentry did not request and start the actual source MainMenu song101')
        # AudioOutput.open starts a new device queue and resets its sample statistics.
        # At1100 only the old track has started; at1250 generation3 is actually playing101.
        check(states[1100]['music_start_events'] == 2 and
              states[1250]['music_open_events'] == 3 and
              states[1250]['music_audio_nonzero_samples'] > 0 and
              states[1250]['music_audio_completed_buffers'] > 0,
              'MainMenu song101 did not deliver nonzero samples on its new device queue')
        check(states[1850]['sound_play_events'] == 10 and states[1850]['voice_start_events'] == 1,
              'menu navigation/cancel/reentry changed source sound or voice requests')
        for kind in ('music', 'voice', 'sound'):
            last = states[1850]
            check(last[f'{kind}_audio_nonzero_samples'] > 0 and
                  last[f'{kind}_audio_completed_buffers'] > 0 and
                  last[f'{kind}_audio_device_error'] == 0,
                  f'{kind} did not deliver native samples successfully')
        check(states[1250]['texture_slots_in_use'] == states[1690]['texture_slots_in_use'] ==
              states[1850]['texture_slots_in_use'], 'MainMenu cancel/reentry leaked resident texture ownership')
        for frame in frames:
            data = (captures / f'{frame}.ppm').read_bytes()
            check(data.startswith(b'P6\n640 480\n255\n') and len(data) == 921615,
                  f'{frame}: actual native capture missing or malformed')
        check_reference_regions(captures)
        saved = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                 for path in profile.iterdir() if path.is_file()}
        check(saved == imported, 'confirmation/cancel/reentry changed the native save snapshot')
        (output / 'result.json').write_text(json.dumps({
            'passed': True, 'frames': 1850, 'execution': 'native', 'save_files_unchanged': True,
            'canonical_mode': 6, 'music_song': 101,
            'source_image_regions_match': True, 'source_image_frames': [1100, 1250, 1690, 1850],
            'limits': ['Exact source image regions exclude the animated Adventure highlight; raw timing differences and FP partials remain separate.',
                       'No fresh-save persistence, CharacterSelect, gameplay or63FPS qualification.']}, indent=2) + '\n')
    print('Native save confirmation, MainMenu navigation/cancel/reentry and song101 delivery pass')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    arguments = parser.parse_args()
    try:
        run(arguments.app, arguments.assets, arguments.output)
        return 0
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        print(f'Native MainMenu journey failed: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
