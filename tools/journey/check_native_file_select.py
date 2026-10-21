#!/usr/bin/env python3
"""Normal-input native boot, save navigation and delivered menu audio.

Requires two distinct valid SonicDX saves in the imported profile. The application
seeds an isolated native profile using its production first-launch path. This
checkpoint does not certify gameplay, reference pixels or gameplay performance.
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


def run(application, assets, output):
    application, assets, output = application.resolve(), assets.resolve(), output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    captures = output / 'captures'
    check(not captures.exists(), 'choose a fresh output directory; stale captures cannot verify this run')
    fixture_path = Path(__file__).resolve().parents[2] / 'reference/frozen-corpora/native-file-select-panel.json'
    panel_reference = json.loads(fixture_path.read_text())
    frames = (740, 899, 902, 950, 1002, 1100, 1190, 1251, 1300, 1450, 1850)
    imported = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (assets / 'saves').glob('*') if path.is_file()}
    with tempfile.TemporaryDirectory(prefix='sadx-native-file-select-') as temporary:
        profile = Path(temporary) / 'savedata'
        command = [str(application), '--assets', str(assets), '--offscreen', '--paced',
                   '--exit-after', str(frames[-1]), '--save-directory', str(profile),
                   '--key-events', '500:13:down,501:13:up,750:13:down,751:13:up,'
                   '900:40:down,901:40:up,1000:38:down,1001:38:up,'
                   '1150:13:down,1151:13:up,1250:90:down,1251:90:up,'
                   '1400:90:down,1401:90:up,1650:13:down,1651:13:up',
                   '--capture-directory', str(captures), '--capture-at',
                   ','.join(map(str, frames))]
        with (output / 'application.log').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=120)
        check(result.returncode == 0, f'native application exit {result.returncode}; see application.log')
        states = {frame: json.loads((captures / f'{frame}.json').read_text()) for frame in frames}
        check(states[740]['execution'] == 'native' and states[740]['game_mode'] == 12 and
              states[740]['global_timer'] == 240,
              'native title checkpoint did not retain the verified startup timing')
        for frame, index in ((899, 1), (902, 2), (950, 2), (1002, 1), (1100, 1)):
            state = states[frame]
            check(state['execution'] == 'native' and state['game_mode'] == 12,
                  f'{frame}: native title mode was not reached')
            check(state['save_file_count'] == 2, f'{frame}: qualification requires two valid saves')
            check(state['selected_save'] == state['menu_selected_save'] == index,
                  f'{frame}: normal arrow input did not select save {index}')
            check(state['file_select_phase'] == 2 and state['file_select_submode'] == 1 and
                  state['file_select_color'] == 4294967295 and state['file_select_fade'] == 1,
                  f'{frame}: file select did not finish its real fade/control transition')
            image = (captures / f'{frame}.ppm').read_bytes()
            check(image.startswith(b'P6\n640 480\n255\n') and len(image) == 921615,
                  f'{frame}: native screenshot is missing or malformed')
        check(states[899]['selected_save_name'] != states[902]['selected_save_name'] and
              states[899]['save_checksum'] != states[902]['save_checksum'],
              'save navigation did not reload the distinct save data')
        check(states[1100]['selected_save_name'] == states[899]['selected_save_name'] and
              states[1100]['save_checksum'] == states[899]['save_checksum'],
              'return navigation did not restore the original selected save')
        reference_panels = 0
        for frame in (899, 950):
            state = states[frame]
            if state['selected_save_name'] != panel_reference['selection']['name']:
                continue
            check(state['save_checksum'] == panel_reference['selection']['checksum'],
                  'reference panel requires the qualified imported SonicDX01 save')
            pixels = (captures / f'{frame}.ppm').read_bytes().split(b'\n', 3)[3]
            rectangle = panel_reference['rectangle']
            x, y = rectangle['x'], rectangle['y']
            width, height = rectangle['width'], rectangle['height']
            panel = b''.join(pixels[((y + row) * 640 + x) * 3:
                                    ((y + row) * 640 + x + width) * 3]
                             for row in range(height))
            check(hashlib.sha256(panel).hexdigest() == panel_reference['rgb_sha256'],
                  f'{frame}: saved-story panel differs from the pinned original image')
            reference_panels += 1
        check(reference_panels != 0, 'qualified reference save was not reached through normal navigation')
        last = states[1100]
        check(last['texture_slots_in_use'] == states[899]['texture_slots_in_use'],
              'idle save navigation changed resident texture ownership')
        for kind, events in (('music', 2), ('voice', 1), ('sound', 3)):
            key = f'{kind}_play_events' if kind == 'sound' else f'{kind}_start_events'
            check(last[key] == events, f'incorrect {kind} playback events')
            check(last[f'{kind}_audio_nonzero_samples'] > 0 and
                  last[f'{kind}_audio_completed_buffers'] > 0 and
                  last[f'{kind}_audio_device_error'] == 0,
                  f'{kind} did not deliver samples successfully')
        check(states[1190]['dialog_phase'] == 2 and states[1190]['dialog_cursor'] == 0 and
              states[1190]['file_select_submode'] == 2,
              'normal Enter did not open the real selected-save dialog')
        check(states[1251]['dialog_cursor'] == 1 and states[1251]['dialog_phase'] in (3, 4),
              'normal cancel input did not select and close the dialog cancel branch')
        check(states[1300]['dialog_phase'] == 5 and states[1300]['file_select_submode'] == 1,
              'dialog cancel did not return control to FileSelect')
        check(states[1450]['file_select_submode'] == 10,
              'normal cancel did not leave FileSelect through its menu transition')
        reentered = states[1850]
        check(reentered['file_select_phase'] == 2 and reentered['file_select_submode'] == 1,
              'normal title confirmation did not reenter FileSelect')
        check(reentered['selected_save_name'] == last['selected_save_name'] and
              reentered['save_checksum'] == last['save_checksum'],
              'menu reentry did not reload the same real save')
        check(reentered['voice_start_events'] == 2 and reentered['music_start_events'] == 4 and
              reentered['sound_play_events'] == 9,
              'menu reentry did not request and deliver its real audio resources')
        for kind in ('music', 'voice', 'sound'):
            check(reentered[f'{kind}_audio_nonzero_samples'] > 0 and
                  reentered[f'{kind}_audio_completed_buffers'] > 0 and
                  reentered[f'{kind}_audio_device_error'] == 0,
                  f'menu reentry {kind} delivery failed')
        resources = states[1190]
        check(resources['caption_buffers_allocated'] > 0 and
              resources['caption_buffers_allocated'] == resources['caption_buffers_freed'] ==
              resources['caption_thread_waits'],
              'empty-English help skipped its actual allocation/wait/release caller chain')
        for frame in (1190, 1251, 1300, 1850):
            state = states[frame]
            check(not state['caption_buffer_live'] and state['caption_timer'] == 0 and
                  state['message_texture_release_count'] == 0 and
                  state['caption_buffers_allocated'] == state['caption_buffers_freed'],
                  f'{frame}: caption resources survived their source frame tail')
            check(state['texture_slots_in_use'] == last['texture_slots_in_use'],
                  f'{frame}: dialog/cancel/reentry changed resident texture ownership')
            image = (captures / f'{frame}.ppm').read_bytes()
            check(image.startswith(b'P6\n640 480\n255\n') and len(image) == 921615,
                  f'{frame}: native dialog/reentry image is missing or malformed')
        for name, digest in imported.items():
            check(hashlib.sha256((profile / name).read_bytes()).hexdigest() == digest,
                  f'loading/navigation changed seeded save {name}')
            check(hashlib.sha256((assets / 'saves' / name).read_bytes()).hexdigest() == digest,
                  f'loading/navigation changed imported save {name}')
    report = {'passed': True, 'frames': frames[-1], 'execution': 'native',
              'application_exit_status': result.returncode,
              'normal_input_command': command, 'state_checkpoints': states,
              'screenshot_sha256': {str(frame): hashlib.sha256(
                  (captures / f'{frame}.ppm').read_bytes()).hexdigest() for frame in frames},
              'reference_panel_matches': reference_panels,
              'reference_panel_fixture_sha256': hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
              'limits': ['No native story gameplay or gameplay63FPS proof.',
                         'Screenshots captured; reference pixel comparison is a separate check.',
                         'Two-save fixture; popup cancel, return and reentry are covered. Fresh-file creation and main-menu confirmation remain later checkpoints.']}
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Native FileSelect navigation, dialog cancel, title return/reentry, resource lifetime and delivered audio pass')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=Path('build/sadx-native'))
    parser.add_argument('--assets', type=Path, default=Path('game/imported'))
    parser.add_argument('--output', type=Path, required=True)
    arguments = parser.parse_args()
    run(arguments.app, arguments.assets, arguments.output)


if __name__ == '__main__':
    main()
