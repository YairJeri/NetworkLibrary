#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#if __has_include(<asio.hpp>)
    #define ASIO_STANDALONE
    #include <asio.hpp>
    #include <asio/ts/buffer.hpp>
    #include <asio/ts/internet.hpp>
#elif __has_include(<boost/asio.hpp>)
    #include <boost/asio.hpp>
    #include <boost/asio/ts/buffer.hpp>
    #include <boost/asio/ts/internet.hpp>
    namespace asio = boost::asio;
#else
    #error "Neither standalone asio nor boost::asio found"
#endif