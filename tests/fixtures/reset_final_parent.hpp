#pragma once
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace sadx::test {
// NOTSA: this finite reserved binding may run only under its live admitted parent.
// EOF prevents orphaned native calls from continuing after the owner is interrupted.
inline void retain_reset_parent(unsigned descriptor, unsigned parent_pid) {
    struct stat pipe{};
    if (descriptor < 3 || ::fstat(static_cast<int>(descriptor), &pipe) != 0 ||
        !S_ISFIFO(pipe.st_mode) || unsigned(::getppid()) != parent_pid)
        throw std::invalid_argument("missing live reserved parent pipe");
    std::thread([descriptor] {
        char value;
        ssize_t count;
        do {
            count = ::read(static_cast<int>(descriptor), &value, 1);
        } while (count < 0 && errno == EINTR);
        std::_Exit(125);
    }).detach();
}
} // namespace sadx::test
