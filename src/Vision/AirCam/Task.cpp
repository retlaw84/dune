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
// Author: João Fortuna                                                     *
//***************************************************************************
// Aquires snapshots from Ubiquiti's AirCam IP cameras

// ISO C++ 98 headers.
#include <cstring>

// DUNE headers.
#include <DUNE/DUNE.hpp>

namespace Vision
{
  namespace AirCam
  {
    using DUNE_NAMESPACES;

    //! %Task arguments.
    struct Arguments
    {
      //! Camera IP address.
      Address address;
    };

    //! Device driver task.
    struct Task: public DUNE::Tasks::Task
    {
      TCPSocket* m_TCP_sock;
      System::IOMultiplexing m_iom;
      uint8_t m_buf[5120];
      //! Configuration parameters
      Arguments m_args;
      //! Video stream HTTP connection
      HTTPClient* m_http;
      //! Destination log folder.
      Path m_log_dir;
      //! Current destination volume.
      Path m_volume;
      //! Current number of volumes.
      unsigned m_volume_count;
      //! Number of files in current volume.
      unsigned m_file_count;
      // Timestamp for last frame
      double m_timestamp;
      //! True if task is activating.
      bool m_activating;

      Task(const std::string& name, Tasks::Context& ctx):
        Tasks::Task(name, ctx),
        m_log_dir(ctx.dir_log),
        m_volume_count(0),
        m_file_count(0),
        m_activating(false)
      {
        // Retrieve configuration values.
        paramActive(Tasks::Parameter::SCOPE_MANEUVER,
                    Tasks::Parameter::VISIBILITY_USER);

        param("IP Address", m_args.address)
        .defaultValue("10.0.20.209")
        .description("IPv4 address of the camera");

        bind<IMC::LoggingControl>(this);
      }

      void
      consume(const IMC::LoggingControl* msg)
      {
        if (!m_activating && (msg->getDestination() != getSystemId()))
          return;

        if (msg->op == IMC::LoggingControl::COP_CURRENT_NAME)
        {
          m_log_dir = m_ctx.dir_log / msg->name / "Photos";
          activate();
        }
      }

      void
      onRequestActivation(void)
      {
        m_activating = true;
        IMC::LoggingControl log_ctl;
        log_ctl.op = IMC::LoggingControl::COP_REQUEST_CURRENT_NAME;
        dispatch(log_ctl);
      }

      void
      onActivation(void)
      {
        m_activating = false;
        m_file_count = 0;
        m_volume_count = 0;
        setEntityState(IMC::EntityState::ESTA_NORMAL, Status::CODE_ACTIVE);
      }

      void
      onDeactivation(void)
      {
        setEntityState(IMC::EntityState::ESTA_NORMAL, Status::CODE_IDLE);
      }

      ~Task(void)
      {
        Task::onResourceRelease();
      }

      void
      onResourceRelease(void)
      {
        Memory::clear(m_TCP_sock);
      }

      void
      onResourceAcquisition(void)
      {
        openConnection();
      }

      void
      openConnection(void)
      {
        try
        {
          m_TCP_sock = new TCPSocket;
          m_TCP_sock->connect(m_args.address, 80);
          m_TCP_sock->addToPoll(m_iom);
          const char cmd[] = "GET /snapshot.cgi HTTP/1.1\r\nUser-Agent: airVision/1.1.3\r\nHost: 10.0.20.209\r\n\r\n";

          m_TCP_sock->write(cmd, std::strlen(cmd));
        }
        catch (...)
        {
          m_TCP_sock = 0;
          war("Connection failed, retrying...");
          setEntityState(IMC::EntityState::ESTA_NORMAL, Status::CODE_COM_ERROR);
        }
      }

      bool
      poll(double timeout)
      {
        if(m_TCP_sock)
        {
          if (m_iom.poll(timeout))
            return m_TCP_sock->wasTriggered(m_iom);
          else
            return false;
        }
        return false;
      }

      int
      receiveData(uint8_t* buf, size_t blen)
      {
        if(m_TCP_sock)
        {
          try
          {
            return m_TCP_sock->read((char*) buf, blen);
          }
          catch (...)
          {
            war("Connection lost, retrying...");
            m_TCP_sock->delFromPoll(m_iom);
            delete m_TCP_sock;
            openConnection();
            return 0;
          }
        }
        return 0;
      }

      void
      handleData(void)
      {
        while (poll(0.01))
        {
          int n = receiveData(m_buf, sizeof(m_buf));
          if (n <= 0)
          {
            err(DTR("receive error"));
            break;
          }
          else
          {
            inf("Bytes read: %d", n);
            for(int i = 0; i < n; i++)
            {
              std::cout << m_buf[i];
            }
            std::cout << std::endl;
          }
        }
      }


      void
      changeVolume(void)
      {
        m_volume = m_log_dir / String::str("%06u", m_volume_count);
        m_volume.create();
        ++m_volume_count;
      }

      void
      onMain(void)
      {
//        ByteBuffer dst;

        while (!stopping())
        {
//          if (isActive())
//          {
//            consumeMessages();
//          }
//          else
//          {
//            waitForMessages(1.0);
//            continue;
//          }

          if(m_TCP_sock)
          {
            handleData();
          }
          else
          {
            Time::Delay::wait(0.5);
            openConnection();
          }



//          // Save file
//          double timestamp = Clock::getSinceEpoch();
//          Path file = m_volume / String::str("%0.4f.jpg", timestamp);
//          std::ofstream jpg(file.c_str(), std::ios::binary);
//          jpg.write(dst.getBufferSigned(), dst.getSize());
        }
      }
    };
  }
}

DUNE_TASK
