#include <esphome/api/discovery.hpp>

#include <asio.hpp>

#include <array>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>

namespace esphome::api {

namespace {

constexpr std::uint16_t type_a = 1;
constexpr std::uint16_t type_ptr = 12;
constexpr std::uint16_t type_txt = 16;
constexpr std::uint16_t type_aaaa = 28;
constexpr std::uint16_t type_srv = 33;

constexpr auto service_suffix = "._esphomelib._tcp";

std::uint16_t read_u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

// Read a (possibly compressed) DNS name starting at `pos`. Returns the position
// just past the name in the record stream (following the first pointer, if any).
std::size_t
read_name(const std::uint8_t* buf, const std::size_t len, std::size_t pos, std::string& out) {
    out.clear();
    bool jumped = false;
    std::size_t next = pos;
    int guard = 0;
    while (pos < len) {
        const std::uint8_t label_len = buf[pos];
        if ((label_len & 0xC0) == 0xC0) {  // compression pointer
            if (pos + 1 >= len) {
                break;
            }
            const std::size_t target =
                (static_cast<std::size_t>(label_len & 0x3F) << 8) | buf[pos + 1];
            if (!jumped) {
                next = pos + 2;
            }
            jumped = true;
            pos = target;
            if (++guard > 128) {
                break;  // malformed / loop
            }
            continue;
        }
        if (label_len == 0) {
            pos += 1;
            break;
        }
        if (pos + 1 + label_len > len) {
            break;
        }
        if (!out.empty()) {
            out.push_back('.');
        }
        out.append(reinterpret_cast<const char*>(buf + pos + 1), label_len);
        pos += 1 + label_len;
    }
    return jumped ? next : pos;
}

struct Accumulator {
    std::set<std::string> instances;                                   // from PTR
    std::map<std::string, std::pair<std::string, std::uint16_t>> srv;  // instance -> (host,port)
    std::map<std::string, std::map<std::string, std::string>> txt;     // instance -> kv
    std::map<std::string, std::string> ipv4;                           // host -> a.b.c.d
};

void parse_txt(const std::uint8_t* rdata,
               const std::size_t rdlen,
               std::map<std::string, std::string>& out) {
    std::size_t i = 0;
    while (i < rdlen) {
        const std::size_t n = rdata[i];
        ++i;
        if (i + n > rdlen) {
            break;
        }
        const std::string entry(reinterpret_cast<const char*>(rdata + i), n);
        i += n;
        if (const auto eq = entry.find('='); eq == std::string::npos) {
            out[entry];  // valueless key
        } else {
            out[entry.substr(0, eq)] = entry.substr(eq + 1);
        }
    }
}

void parse_packet(const std::uint8_t* buf, const std::size_t len, Accumulator& acc) {
    if (len < 12) {
        return;
    }
    const std::uint16_t qd = read_u16(buf + 4);
    const std::uint16_t an = read_u16(buf + 6);
    const std::uint16_t ns = read_u16(buf + 8);
    const std::uint16_t ar = read_u16(buf + 10);
    std::size_t pos = 12;

    // Skip the question section.
    std::string scratch;
    for (std::uint16_t i = 0; i < qd && pos < len; ++i) {
        pos = read_name(buf, len, pos, scratch);
        pos += 4;  // qtype + qclass
    }

    const std::uint32_t records = static_cast<std::uint32_t>(an) + ns + ar;
    for (std::uint32_t i = 0; i < records && pos < len; ++i) {
        std::string name;
        pos = read_name(buf, len, pos, name);
        if (pos + 10 > len) {
            break;
        }
        const std::uint16_t type = read_u16(buf + pos);
        const std::uint16_t rdlen = read_u16(buf + pos + 8);
        pos += 10;
        if (pos + rdlen > len) {
            break;
        }
        const std::uint8_t* rdata = buf + pos;

        switch (type) {
        case type_ptr: {
            std::string target;
            read_name(buf, len, pos, target);
            acc.instances.insert(target);
            break;
        }
        case type_srv: {
            if (rdlen >= 6) {
                const std::uint16_t port = read_u16(rdata + 4);
                std::string target;
                read_name(buf, len, pos + 6, target);
                acc.srv[name] = {target, port};
            }
            break;
        }
        case type_txt: {
            parse_txt(rdata, rdlen, acc.txt[name]);
            break;
        }
        case type_a: {
            if (rdlen == 4) {
                acc.ipv4[name] = std::to_string(rdata[0]) + "." + std::to_string(rdata[1]) + "." +
                                 std::to_string(rdata[2]) + "." + std::to_string(rdata[3]);
            }
            break;
        }
        case type_aaaa:
        default:
            break;
        }
        pos += rdlen;
    }
}

std::string instance_short_name(const std::string& instance) {
    const auto p = instance.find(service_suffix);
    return p == std::string::npos ? instance : instance.substr(0, p);
}

}  // namespace

std::vector<DiscoveredDevice> parse_mdns_packets(const std::vector<ByteBuffer>& packets) {
    Accumulator acc;
    for (const ByteBuffer& pkt : packets) {
        parse_packet(pkt.data(), pkt.size(), acc);
    }

    // Every instance we saw a PTR or SRV for is a candidate device.
    std::set<std::string> instances = acc.instances;
    for (const auto& [name, _] : acc.srv) {
        instances.insert(name);
    }

    std::vector<DiscoveredDevice> devices;
    for (const std::string& instance : instances) {
        if (instance.find("_esphomelib") == std::string::npos) {
            continue;
        }
        DiscoveredDevice dev;
        dev.name = instance_short_name(instance);

        if (const auto it = acc.srv.find(instance); it != acc.srv.end()) {
            dev.hostname = it->second.first;
            dev.port = it->second.second;
            if (const auto a = acc.ipv4.find(dev.hostname); a != acc.ipv4.end()) {
                dev.address = a->second;
            }
        }
        if (const auto it = acc.txt.find(instance); it != acc.txt.end()) {
            dev.properties = it->second;
            const auto get = [&](const char* k) -> std::string {
                const auto f = dev.properties.find(k);
                return f == dev.properties.end() ? std::string{} : f->second;
            };
            dev.mac = get("mac");
            dev.version = get("version");
            dev.friendly_name = get("friendly_name");
            dev.platform = get("platform");
            dev.board = get("board");
            dev.network = get("network");
            dev.project_name = get("project_name");
            dev.project_version = get("project_version");
            dev.requires_encryption = dev.properties.count("api_encryption") != 0;
            dev.supports_encryption =
                dev.requires_encryption || dev.properties.count("api_encryption_supported") != 0;
        }
        devices.push_back(std::move(dev));
    }
    return devices;
}

std::vector<DiscoveredDevice> Discovery::scan(const std::chrono::milliseconds timeout) {
    asio::io_context io;
    asio::ip::udp::socket sock(io);
    std::error_code ec;

    sock.open(asio::ip::udp::v4(), ec);
    if (ec) {
        return {};
    }
    sock.set_option(asio::socket_base::reuse_address(true), ec);
#ifdef SO_REUSEPORT
    {
        constexpr int one = 1;
        ::setsockopt(sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    }
#endif

    bool unicast = false;
    sock.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 5353), ec);
    if (ec) {
        // Port 5353 unavailable: fall back to an ephemeral port and request
        // unicast responses (QU bit).
        ec.clear();
        sock.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0), ec);
        if (ec) {
            return {};
        }
        unicast = true;
    }

    const auto group = asio::ip::make_address_v4("224.0.0.251");
    sock.set_option(asio::ip::multicast::join_group(group), ec);
    sock.set_option(asio::ip::multicast::enable_loopback(true), ec);

    // Build the DNS-SD PTR query for _esphomelib._tcp.local.
    ByteBuffer query;
    const auto u16 = [&](const std::uint16_t v) {
        query.push_back(static_cast<std::uint8_t>(v >> 8));
        query.push_back(static_cast<std::uint8_t>(v & 0xFF));
    };
    u16(0);  // id
    u16(0);  // flags (standard query)
    u16(1);  // qdcount
    u16(0);  // ancount
    u16(0);  // nscount
    u16(0);  // arcount
    for (const char* label : {"_esphomelib", "_tcp", "local"}) {
        const auto n = static_cast<std::uint8_t>(std::strlen(label));
        query.push_back(n);
        query.insert(query.end(), label, label + n);
    }
    query.push_back(0);
    u16(type_ptr);
    u16(unicast ? 0x8001 : 0x0001);  // QU bit + IN, or just IN

    sock.send_to(asio::buffer(query), asio::ip::udp::endpoint(group, 5353), 0, ec);

    std::vector<ByteBuffer> packets;
    std::array<std::uint8_t, 9000> rbuf{};
    asio::ip::udp::endpoint sender;
    std::function<void()> do_receive = [&]() {
        sock.async_receive_from(
            asio::buffer(rbuf), sender, [&](const std::error_code rec, const std::size_t n) {
                if (!rec && n > 0) {
                    packets.emplace_back(rbuf.begin(), rbuf.begin() + static_cast<long>(n));
                }
                if (!rec) {
                    do_receive();
                }
            });
    };
    do_receive();
    io.run_for(timeout);
    sock.close(ec);

    return parse_mdns_packets(packets);
}

}  // namespace esphome::api
