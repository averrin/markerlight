#include <msgpack.hpp>
#include <opendht.h>
#include <opendht/rng.h>
#include <opendht/infohash.h>
#include <vector>

#include "rang.hpp"
using namespace rang;

static std::mt19937_64 rd {dht::crypto::random_device{}()};
static std::uniform_int_distribution<dht::Value::Id> rand_id;
static std::uniform_int_distribution<int> uni(1, 100);

struct AgentInfo {
    std::string agent_id;
    std::string agent_pkey;
    std::string host;
    int port;

    // implements msgpack-c serialization methods
    MSGPACK_DEFINE(agent_id, agent_pkey, host, port);
};

int main()
{
    dht::DhtRunner node;

    auto creds = dht::crypto::generateIdentity();
    auto port = 10000 + uni(rd);
    node.run(port, creds, true);
    node.bootstrap("bootstrap.ring.cx", "4222");

    const dht::InfoHash myid = node.getId();
    const dht::InfoHash myNodeid = node.getNodeId();
    std::cout << "My node port: " << fg::green << port << style::reset << std::endl;
    std::cout << "My node id: " << fg::green << myNodeid << style::reset << std::endl;
    std::cout << "My public key: " << fg::green << myid << style::reset << std::endl;

    AgentInfo info {myNodeid.toString(), myid.toString(), "localhost", port};

    auto channel = "markerlight-agents";
    auto room = dht::InfoHash(channel);
    if (not room) {
        room = dht::InfoHash::get(channel);
        std::cout << "Joining h(" << channel << ") = " << room << std::endl;
    }

    node.listen<AgentInfo>(room, [&](AgentInfo&& msg/*, bool expired*/) {
        if (msg.agent_pkey != myid.toString()) {
            // if (expired) {
            //     std::cout << "expired" << std::endl;
            // }
            std::cout << msg.agent_pkey << " (" << msg.host << ":" << msg.port << ")" << std::endl;
        }
        return true;
    });
    
    // auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    node.putSigned(room, info, [](bool ok) {
        if (not ok)
            std::cout << "Message publishing failed !" << std::endl;
    });

    std::cout << std::endl <<  "Serving..." << std::endl;
    while(true) {}

    // wait for dht threads to end
    std::cout << std::endl <<  "Stopping node..." << std::endl;
    node.join();
    return 0;
}
