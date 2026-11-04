// NOTSA: register the Python fixture checks in the native unit manifest too.
#include <cstdlib>

int main() {
    return std::system("python3 tests/python/test_private_native_assets.py") == 0 ? 0 : 1;
}
