// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _TCP_ACCEPTOR_SECURE_
#define _TCP_ACCEPTOR_SECURE_

#include <fastrtps/transport/TCPAcceptor.h>
#include <fastrtps/transport/TCPChannelResourceSecure.h>
#include <asio/ssl.hpp>

namespace eprosima{
namespace fastrtps{
namespace rtps{

class TCPAcceptorSecure : public TCPAcceptor
{
public:
    tcp_secure::eProsimaTCPSocket secure_socket_;

    /**
    * Constructor
    * @param io_service Reference to the ASIO service.
    * @param ssl_context Reference to the ASIO ssl context.
    * @param parent Pointer to the transport that is going to manage the acceptor.
    * @param locator Locator with the information about where to accept connections.
    */
    TCPAcceptorSecure(
        asio::io_service& io_service,
        asio::ssl::context& ssl_context,
        TCPTransportInterface* parent,
        const Locator_t& locator);

    /**
    * Constructor
    * @param io_service Reference to the ASIO service.
    * @param ssl_context Reference to the ASIO ssl context.
    * @param sInterface Network interface to bind the socket
    * @param locator Locator with the information about where to accept connections.
    */
    TCPAcceptorSecure(
        asio::io_service& io_service,
        asio::ssl::context& ssl_context,
        const std::string& interface,
        const Locator_t& locator);

    /**
    * Destructor
    */
    ~TCPAcceptorSecure()
    {
        try { asio::error_code ec; secure_socket_->lowest_layer().cancel(ec); }
        catch (...) {}
        secure_socket_->lowest_layer().close();
    }

    //! Method to start the accepting process.
    void Accept(TCPTransportInterface* parent, asio::io_service&, asio::ssl::context&);
};


} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // _TCP_ACCEPTOR_SECURE_
