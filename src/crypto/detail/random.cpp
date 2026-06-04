// ---------------------------------------------------------------------------
// Native per-OS CSPRNG.
//
//   Windows : BCryptGenRandom(BCRYPT_USE_SYSTEM_PREFERRED_RNG)  (links bcrypt)
//   Linux   : getrandom(2), falling back to /dev/urandom only if the syscall is
//             unavailable at runtime (ENOSYS)
//   macOS/* : getentropy(3) in 256-byte chunks (its per-call maximum)
//
// Fails closed: any OS RNG error throws EncryptionError. Compiled with relaxed
// warnings.
// ---------------------------------------------------------------------------

#include "random.hpp"

#include <esphome/api/exception.hpp>

#include <cstddef>

#if defined(_WIN32)

#include <windows.h>
// bcrypt.h must follow windows.h.
#include <bcrypt.h>

namespace esphome::api::noise::detail {

void secure_random(void* buf, std::size_t len) {
    auto* p = static_cast<unsigned char*>(buf);
    while (len > 0) {
        const ULONG chunk = static_cast<ULONG>(len > 0xffffffffUL ? 0xffffffffUL : len);
        const NTSTATUS status = BCryptGenRandom(nullptr, p, chunk, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (!BCRYPT_SUCCESS(status)) {
            throw esphome::api::EncryptionError("BCryptGenRandom failed");
        }
        p += chunk;
        len -= chunk;
    }
}

}  // namespace esphome::api::noise::detail

#elif defined(__linux__)

#include <cerrno>

#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>

namespace esphome::api::noise::detail {

namespace {

// Fallback used only when getrandom(2) is missing (very old kernels: ENOSYS).
void read_urandom(unsigned char* p, std::size_t len) {
    int fd = -1;
    do {
        fd = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        throw esphome::api::EncryptionError("opening /dev/urandom failed");
    }
    std::size_t off = 0;
    while (off < len) {
        const ssize_t r = ::read(fd, p + off, len - off);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            ::close(fd);
            throw esphome::api::EncryptionError("reading /dev/urandom failed");
        }
        if (r == 0) {
            ::close(fd);
            throw esphome::api::EncryptionError("unexpected EOF on /dev/urandom");
        }
        off += static_cast<std::size_t>(r);
    }
    ::close(fd);
}

}  // namespace

void secure_random(void* buf, std::size_t len) {
    auto* p = static_cast<unsigned char*>(buf);
    std::size_t off = 0;
    while (off < len) {
        const ssize_t r = ::getrandom(p + off, len - off, 0);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSYS) {
                read_urandom(p + off, len - off);
                return;
            }
            throw esphome::api::EncryptionError("getrandom failed");
        }
        off += static_cast<std::size_t>(r);
    }
}

}  // namespace esphome::api::noise::detail

#else  // macOS / BSD

#include <sys/random.h>

namespace esphome::api::noise::detail {

void secure_random(void* buf, std::size_t len) {
    auto* p = static_cast<unsigned char*>(buf);
    while (len > 0) {
        const std::size_t chunk = len < 256 ? len : 256;  // getentropy max per call
        if (::getentropy(p, chunk) != 0) {
            throw esphome::api::EncryptionError("getentropy failed");
        }
        p += chunk;
        len -= chunk;
    }
}

}  // namespace esphome::api::noise::detail

#endif
