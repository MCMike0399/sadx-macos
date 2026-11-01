"""Read-only candidate review; all mutations occur in disposable metadata fixtures."""
import json
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tests/python'))
import test_refactor_inventory_lineage as fixture
from test_refactor_inventory_lineage_regressions import CommittedLineageReproductions


def run():
    test = CommittedLineageReproductions()
    test.setUp()
    try:
        c = test.case
        name = c.policy['source_lineage_receipt']
        genesis = (c.root / name).read_bytes()
        genesis_ledger = (c.root / 'docs/lineage.json').read_bytes()
        test.chain()
        before = json.loads(c.render()['summary.json'])
        # No reset/rebase: revert the active artifacts and Source in new commits.
        c.run_git('mv', 'src/player/later.hpp', c.new)
        (c.root / name).write_bytes(genesis)
        (c.root / 'docs/lineage.json').write_bytes(genesis_ledger)
        c.commit()
        after = json.loads(c.render()['summary.json'])
        assert before['historical_retained_scope'] == 2
        assert after['historical_retained_scope'] == 1
        assert 'specific intermediate' not in c.render()['lineage-001.jsonl']
        keys = ['historical_retained_scope', 'historical_open_debt', 'accounted_scope', 'accounted_open_entries']
        print(json.dumps(dict(case='committed_receipt_rollback', accepted=True,
            before={k: before[k] for k in keys}, after={k: after[k] for k in keys},
            specific_debt_present='specific intermediate' in c.render()['lineage-001.jsonl']), sort_keys=True))
        # A fresh seal and production preparation also accept the truncated chain.
        c.seal([c.edge], write_receipt=False)
        fixture.inv.prepare_lineage_receipt(name, 'docs/lineage.json')
        c.commit()
        prepared = c.render()
        test.archive()
        assert c.render() == prepared
        print(json.dumps(dict(case='prepare_after_committed_rollback_and_archive',
            preparation_accepted=True, consumption_accepted=True, archive_accepted=True,
            historical_retained_scope=json.loads(prepared['summary.json'])['historical_retained_scope']), sort_keys=True))
    finally:
        test.doCleanups()


if __name__ == '__main__':
    run()
