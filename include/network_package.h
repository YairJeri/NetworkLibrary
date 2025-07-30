#pragma once
#include "network_core.h"

namespace network
{
    template <typename T>
    struct package_header
    {
        T id{};
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
    };
}
