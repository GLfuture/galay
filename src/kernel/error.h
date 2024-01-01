#ifndef GALAY_STATUS_H
#define GALAY_STATUS_H

namespace galay
{
    namespace error{
        enum base_error{
            GY_SUCCESS,
            GY_SOCKET_ERROR,
            GY_SEND_ERROR,                      //send
            GY_RECV_ERROR,                      //recv
            GY_SSL_RECV_ERROR,                  //ssl_recv
            GY_SSL_SEND_ERROR,                  //ssl_send
            GY_BASE_ERROR_END
        };

        enum server_error{
            GY_BIND_ERROR = GY_BASE_ERROR_END,  //bind
            GY_SETSOCKOPT_ERROR,                //setsockopt
            GY_LISTEN_ERROR,                    //listen
            GY_ACCEPT_ERROR,                    //accept
            GY_SSL_ACCEPT_ERROR,                //ssl_accept
            GY_SSL_CTX_INIT_ERROR,              //ssl_ctx_new
            GY_SSL_OBJ_INIT_ERROR,              //ssl_new
            GY_SSL_CRT_OR_KEY_FILE_ERROR,       //ssl certifaction and key file error
            GY_ENGINE_CHOOSE_ERROR,             //engine choose error
            GY_ENGINE_HAS_ERROR,                //engine has error ,need to call engine's get_error interface
            GY_SERVER_ERROR_END
        };

        enum client_error{
            GY_CONNECT_ERROR = GY_SERVER_ERROR_END,                     //connect
            GY_SSL_CONNECT_ERROR,                                       //ssl_connect
            GY_CLIENT_ERROR_RND
        };

        enum engine_error{
            GY_ENGINE_EPOLL_WAIT_ERROR = GY_CLIENT_ERROR_RND,                //epoll egine epoll_wait error
            GY_ENGINE_ERROR_END
        };

        enum protocol_error{
            GY_PROTOCOL_INCOMPLETE = GY_ENGINE_ERROR_END,             //incomplete package
            GY_PROTOCOL_BAD_REQUEST,                                  //bad request
            GY_PROTOCOL_ERROR_END
        };

        extern const char* global_err_str[];
        extern const char* get_err_str(int error);
    }
}

#endif