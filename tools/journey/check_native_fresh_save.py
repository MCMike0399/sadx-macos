#!/usr/bin/env python3
"""Create a real native Sonic save, restart, cancel, confirm and reenter MainMenu.

Uses only normal key events and a private user profile. Reference drawing proves
only the saving-text region; CharacterSelect/gameplay/performance remain separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

# Original-task reference952, native950: identical source MSG glyph/render region.
# Durable provenance accompanies the separate evidenced judge change.
SAVING_REGION = (455, 35, 635, 78)
SAVING_REGION_SHA256 = 'f49c8a15e9cadd8aab3e1ff1f9df608d91b5d588ccd5f972aa77831d05aec217'


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def file_hashes(directory):
    return {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
            for path in directory.iterdir() if path.is_file()}


def image_region(path):
    data = path.read_bytes()
    header = b'P6\n640 480\n255\n'
    check(data.startswith(header) and len(data) == len(header) + 640 * 480 * 3,
          f'missing or malformed native image: {path}')
    data = data[len(header):]
    left, top, right, bottom = SAVING_REGION
    return b''.join(data[3 * (y * 640 + left):3 * (y * 640 + right)]
                    for y in range(top, bottom))


def play(application, assets, profile, output, frames, capture_frames, keys):
    output.mkdir(parents=True, exist_ok=True)
    captures = output / 'captures'
    check(not captures.exists(), 'choose a fresh output; stale native captures cannot prove this run')
    command = [str(application), '--assets', str(assets), '--offscreen', '--paced',
               '--exit-after', str(frames), '--save-directory', str(profile),
               '--key-events', keys, '--capture-directory', str(captures),
               '--capture-at', ','.join(map(str, capture_frames))]
    with (output / 'application.log').open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=150)
    check(result.returncode == 0, f'native application exit {result.returncode}; see {output / "application.log"}')
    states = {frame: json.loads((captures / f'{frame}.json').read_text()) for frame in capture_frames}
    for frame in capture_frames:
        check(states[frame]['execution'] == 'native', 'fresh-save proof must use native execution')
        image_region(captures / f'{frame}.ppm')
    return states


def check_main(state):
    check(state['game_mode'] == 12 and state['sequence_mode'] == state['sequence_fixed_mode'] == 6
          and state['sequence_waiting'] == 0 and state['main_menu_phase'] == 2
          and state['main_menu_submode'] == 1 and state['dialog_phase'] == 2,
          'normal confirmation did not reach the actual live MainMenu/dialog')
    check(state['active_save_name'] == 'SonicDX03.snc' and state['fallback_save_name'] is None,
          'MainMenu did not retain the real newly created SaveName binding')
    check(state['texture_slots_in_use'] == 355 and state['message_texture_release_count'] == 0
          and not state['caption_buffer_live'] and state['caption_timer'] == 0
          and state['caption_buffers_allocated'] == state['caption_buffers_freed'],
          'fresh-save menu resources survived their source frame/reentry release boundaries')


def run(application, assets, output):
    application, assets, output = application.resolve(), assets.resolve(), output.resolve()
    check(application.is_file(), f'required native application missing: {application}')
    for name in ('message-fonts.json', 'save-files.json', 'main-menu-dialog.json', 'menu-voice.json'):
        check((assets / name).is_file(), f'required fresh-save prerequisite missing: {assets / name}')
    snapshot = assets / 'saves'
    check(snapshot.is_dir(), f'required native save fixture missing: {snapshot}')
    imported = file_hashes(snapshot)
    slots = {name for name in imported if re.fullmatch(r'SonicDX[0-9]{2}\.snc', name)}
    check(slots == {'SonicDX01.snc', 'SonicDX02.snc'},
          'required fresh-save journey needs the pinned two-save snapshot; no checkpoint was skipped')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sadx-native-fresh-save-') as temporary:
        profile = Path(temporary) / 'savedata'
        created = play(application, assets, profile, output / 'create', 1300,
                       (899, 902, 950, 951, 960, 1000, 1300),
                       '500:13:down,501:13:up,750:13:down,751:13:up,'
                       '900:38:down,901:38:up,950:13:down,951:13:up')
        check(created[899]['selected_save'] == 1 and created[902]['selected_save'] == 0,
              'normal Up did not select Create New File')
        pending = created[950]
        check(pending['file_select_submode'] == 12 and pending['save_phase'] == 1
              and pending['save_ready'] == pending['save_write_count'] == pending['save_exec'] == 1
              and pending['saving_message_pending'] == 1 and pending['save_ok'] == 0
              and pending['active_save_name'] is None,
              'fresh save did not pass through the real pending-message/pre-write frame')
        check(hashlib.sha256(image_region(output / 'create/captures/950.ppm')).hexdigest()
              == SAVING_REGION_SHA256, 'native saving text differs from original-task reference glyph pixels')
        written = created[951]
        check(written['save_phase'] == written['save_ready'] == written['save_exec']
              == written['save_write_count'] == written['saving_message_pending'] == 0
              and written['save_ok'] == 1 and written['active_save_name'] == 'SonicDX03.snc',
              'source frame completion did not perform and finish the real save write')
        check(pending['texture_slots_in_use'] == written['texture_slots_in_use'] + 1,
              'saving texture did not release on the following source frame tail')
        check(created[960]['file_select_submode'] == 13,
              'new-save completion skipped the source minimum wait')
        for frame in (1000, 1300):
            check_main(created[frame])
        persisted = file_hashes(profile)
        check(set(persisted) == set(imported) | {'SonicDX03.snc'} and
              all(persisted[name] == digest for name, digest in imported.items()),
              'native creation modified existing imported/profile saves')
        saved_bytes = (profile / 'SonicDX03.snc').read_bytes()
        check(len(saved_bytes) == 1392, 'new native save has the wrong encoded source size')
        check(int.from_bytes(saved_bytes[:4], 'little') == created[1300]['save_checksum'],
              'written save checksum does not match the canonical live save')
        restarted = play(application, assets, profile, output / 'restart', 2150,
                          (899, 970, 1050, 1140, 1190, 1270, 1370, 1500, 1600, 1770, 2000, 2150),
                          '500:13:down,501:13:up,750:13:down,751:13:up,'
                          '950:13:down,951:13:up,1000:90:down,1001:90:up,'
                          '1120:13:down,1121:13:up,1170:39:down,1171:39:up,'
                          '1220:13:down,1221:13:up,1350:13:down,1351:13:up,'
                          '1450:13:down,1451:13:up,1650:90:down,1651:90:up,'
                          '1950:13:down,1951:13:up')
        check(restarted[899]['selected_save_name'] == 'SonicDX03.snc' and
              restarted[899]['save_checksum'] == created[1300]['save_checksum'],
              'application restart did not reload the actual newly written save')
        for frame in (970, 1140, 1370):
            check(restarted[frame]['file_select_submode'] == 2 and
                  restarted[frame]['dialog_phase'] == 2 and restarted[frame]['dialog_cursor'] == 0,
                  'normal reentry did not reopen the actual selected-save confirmation')
        for frame in (1050, 1270):
            check(restarted[frame]['file_select_submode'] == 1 and
                  restarted[frame]['active_save_name'] is None,
                  'cancel unexpectedly confirmed or changed the selected save')
        check(restarted[1190]['dialog_cursor'] == 1, 'normal Right did not select the Cancel choice')
        check(restarted[1770]['sequence_mode'] == 5, 'MainMenu cancel did not return to the actual Title')
        for frame in (1500, 1600, 2000, 2150):
            check_main(restarted[frame])
            check(restarted[frame]['menu_voice_cursor'] == 1, 'confirmation lost source menu-voice cursor1')
        check(restarted[2150]['music_song'] == 101 and restarted[2150]['music_start_events'] == 5,
              'restart/reentry did not deliver the required source songs')
        for kind in ('music', 'voice', 'sound'):
            state = restarted[2150]
            check(state[f'{kind}_audio_nonzero_samples'] > 0 and
                  state[f'{kind}_audio_completed_buffers'] > 0 and state[f'{kind}_audio_device_error'] == 0,
                  f'{kind} did not deliver native audio samples')
        check(file_hashes(profile) == persisted and file_hashes(snapshot) == imported,
              'restart, cancel, confirm or reentry changed native/source save bytes')
        (output / 'result.json').write_text(json.dumps({
            'passed': True, 'execution': 'native', 'create_frames': 1300, 'restart_frames': 2150,
            'created_file': 'SonicDX03.snc', 'save_sha256': persisted['SonicDX03.snc'],
            'save_checksum': created[1300]['save_checksum'], 'saving_region_matches_reference': True,
            'limits': ['Fresh-save/MainMenu checkpoint only; CharacterSelect and gameplay remain separate.',
                       'The saving region alone is reference-image qualified; file ordering/calendar differ elsewhere.',
                       'No gameplay63FPS or new physical-controller qualification.']}, indent=2) + '\n')
    print('Native fresh-save creation, restart persistence, confirm/cancel and reentry pass')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        run(args.app, args.assets, args.output)
        return 0
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        print(f'Native fresh-save journey failed: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
