#include <iostream>
#include <network.h>

enum class CustomMsgTypes : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    PackageAll,
    ServerPackage,
};

class CustomServer : public network::server_interface<CustomMsgTypes>
{
public:
    CustomServer(uint16_t tcp_port, uint16_t udp_port) : network::server_interface<CustomMsgTypes>(tcp_port, udp_port)
    {

    }
protected:
    
        virtual bool OnClientConnect(std::shared_ptr<network::session<CustomMsgTypes>> client) 
        {
            network::package<CustomMsgTypes> pkg;
            pkg.header.id = CustomMsgTypes::ServerAccept;
            client->SendTCP(pkg); 
            return true; 
        }
        virtual void OnClientDisconnect(std::shared_ptr<network::session<CustomMsgTypes>> client) 
        {
		    std::cout << "Removing client [" << client->GetID() << "]\n";
        }
        virtual void OnPackage(std::shared_ptr<network::session<CustomMsgTypes>> client, network::package<CustomMsgTypes>& pkg) 
        {
            switch (pkg.header.id)                
            {
            case CustomMsgTypes::ServerPing:
            {
                std::cout << "[" << client->GetID() << "]: Server Ping\n";

                client->SendUDP(udp_socket_, pkg);

            }
            break;

            case CustomMsgTypes::PackageAll:
            {
                std::cout << "[" << client->GetID() << "]: Package All\n";

                // Construct a new message and send it to all clients
                network::package<CustomMsgTypes> pkg;
                pkg.header.id = CustomMsgTypes::PackageAll;
                pkg << client->GetID();
                // Broadcast to all except sender
                SendToAll(pkg, client->GetID());

            }
            break;
            }
        }
        virtual void OnUDPReceive(std::shared_ptr<network::session<CustomMsgTypes>> client, const asio::ip::udp::endpoint& from, network::package<CustomMsgTypes>& pkg) 
        {
            switch (pkg.header.id)
            {
            case CustomMsgTypes::ServerPing:
            {
                std::cout << "[" << client->GetUDPEndpoint()->address() << "]: UDP Server Ping\n";
                std::cout << "[]"<< client->GetID() << std::endl;
                client->SendUDP(udp_socket_, pkg);
                //SendPackageUDP(sender, pkg);
                //client->Send(pkg);

            }
            break;
            case CustomMsgTypes::ServerPackage:
            {
                std::cout << "[" << client->GetUDPEndpoint()->address() << "]: UDP Server\n";

            }
            break;
            }
        }

};

int main()
{
    CustomServer server(60000, 60001);
    server.Start();

    while(1)
    {
		server.Update(-1, true);
    }
    return 0;
}