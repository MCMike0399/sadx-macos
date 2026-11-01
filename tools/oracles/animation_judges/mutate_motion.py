"""Meaningful compiled animation faults; only private source copies are changed."""
import argparse
import json
import pathlib
import subprocess
import sys

from build import COMMON, FLAGS, ROOT, build, sha

ANIMATION = "src/player/player_animation.cpp"
BLEND = "src/player/player_animation_blend.cpp"
FAULTS = {
    "next_high_byte": (ANIMATION,
        "auto next=static_cast<std::uint8_t>(known(record(v,from,0x446919).next,0x446919));",
        "auto next=static_cast<std::uint16_t>(known(record(v,from,0x446919).next,0x446919));", "unit"),
    "cached_alias": (ANIMATION,
        "if(ref.byte_offset==0x38)return v.player.motion.speed.x;",
        "if(ref.byte_offset==0x38){static thread_local std::optional<float> cached;if(!cached)cached=v.player.motion.speed.x;return *cached;}", "unit"),
    "reordered_action_publication": (ANIMATION,
        "}v.tail.buffer_at_148=a;\n    const auto& selected=action(record(v,req,0x44a879),rt,0x44a87c);const auto& object=known(selected.object,0x44a87c);",
        "}\n    const auto& selected=action(record(v,req,0x44a879),rt,0x44a87c);const auto& object=known(selected.object,0x44a87c);v.tail.buffer_at_148=a;", "unit"),
    "declared_count_clamp": (BLEND,
        "    const auto frame=known(v.tail.frame_at_130,at);",
        "    PlayerAnimationKeyBytes bounded=*input;bounded.byte_extent=std::min<std::size_t>(bounded.byte_extent,std::uint64_t(known(n.counts[channel],at))*16);input=&bounded;\n    const auto frame=known(v.tail.frame_at_130,at);", "blend"),
    "normalized_raw_pointer_count": (BLEND,
        "known(n.scale_pointer_bits_at_8,0x43f39c)",
        "static_cast<std::uint32_t>(known(n.keys[2],0x43f39c).id.value_or(0))", "blend"),
    "spilled_vector_fraction": (BLEND,
        "    if(channel==1){const auto coefficient=fp.to_float(f);",
        "    f=fp.from_float(fp.to_float(f));\n    if(channel==1){const auto coefficient=fp.to_float(f);", "blend"),
    "release_order": (ANIMATION,
        "for(std::uint32_t channel=0;channel<(channels==3?3U:2U);++channel){const auto slot=cursor+4*channel;",
        "for(std::uint32_t ordinal=0;ordinal<(channels==3?3U:2U);++ordinal){const auto channel=(channels==3?3U:2U)-1U-ordinal;const auto slot=cursor+4*channel;", "unit"),
    "default_release_order": (ANIMATION,
        "if(data){arena_result(rt.arena->release(data),0x44ae07);m.data=TaskWorkHandle{};}arena_result(rt.arena->release(m.allocation),0x44ae22);ref->motion=std::shared_ptr<PlayerAnimationAllocatedMotion>{};arena_result(rt.arena->release(ref->allocation),0x44ae34);",
        "arena_result(rt.arena->release(ref->allocation),0x44ae34);if(data){arena_result(rt.arena->release(data),0x44ae07);m.data=TaskWorkHandle{};}arena_result(rt.arena->release(m.allocation),0x44ae22);ref->motion=std::shared_ptr<PlayerAnimationAllocatedMotion>{};", "unit"),
}


def unit(output, replacement=None):
    output.mkdir(parents=True, exist_ok=True)
    sources = COMMON.copy()
    if replacement:
        old, new = replacement
        sources[sources.index(old)] = str(new)
    command = FLAGS + ["-Isrc", "tests/unit/animation_characterization.cpp"] + sources + [
        str(ROOT / "build/animation-judges/softfloat/softfloat.a"), "-o", str(output / "test")]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    (output / "build.log").write_text(result.stdout + result.stderr)
    receipt = dict(command=command, build_exit_status=result.returncode)
    if not result.returncode:
        test = subprocess.run([str(output / "test")], capture_output=True, text=True)
        (output / "cases.jsonl").write_text(test.stdout)
        (output / "judge.log").write_text(test.stderr)
        receipt.update(judge_exit_status=test.returncode, cases=len(test.stdout.splitlines()),
                       stream_sha256=sha(output / "cases.jsonl"), executable_sha256=sha(output / "test"))
    return receipt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    baseline = unit(args.output / "baseline")
    assert baseline["build_exit_status"] == 0 and baseline["judge_exit_status"] == 0
    receipts = []
    for name, (source, old, new, judge) in FAULTS.items():
        output = args.output / name
        output.mkdir(parents=True, exist_ok=True)
        body = (ROOT / source).read_text()
        assert body.count(old) == 1
        replacement = output / pathlib.Path(source).name
        replacement.write_text(body.replace(old, new))
        if judge == "unit":
            receipt = unit(output, (source, replacement))
            receipt["caught"] = receipt["build_exit_status"] == 0 and receipt.get("judge_exit_status") == 1
        else:
            compiled = build(output, "player_animation_blend", (source, replacement))
            command = [sys.executable, str(pathlib.Path(__file__).with_name("capture.py")),
                       "player_animation_blend", str(args.exe), compiled["library"], str(output / "judge")]
            result = subprocess.run(command, capture_output=True, text=True)
            (output / "judge.log").write_text(result.stdout + result.stderr)
            report_path = output / "judge/report.json"
            report = json.loads(report_path.read_text()) if report_path.exists() else {}
            receipt = dict(build_exit_status=0, judge_exit_status=result.returncode,
                           library_sha256=compiled["library_sha256"], cases=report.get("cases"),
                           stored_mismatches=report.get("stored_mismatches"),
                           status_mismatches=report.get("status_mismatches"),
                           caught=result.returncode == 1 and bool(report.get("stored_mismatches")))
        receipt.update(name=name, source=source, old=old, new=new,
                       private_source_sha256=sha(replacement))
        receipts.append(receipt)
        print(json.dumps({k: v for k, v in receipt.items() if k not in ("command", "old", "new")}), flush=True)
    (args.output / "receipts.json").write_text(json.dumps(dict(baseline=baseline, faults=receipts), indent=2) + "\n")
    return int(not all(receipt["caught"] for receipt in receipts))


if __name__ == "__main__":
    raise SystemExit(main())
