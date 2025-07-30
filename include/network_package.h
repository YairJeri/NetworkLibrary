#pragma once
#include "network_core.h"

namespace network
{
    template <typename T>
    struct package_header
    {
        T id{};
        uint32_t size = 0;
        void reserve(size_t bytes)
        {
            body.reserve(bytes);
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
    struct package
    {
        package_header<T> header{};
        std::vector<uint8_t> body;

        size_t size() const
        {
            return body.size();
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
        void reserve(size_t bytes)
        {
            body.reserve(bytes);
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
}
