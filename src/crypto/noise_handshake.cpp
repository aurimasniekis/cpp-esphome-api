#include <esphome/api/crypto/noise_handshake.hpp>
#include <esphome/api/exception.hpp>

#include <array>
#include <cstddef>

namespace esphome::api::noise {

namespace {
template <std::size_t N>
ByteView view_of(const std::array<std::uint8_t, N>& k) {
    return ByteView(k.data(), k.size());
}
}  // namespace

void NoiseHandshake::initialize(const Role role, const SymmetricKey& psk, const ByteView prologue) {
    ensure_init();
    role_ = role;
    psk_ = psk;
    have_ephemeral_ = false;
    complete_ = false;
    symmetric_.initialize(protocol_name);
    symmetric_.mix_hash(prologue);
}

void NoiseHandshake::ensure_ephemeral() {
    if (!have_ephemeral_) {
        generate_keypair(ephemeral_private_, ephemeral_public_);
        have_ephemeral_ = true;
    }
}

void NoiseHandshake::set_ephemeral_for_testing(const PrivateKey& private_key) {
    ephemeral_private_ = private_key;
    ephemeral_public_ = x25519_base(private_key);
    have_ephemeral_ = true;
}

ByteBuffer NoiseHandshake::write_message(const ByteView payload) {
    ByteBuffer out;
    if (role_ == Role::Initiator) {
        // -> psk, e
        symmetric_.mix_key_and_hash(view_of(psk_));
        ensure_ephemeral();
        out.insert(out.end(), ephemeral_public_.begin(), ephemeral_public_.end());
        symmetric_.mix_hash(view_of(ephemeral_public_));
        symmetric_.mix_key(view_of(ephemeral_public_));
        const ByteBuffer ct = symmetric_.encrypt_and_hash(payload);
        out.insert(out.end(), ct.begin(), ct.end());
    } else {
        // <- e, ee
        ensure_ephemeral();
        out.insert(out.end(), ephemeral_public_.begin(), ephemeral_public_.end());
        symmetric_.mix_hash(view_of(ephemeral_public_));
        symmetric_.mix_key(view_of(ephemeral_public_));
        const SymmetricKey dh = x25519(ephemeral_private_, remote_ephemeral_);
        symmetric_.mix_key(view_of(dh));
        const ByteBuffer ct = symmetric_.encrypt_and_hash(payload);
        out.insert(out.end(), ct.begin(), ct.end());
        complete_ = true;
    }
    return out;
}

bool NoiseHandshake::read_message(const ByteView message, ByteBuffer& out_payload) {
    if (message.size() < key_len) {
        return false;
    }
    for (std::size_t i = 0; i < key_len; ++i) {
        remote_ephemeral_[i] = message[i];
    }
    const ByteView rest = message.subview(key_len);

    try {
        if (role_ == Role::Responder) {
            // -> psk, e
            symmetric_.mix_key_and_hash(view_of(psk_));
            symmetric_.mix_hash(view_of(remote_ephemeral_));
            symmetric_.mix_key(view_of(remote_ephemeral_));
            out_payload = symmetric_.decrypt_and_hash(rest);
        } else {
            // <- e, ee
            symmetric_.mix_hash(view_of(remote_ephemeral_));
            symmetric_.mix_key(view_of(remote_ephemeral_));
            const SymmetricKey dh = x25519(ephemeral_private_, remote_ephemeral_);
            symmetric_.mix_key(view_of(dh));
            out_payload = symmetric_.decrypt_and_hash(rest);
            complete_ = true;
        }
    } catch (const EncryptionError&) {
        return false;
    }
    return true;
}

void NoiseHandshake::split(CipherState& send, CipherState& recv) {
    CipherState c1;
    CipherState c2;
    symmetric_.split(c1, c2);
    if (role_ == Role::Initiator) {
        send = c1;
        recv = c2;
    } else {
        send = c2;
        recv = c1;
    }
}

}  // namespace esphome::api::noise
