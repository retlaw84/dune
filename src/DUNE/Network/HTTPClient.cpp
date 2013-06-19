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

// ISO C++ 98 headers.
#include <cstring>
#include <iostream>

// DUNE headers.
#include <DUNE/System/IOMultiplexing.hpp>
#include <DUNE/Network.hpp>

namespace DUNE
{
  namespace Network
  {
    HTTPClient::HTTPClient(const char* request, Address address, unsigned port) :
            m_bfr_len(0)
    {
      // FIXME: add exception handling
      m_socket.connect(address, port);
      m_socket.addToPoll(m_iom);
      m_socket.write(request, std::strlen(request));
    }

    void
    HTTPClient::getHeader(std::vector<std::string>& dst, double timeout)
    {
      std::string line;

      while (line != "\r\n")
      {
        line.clear();
        readLine(line, timeout);
        dst.push_back(line);
      }
    }

    void
    HTTPClient::getBody(std::vector<std::string>& dst, double timeout)
    {
      std::string line;

      while (true)
      {
        line.clear();
        readLine(line, timeout);
        dst.push_back(line);
      }
    }

    void
    HTTPClient::skipToBoundary(std::string boundary, double timeout)
    {
      std::string line;

      boundary = "--" + boundary + "\r\n";
      while (line != boundary)
      {
        line.clear();
        readLine(line, timeout);
      }
    }

    void
    HTTPClient::getBinary(char* bfr, size_t len, double timeout)
    {
      readBinary(bfr, len, timeout);
    }

    void
    HTTPClient::readLine(std::string& dst, double timeout)
    {
      bool done = false;
      size_t rdlen;

      while (!done)
      {
        // if pending buffer is empty, read data from socket
        if (m_bfr_len == 0)
        {
          if (!m_iom.poll(timeout))
            throw std::runtime_error("timed out while waiting for reply");

          // Read to buffer
          m_bfr_len = m_socket.read(m_bfr, c_bfrlen);
        }

        // Find the EOL character
        for (rdlen = 0; rdlen < m_bfr_len; rdlen++)
        {
          if (m_bfr[rdlen] == '\n')
          {
            done = true;
            // include the EOL character in the copy
            rdlen++;
            break;
          }
        }

        // copy data from the buffer to the line
        dst.append(m_bfr, rdlen);
        // remove parsed data from the buffer
        m_bfr_len -= rdlen;
        if (m_bfr_len)
          // move the remaining bytes to the start of the buffer
          std::memmove(m_bfr, m_bfr + rdlen, m_bfr_len);
      }
    }

    void
    HTTPClient::readBinary(char* bfr, size_t len, double timeout)
    {
      // read data from the pending buffer first
      size_t rdlen;

      if (len < m_bfr_len)
        rdlen = len;
      else
        rdlen = m_bfr_len;

      // memcpy
      std::memcpy(bfr, m_bfr, rdlen);

      // remove parsed data from the buffer
      m_bfr_len -= rdlen;
      if (m_bfr_len)
        // move the remaining bytes to the start of the buffer
        std::memmove(m_bfr, m_bfr + rdlen, m_bfr_len);

      // move forward
      len -= rdlen;
      bfr += rdlen;

      // read the rest of the data directly from the socket
      while (len > 0)
      {
        if (!m_iom.poll(timeout))
          throw std::runtime_error("timed out while waiting for reply");

        rdlen = m_socket.read(bfr, len);

        // move forward
        len -= rdlen;
        bfr += rdlen;
      }
    }
  }
}
