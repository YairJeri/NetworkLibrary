#pragma once

#include "network_core.h"
#include "network_tsqueue.h"
#include "network_package.h"
/*
namespace network
{
    template<typename T>
		class connection
		{
		public:
			enum class owner
			{
				server,
				client
			};

			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, ThreadSafeQueue<owned_package<T>>& qIn)
				: io_context_(asioContext), connection_socket_(std::move(socket)), packages_in_(qIn)
			{
				owner_type_ = parent;
			}

			std::shared_ptr<connection<T>> self_ = nullptr;

			virtual ~connection()
			{}

			uint32_t GetID() const
			{
				return id;
			}

			void ConnectToClient(uint32_t uid = 0)
			{
				if (owner_type_ == owner::server)
				{
					if (connection_socket_.is_open())
					{
						id = uid;
						ReadHeader();
					}
				}
			}

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				// Only clients can connect to servers
				if (owner_type_ == owner::client)
				{
					// Request asio attempts to connect to an endpoint
					asio::async_connect(connection_socket_, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
						{
							if (!ec)
							{
								ReadHeader();
							}
						});
				}
			}

			void Disconnect()
			{
				if (IsConnected())
					asio::post(io_context_, [this]() { connection_socket_.close(); });
			}

			bool IsConnected() const
			{
				return connection_socket_.is_open();
			}

			void StartListening()
			{
				
			}

			void Send(const package<T>& pkg)
			{
				asio::post(io_context_,
					[this, pkg]()
					{
						bool bWritingPackage = !packages_out_.empty();
						packages_out_.push_back(pkg);
						if (!bWritingPackage)
						{
							WriteHeader();
						}
					});
			}

		private:
			void WriteHeader()
			{
				asio::async_write(connection_socket_, asio::buffer(&packages_out_.front().header, sizeof(package_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (packages_out_.front().body.size() > 0)
							{
								WriteBody();
							}
							else
							{
								packages_out_.pop_front();
								if (!packages_out_.empty())
								{
									WriteHeader();
								}
							}
						}
						else
						{
							std::cout << "[" << id << "] Write Header Fail.\n";
							connection_socket_.close();
						}
					});
			}

			void WriteBody()
			{
				asio::async_write(connection_socket_, asio::buffer(packages_out_.front().body.data(), packages_out_.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (ec)
						{							
							std::cout << "[" << id << "] Write Body Fail.\n";
							connection_socket_.close();
							return;
						}
						packages_out_.pop_front();

						if (!packages_out_.empty())
						{
							WriteHeader();
						}
					});
			}

			void ReadHeader()
			{
				asio::async_read(connection_socket_, asio::buffer(&temporary_package_in_.header, sizeof(package_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if(ec)
						{
							std::cout << "[" << id << "] Read Header Fail.\n";
							connection_socket_.close();
							return;
						}

						if(temporary_package_in_.header.size > 0)
						{
							temporary_package_in_.body.resize(temporary_package_in_.header.size);
							ReadBody();
							return;
						}

						AddToIncomingPackageQueue();
					});
			}

			void ReadBody()
			{
				asio::async_read(connection_socket_, asio::buffer(temporary_package_in_.body.data(), temporary_package_in_.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (ec)
						{
							std::cout << "[" << id << "] Read Body Fail.\n";
							connection_socket_.close();
							return;
						}

						AddToIncomingPackageQueue();
					});
			}

			void AddToIncomingPackageQueue()
			{
				if(!self_){
					std::cout << "vacioooo"<< std::endl;
				} else {
					std::cout << "Servidor" << std::endl;
				}
				packages_in_.push_back({ transport_type::TCP, self_, {}, temporary_package_in_ });
				ReadHeader();
			}

			uint64_t scramble(uint64_t nInput)
			{
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
				return out ^ 0xC0DEFACE12345678;
			}

		protected:
			asio::ip::tcp::socket connection_socket_;

			asio::io_context& io_context_;

			ThreadSafeQueue<package<T>> packages_out_;

			ThreadSafeQueue<owned_package<T>>& packages_in_;

			package<T> temporary_package_in_;

			owner owner_type_ = owner::server;
		};
}

*/
namespace network
{
    template<typename T>
    // forward declarations

    template<typename T>
    class connection
    {
    public:
        enum class owner { server, client };

        connection(owner parent, asio::io_context& asioContext,
                   asio::ip::tcp::socket socket,
                   ThreadSafeQueue<owned_package<T>>& qIn)
            : io_context_(asioContext),
              connection_socket_(std::move(socket)),
              packages_in_(qIn),
              owner_type_(parent)
        {}

        virtual ~connection() {}

        void ConnectToClient(std::shared_ptr<session<T>> session)
        {
            SetOwner(session);
            if (connection_socket_.is_open()) {
                ReadHeader();
            }
        }

        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
        {
            if (owner_type_ == owner::client)
            {
                asio::async_connect(connection_socket_, endpoints,
                    [this](std::error_code ec, asio::ip::tcp::endpoint)
                    {
                        if (!ec)
                        {
                            ReadHeader();
                        }
                    });
            }
        }

        void Disconnect()
        {
            if (IsConnected()) {
                asio::post(io_context_, [this]() { connection_socket_.close(); });
            }
        }

        bool IsConnected() const
        {
            return connection_socket_.is_open();
        }

        void Send(const package<T>& pkg)
        {
            asio::post(io_context_,
                [this, pkg]()
                {
                    bool writing = !packages_out_.empty();
                    packages_out_.push_back(pkg);
                    if (!writing) {
                        WriteHeader();
                    }
                });
        }

        void SetOwner(std::shared_ptr<session<T>> session)
        {
            owner_session_ = session;
        }

    private:
        void WriteHeader()
        {
            asio::async_write(connection_socket_,
                asio::buffer(&packages_out_.front().header, sizeof(package_header<T>)),
                [this](std::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        if (packages_out_.front().body.size() > 0)
                        {
                            WriteBody();
                        }
                        else
                        {
                            packages_out_.pop_front();
                            if (!packages_out_.empty()) {
                                WriteHeader();
                            }
                        }
                    }
                    else
                    {
                        std::cout << "[TCP] Write Header Fail.\n";
                        connection_socket_.close();
                    }
                });
        }

        void WriteBody()
        {
            asio::async_write(connection_socket_,
                asio::buffer(packages_out_.front().body.data(), packages_out_.front().body.size()),
                [this](std::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        packages_out_.pop_front();
                        if (!packages_out_.empty()) {
                            WriteHeader();
                        }
                    }
                    else
                    {
                        std::cout << "[TCP] Write Body Fail.\n";
                        connection_socket_.close();
                    }
                });
        }

        void ReadHeader()
        {
            asio::async_read(connection_socket_,
                asio::buffer(&temporary_package_in_.header, sizeof(package_header<T>)),
                [this](std::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        if (temporary_package_in_.header.size > 0)
                        {
                            temporary_package_in_.body.resize(temporary_package_in_.header.size);
                            ReadBody();
                        }
                        else
                        {
                            AddToIncomingPackageQueue();
                        }
                    }
                    else
                    {
                        std::cout << "[TCP] Read Header Fail.\n";
                        connection_socket_.close();
                    }
                });
        }

        void ReadBody()
        {
            asio::async_read(connection_socket_,
                asio::buffer(temporary_package_in_.body.data(), temporary_package_in_.body.size()),
                [this](std::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        AddToIncomingPackageQueue();
                    }
                    else
                    {
                        std::cout << "[TCP] Read Body Fail.\n";
                        connection_socket_.close();
                    }
                });
        }

        void AddToIncomingPackageQueue()
        {
            if (owner_type_ == owner::server) {
				if (auto session = owner_session_.lock()) {
					packages_in_.push_back({ transport_type::TCP, session, temporary_package_in_ });
				} else {
					std::cerr << "[TCP] Session expired while queuing package.\n";
				}
			} else {
				packages_in_.push_back({ transport_type::TCP, nullptr, temporary_package_in_ });
			}


            ReadHeader();
        }

    protected:
        asio::ip::tcp::socket connection_socket_;
        asio::io_context& io_context_;
        ThreadSafeQueue<package<T>> packages_out_;
        ThreadSafeQueue<owned_package<T>>& packages_in_;
        package<T> temporary_package_in_;

        owner owner_type_ = owner::server;

        // Future session back-ref
        uint32_t client_id_ = 0;
    };
}
