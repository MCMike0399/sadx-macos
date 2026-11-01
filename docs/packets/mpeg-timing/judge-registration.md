# Canonical no-argument registrations

The two new rows initially included an empty trailing tab. gen_make.py accepted
both, but git diff --check reported trailing whitespace. Remove only those empty
trailing fields; both tests still have no arguments and the same target names.
All inherited registrations retain their exact byte prefix. Regeneration still
reports 287 unit tests and 231 oracle targets. No source, expected value, fixture,
tolerance or test input changes. The complete 946-path manifest remains present.
