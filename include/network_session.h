#pragma once
#include "network_core.h"
#include "network_package.h"

namespace network
{
    template <typename T>
    class connection;

    template<typename T>
    class session {
    public:
        session(uint32_t id)
            : client_id_(id), authenticated_(false)
        {}

        uint32_t GetID() const { return client_id_; }

        void ReassignTCPConnection(std::shared_ptr<connection<T>> new_conn) {
            tcp_conn_ = new_conn;
            last_active_ = std::chrono::steady_clock::now();
        }

        void SetUDP(asio::ip::udp::endpoint ep) {
            udp_endpoint_ = ep;
            last_active_ = std::chrono::steady_clock::now();
        }

        void SetAuthenticated(bool auth) {
            authenticated_ = auth;
        }

        bool IsAuthenticated() const {
            return authenticated_;
        }

        bool IsConnected() const {
            return tcp_conn_ && tcp_conn_->IsConnected();
        }

        std::optional<asio::ip::udp::endpoint> GetUDPEndpoint() const {
            return udp_endpoint_;
        }

        void SendTCP(const package<T>& pkg) {
            if (tcp_conn_ && tcp_conn_->IsConnected()) {
                tcp_conn_->Send(pkg);
            }
        }

        void SendUDP(asio::ip::udp::socket& udp_socket, const package<T>& pkg) {
            if (!udp_endpoint_) return;

            std::vector<uint8_t> buffer(sizeof(package_header<T>) + pkg.body.size());
            std::memcpy(buffer.data(), &pkg.header, sizeof(package_header<T>));
            std::memcpy(buffer.data() + sizeof(package_header<T>), pkg.body.data(), pkg.body.size());

            udp_socket.async_send_to(
                asio::buffer(buffer), *udp_endpoint_,
                [](std::error_code ec, std::size_t /*bytes*/) {
                    if (ec)
                        std::cerr << "[SERVER] UDP send failed: " << ec.message() << "\n";
                }
            );
        }

        std::shared_ptr<connection<T>> GetTCPConnection() const {
            return tcp_conn_;
        }

    private:
        uint32_t client_id_;
        std::shared_ptr<connection<T>> tcp_conn_;
        std::optional<asio::ip::udp::endpoint> udp_endpoint_;
        std::chrono::steady_clock::time_point last_active_;
        bool authenticated_;
    };
}