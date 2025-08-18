#pragma once

#include "network_core.h"
#include "network_tsmap.h"
#include "network_package.h"
#include "network_connection.h"
#include "network_session.h"

namespace network
{
    template<typename T>
    class server_interface
    {
    public:
        server_interface(uint16_t tcp_port, uint16_t udp_port)
            : acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcp_port)),
              udp_socket_(io_context_, asio::ip::udp::endpoint(asio::ip::udp::v4(), udp_port))
        {}

        virtual ~server_interface() { Stop(); }

        bool Start()
        {
            try {
                WaitForClientConnection();
                StartUDPListener();
                context_thread_ = std::thread([this]() { io_context_.run(); });
            } catch (const std::exception& e) {
                std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                return false;
            }

            std::cout << "[SERVER] Started!\n";
            return true;
        }

        void Stop()
        {
            io_context_.stop();
            if (context_thread_.joinable()) context_thread_.join();
            std::cout << "[SERVER] Stopped!\n";
        }

        void WaitForClientConnection()
        {
            acceptor_.async_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::cout << "[SERVER] New TCP connection from " << socket.remote_endpoint() << "\n";

                        auto conn = std::make_shared<connection<T>>(connection<T>::owner::server, io_context_, std::move(socket), packages_in_);
                        auto sess = std::make_shared<session<T>>(client_id_counter_++);
                        sess->ReassignTCPConnection(conn);

                        conn->ConnectToClient(sess);

                        if (OnClientConnect(sess))
                        {
                            sessions_.set(sess->GetID(), sess);
                            std::cout << "[SERVER] Session " << sess->GetID() << " established\n";
                        
                            package<T> pkg;
                            pkg.header.type = package_type::server_assign_id;
                            pkg << sess->GetID();

                            sess->SendTCP(pkg);
                        }
                        else
                        {
                            std::cout << "[SERVER] Connection denied\n";
                            conn->Disconnect();
                        }
                    }
                    else
                    {
                        std::cerr << "[SERVER] Accept failed: " << ec.message() << "\n";
                    }

                    WaitForClientConnection();
                });
        }

        void StartUDPListener()
        {
            udp_socket_.async_receive_from(
                asio::buffer(udp_buffer_), remote_udp_endpoint_,
                [this](std::error_code ec, std::size_t bytes_received)
                {
                    if (!ec && bytes_received >= sizeof(package_header<T>))
                    {
                        package<T> pkg;
                        std::memcpy(&pkg.header, udp_buffer_.data(), sizeof(package_header<T>));

                        size_t body_size = bytes_received - sizeof(package_header<T>);
                        pkg.body.resize(body_size);
                        std::memcpy(pkg.body.data(), udp_buffer_.data() + sizeof(package_header<T>), body_size);

                        // --- Autenticación UDP ---
                        if (pkg.header.type == package_type::udp_auth)
                        {
                            if (body_size < sizeof(uint32_t)) {
                                std::cerr << "[SERVER] Malformed UDP auth packet\n";
                                StartUDPListener();
                                return;
                            }
                            
                            uint32_t client_id = 0;
                            std::memcpy(&client_id, pkg.body.data(), sizeof(uint32_t));

                            auto session_opt = sessions_.get(client_id);
                            if (session_opt)
                            {
                                auto& session = session_opt.value();
                                session->SetUDP(remote_udp_endpoint_);
                                std::cout << "[SERVER] UDP authenticated for client " << client_id << "\n";
                            }
                            else
                            {
                                std::cerr << "[SERVER] UDP Auth failed: unknown client ID " << client_id << "\n";
                            }
                        }
                        // --- Manejo normal UDP ---
                        else
                        {
                            std::shared_ptr<session<T>> matched_session = nullptr;

                            // Búsqueda lineal de endpoint registrado en mapa para encontrar por endpoint
                            for (uint32_t id = 10000; id < client_id_counter_; ++id)
                            {
                                auto opt_sess = sessions_.get(id);
                                if (!opt_sess) continue;

                                auto& sess = opt_sess.value();
                                if (sess->GetUDPEndpoint() && *(sess->GetUDPEndpoint()) == remote_udp_endpoint_)
                                {
                                    matched_session = sess;
                                    break;
                                }
                            }

                            if (matched_session)
                            {
                                packages_in_.push_back({ transport_type::UDP, matched_session, pkg });
                            }
                            else
                            {
                                std::cerr << "[SERVER] Unauthenticated UDP packet from " << remote_udp_endpoint_ << "\n";
                            }
                        }
                    }

                    StartUDPListener();
                });
        }


        void SendToClient(uint32_t client_id, const package<T>& pkg)
        {
            auto opt_session = sessions_.get(client_id);
            if (opt_session && opt_session.value()->tcp_conn && opt_session.value()->tcp_conn->IsConnected()) {
                opt_session.value()->SendTCP(pkg);
            }
        }


        void SendToAll(const package<T>& pkg, std::optional<uint32_t> ignore_id = std::nullopt)
        {
            std::vector<uint32_t> disconnected;

            for (uint32_t id = 10000; id < client_id_counter_; ++id)
            {
                auto opt_session = sessions_.get(id);
                if (!opt_session) continue;

                auto& session = opt_session.value();

                if (!session->GetTCPConnection() || !session->GetTCPConnection()->IsConnected())
                {
                    disconnected.push_back(id);
                    OnClientDisconnect(session);
                    continue;
                }

                if (ignore_id && id == ignore_id.value()) continue;

                session->SendTCP(pkg);
            }

            for (auto id : disconnected)
                sessions_.erase(id);
        }


        void Update(size_t max = -1, bool wait = false)
        {
            if (wait) packages_in_.wait();

            size_t count = 0;
            while (count < max && !packages_in_.empty()) {
                auto pkg = packages_in_.pop_front();

                if (!pkg.remote) {
                    std::cerr << "[SERVER] Received package from null session.\n";
                    continue;
                }

                switch (pkg.type) {
                    case transport_type::TCP:
                        OnPackage(pkg.remote, pkg.pkg);
                        break;
                    case transport_type::UDP:
                        OnUDPReceive(pkg.remote, *(pkg.remote->GetUDPEndpoint()), pkg.pkg);
                        break;
                }

                ++count;
            }
        }

    protected:
        virtual bool OnClientConnect(std::shared_ptr<session<T>> client) { return true; }
        virtual void OnClientDisconnect(std::shared_ptr<session<T>> client) {}
        virtual void OnPackage(std::shared_ptr<session<T>> client, package<T>& pkg) {}
        virtual void OnUDPReceive(std::shared_ptr<session<T>> client, const asio::ip::udp::endpoint& from, package<T>& pkg) {}

    protected:
        asio::io_context io_context_;
        std::thread context_thread_;
        asio::ip::tcp::acceptor acceptor_;
        asio::ip::udp::socket udp_socket_;
        asio::ip::udp::endpoint remote_udp_endpoint_;
        std::array<uint8_t, 65536> udp_buffer_;

        ThreadSafeMap<uint32_t, std::shared_ptr<session<T>>> sessions_;
        //std::deque<std::shared_ptr<session<T>>> sessions_;
        ThreadSafeQueue<owned_package<T>> packages_in_;
        std::atomic<uint32_t> client_id_counter_ = 10000;
    };
}