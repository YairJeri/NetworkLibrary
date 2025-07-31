#pragma once
#include "network_core.h"

namespace network
{
    enum class package_type : uint8_t {
        normal = 0,
        udp_auth = 1,
        server_assign_id = 2,
    };
    template <typename T>
    struct package_header
    {
        T id{};
        package_type type = package_type::normal;
        uint32_t size = 0;
    };

    template <typename T>
    struct package
    {
        package_header<T> header{};
        std::vector<uint8_t> body;

        size_t size() const
        {
				return body.size();
        }

        void reserve(size_t bytes)
        {
            body.reserve(bytes);
        }

        void reset()
        {
            body.clear();
            header.size = 0;
        }

        friend std::ostream& operator << (std::ostream& os, const package<T>& pkg)
        {
            os << "ID:" << int(pkg.header.id) << " Size:" << pkg.header.size;
            return os;
        }

        template<typename DataType>
        friend package<T>& operator << (package<T>& pkg, const DataType& data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex");
            
            size_t i = pkg.body.size();

            pkg.body.resize(i + sizeof(DataType));

            std::memcpy(pkg.body.data() + i, &data, sizeof(DataType));

            pkg.header.size = pkg.size();

            return pkg;
        }

        template<typename DataType>
        friend package<T>& operator >> (package<T>& pkg, DataType& data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex");
            
            size_t i = pkg.body.size() - sizeof(DataType);

            std::memcpy(&data, pkg.body.data() + i, sizeof(DataType));

            pkg.body.resize(i);

            pkg.header.size = pkg.size();

            return pkg;
        }
    };

    template <typename T>
    class connection;
    template<typename T>
    class session;

    /*
    enum class transport_type { TCP, UDP };    
    template <typename T>
    struct owned_package
    {
        transport_type type;
        std::shared_ptr<connection<T>> remote_tcp = nullptr;
        asio::ip::udp::endpoint remote_udp;
        package<T> pkg;
        
        friend std::ostream& operator << (std::ostream& os, const owned_package<T>& pkg)
        {
            os << pkg.pkg;
            return os;
        }
    };
    */
    enum class transport_type { TCP, UDP };
    template <typename T>
    struct owned_package
    {
        transport_type type;
        std::shared_ptr<session<T>> remote = nullptr;
        package<T> pkg;

        friend std::ostream& operator<<(std::ostream& os, const owned_package<T>& p)
        {
            os << p.pkg;
            return os;
        }
    };
}