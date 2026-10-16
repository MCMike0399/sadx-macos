"""Keep the native and portable frame-control dependency boundaries distinct."""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/gate'))
from check_native_frame_control import PORTABLE_HEADERS, validate_closure


class FrameControlIsolationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()

    def write(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        return name

    def test_native_rejects_even_the_declared_portable_adapter(self):
        sources = (
            self.write('src/core.cpp', '#include "guest/frame_control_reference.hpp"\n'),
            self.write('src/guest/frame_control_reference.hpp', 'struct Facts {};\n'),
        )
        with self.assertRaisesRegex(RuntimeError, 'reference-runtime identifier'):
            validate_closure(self.root, sources)
        validate_closure(self.root, sources, PORTABLE_HEADERS)

    def test_portable_header_body_still_rejects_runtime_dependencies(self):
        for forbidden in ('ExecutableImage image;', 'HleProcess process;',
                          '#include <unicorn/unicorn.h>', 'void uc_open();',
                          'sadx::guest::Machine machine;'):
            with self.subTest(forbidden=forbidden):
                source = self.write('src/guest/frame_control_reference.hpp', forbidden)
                with self.assertRaisesRegex(RuntimeError, 'reference-runtime identifier'):
                    validate_closure(self.root, (source,), PORTABLE_HEADERS)

    def test_portable_exception_cannot_admit_another_guest_header(self):
        source = self.write('src/core.cpp', '#include "guest/another.hpp"\n')
        header = self.write('src/guest/another.hpp', 'struct Another {};\n')
        with self.assertRaisesRegex(RuntimeError, 'reference-runtime identifier'):
            validate_closure(self.root, (source, header), PORTABLE_HEADERS)

    def test_dependencies_must_be_declared_and_cannot_escape(self):
        self.write('src/ordinary.hpp', 'struct Ordinary {};\n')
        for include, error in (('ordinary.hpp', 'undeclared maintained include'),
                               ('../ordinary.hpp', 'include escapes'),
                               ('/tmp/ordinary.hpp', 'include escapes')):
            with self.subTest(include=include):
                source = self.write('src/core.cpp', f'#include "{include}"\n')
                with self.assertRaisesRegex(RuntimeError, error):
                    validate_closure(self.root, (source,), PORTABLE_HEADERS)


if __name__ == '__main__':
    unittest.main()
