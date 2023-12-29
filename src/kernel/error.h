#ifndef GALAY_STATUS_H
#define GALAY_STATUS_H

namespace galay
{
    namespace error{
        enum base_error{
            GY_SUCCESS,
            GY_BASE_ERROR_END
        };
        enum server_error{
            GY_SOCKET_ERROR = GY_BASE_ERROR_END, //socket
            GY_BIND_ERROR,                      //bind
            GY_LISTEN_ERROR,                    //listen
            GY_CONNECT_ERROR,                   //connect
            GY_ACCEPT_ERROR,                    //accept
            GY_SEND_ERROR,                      //send
            GY_RECV_ERROR,                      //recv
            GY_SSL_CTX_INIT_ERROR,              //ssl_ctx_new
            GY_SSL_OBJ_INIT_ERROR,              //ssl_new
            GY_SSL_ACCEPT_ERROR,                //ssl_accept
            GY_SSL_CONNECT_ERROR,               //ssl_connect
            GY_SSL_RECV_ERROR,                  //ssl_recv
            GY_SSL_SEND_ERROR,                  //ssl_send
            GY_ENGINE_CHOOSE_ERROR,             //engine choose error
            GY_ENGINE_HAS_ERROR,                //engine has error ,need to call engine's get_error interface
            GY_SERVER_END
        };

        enum engine_error{
            GY_ENGINE_EPOLL_WAIT_ERROR = server_error::GY_SERVER_END,                //epoll egine epoll_wait error
            GY_ENGINE_ERROR_END
        };

        enum protocol_error{
            GY_PROTOCOL_INCOMPLETE = engine_error::GY_ENGINE_ERROR_END,             //incomplete package
            GY_PROTOCOL_BAD_REQUEST,                                                   //bad request
        };
    }
}

#endif