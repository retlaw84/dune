//***************************************************************************
// Copyright 2007-2013 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Universidade do Porto. For licensing   *
// terms, conditions, and further information contact lsts@fe.up.pt.        *
//                                                                          *
// European Union Public Licence - EUPL v.1.1 Usage                         *
// Alternatively, this file may be used under the terms of the EUPL,        *
// Version 1.1 only (the "Licence"), appearing in the file LICENCE.md       *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://www.lsts.pt/dune/licence.                                        *
//***************************************************************************
// Author: Renato Caldas                                                    *
//***************************************************************************

#ifndef DUNE_NETWORK_HTTP_CLIENT_HPP_INCLUDED_
#define DUNE_NETWORK_HTTP_CLIENT_HPP_INCLUDED_

// ISO C++ 98 headers.
#include <cstring>
#include <iostream>

// DUNE headers.
#include <DUNE/Concurrency/TSQueue.hpp>
#include <DUNE/Concurrency/Thread.hpp>
#include <DUNE/Network/TCPSocket.hpp>
#include <DUNE/Network/HTTPRequestHandler.hpp>

namespace DUNE
{
  namespace Network
  {
    // Export DLL Symbol.
    class DUNE_DLL_SYM HTTPClient;

    class HTTPClient
    {
    public:
      HTTPClient(const char* request, Address address, unsigned port = 80);

      void
      getHeader(std::vector<std::string>& dst, double timeout = 2.0);

      void
      getBody(std::vector<std::string>& dst, double timeout = 2.0);

      void
      skipToBoundary(std::string boundary, double timeout = 2.0);

      void
      getBinary(char* bfr, size_t len, double timeout = 2.0);

    private:
      // Reception buffer length
      static const size_t c_bfrlen = 128;
      // TCP socket
      TCPSocket m_socket;
      // I/O multiplexing
      System::IOMultiplexing m_iom;
      // Incomming buffer
      char m_bfr[c_bfrlen];
      // Length of incomming buffer
      size_t m_bfr_len;

      void
      readLine(std::string& dst, double timeout = 2.0);

      void
      readBinary(char* bfr, size_t len, double timeout = 2.0);
    };
  }
}
#endif
