#pragma once

#include "network_core.h"
#include "network_package.h"
#include "network_tsqueue.h"
#include "network_connection.h"

namespace network
{
    template <typename T>
    class client_interface
    {
    public:
        client_interface()
            : udp_socket_(io_context_)
        {}

        virtual ~client_interface()
        {
            Disconnect();
        }

    public:
        bool Connect(const std::string& host, const uint16_t port_tcp, const uint16_t port_udp)
        {
            try
            {
                asio::ip::tcp::resolver resolver_tcp(io_context_);
                asio::ip::udp::resolver resolver_udp(io_context_);

                asio::ip::tcp::resolver::results_type endpoints_tcp = resolver_tcp.resolve(host, std::to_string(port_tcp));
                asio::ip::udp::resolver::results_type endpoints_udp = resolver_udp.resolve(host, std::to_string(port_udp));

                connection_ = std::make_unique<connection<T>>(connection<T>::owner::client, io_context_, asio::ip::tcp::socket(io_context_), packages_in_);
                connection_->ConnectToServer(endpoints_tcp);

                udp_socket_.open(asio::ip::udp::v4());
                // StartUDPReceive();

                server_udp_endpoint_ = *endpoints_udp.begin();

                context_thread_ = std::thread([this]() { io_context_.run(); });
            }
            catch (std::exception& e)
            {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }
            return true;
        }

        void StartUDPReceive()
        {
            udp_socket_.async_receive_from(
                asio::buffer(udp_buffer_), server_udp_endpoint_,
                [this](std::error_code ec, std::size_t bytes_recvd)
                {
                    if (!ec && bytes_recvd >= sizeof(package_header<T>))
                    {
                        package<T> pkg;

                        std::memcpy(&pkg.header, udp_buffer_.data(), sizeof(package_header<T>));
                        pkg.body.resize(pkg.header.size);
                        std::memcpy(pkg.body.data(), udp_buffer_.data() + sizeof(package_header<T>), pkg.header.size);

                        owned_package<T> opkg;
                        opkg.type = transport_type::UDP;
                        opkg.pkg = std::move(pkg);
                        packages_in_.push_back(std::move(opkg));
                    }
                    else
                    {
                        std::cerr << "[CLIENT] UDP Receive Error: " << ec.message() << "\n";
                    }

                    StartUDPReceive(); // Loop continuo
                }
            );
        }


        void Disconnect()
        {
            if(IsConnected())
            {
                connection_->Disconnect();
            }
			
            io_context_.stop();

            if (context_thread_.joinable())
                context_thread_.join();

            connection_.release();
        }

        bool IsConnected()
        {
            if (connection_)
                return connection_->IsConnected();
            else
                return false;
        }

    public:
        void Send(const package<T>& pkg)
        {
            if (IsConnected())
                    connection_->Send(pkg);
        }

        void SendUDP(const package<T>& pkg)
        {
            std::vector<uint8_t> buffer;
            size_t header_size = sizeof(package_header<T>);
            buffer.resize(header_size + pkg.body.size());

            std::memcpy(buffer.data(), &pkg.header, header_size);
            std::memcpy(buffer.data() + header_size, pkg.body.data(), pkg.body.size());

            udp_socket_.async_send_to(
                asio::buffer(buffer), server_udp_endpoint_,
                [](std::error_code ec, std::size_t bytes_sent)
                {
                    if (ec)
                        std::cerr << "[CLIENT] UDP Send Error: " << ec.message() << "\n";
                });
        }

        void HandleServerAssignID(const package<T>& pkg)
        {
            if (pkg.body.size() < sizeof(uint32_t)) return;

            std::memcpy(&client_id_, pkg.body.data(), sizeof(uint32_t));
            std::cout << "[CLIENT] Assigned client_id: " << client_id_ << "\n";

            package<T> auth;
            auth.header.type = package_type::udp_auth;
            auth << client_id_;
            SendUDP(auth);
        }


        ThreadSafeQueue<owned_package<T>>& Incoming()
        { 
            return packages_in_;
        }

    protected:
        asio::io_context io_context_;
        std::thread context_thread_;
        std::unique_ptr<connection<T>> connection_;

        asio::ip::udp::socket udp_socket_;
        asio::ip::udp::endpoint server_udp_endpoint_;
        std::array<uint8_t, 65536> udp_buffer_;

    private:
        ThreadSafeQueue<owned_package<T>> packages_in_;
        uint32_t client_id_ = 0;
    };
}