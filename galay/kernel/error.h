#ifndef GALAY_STATUS_H
#define GALAY_STATUS_H

namespace galay
{
    namespace Error{
        enum NoError{
            GY_SUCCESS,
            GY_NOERROR_END,
        };

        enum NetError{
            GY_SOCKET_ERROR = GY_NOERROR_END,   //socket error
            GY_SEND_ERROR,                      //send
            GY_RECV_ERROR,                      //recv
            GY_SSL_RECV_ERROR,                  //ssl_recv
            GY_SSL_SEND_ERROR,                  //ssl_send
            GY_BIND_ERROR,                      //bind
            GY_SETSOCKOPT_ERROR,                //setsockopt
            GY_LISTEN_ERROR,                    //listen
            GY_ACCEPT_ERROR,                    //accept
            GY_SSL_ACCEPT_ERROR,                //ssl_accept
            GY_SSL_CTX_INIT_ERROR,              //ssl_ctx_new
            GY_SSL_OBJ_INIT_ERROR,              //ssl_new
            GY_SSL_CRT_OR_KEY_FILE_ERROR,       //ssl certifaction and key file error
            GY_CONNECT_ERROR,                   //connect
            GY_SSL_CONNECT_ERROR,               //ssl_connect
            GY_GETSOCKET_STATUS_ERROR,          //getsocketopt error
            GY_SENDTO_ERROR,                    //sendto
            GY_RECVFROM_ERROR,                  //recvfrom
            GY_SET_NOBLOCK_ERROR,               //fcntl no block
            GY_NETERROR_END
        };

        enum ConfigError{
            GY_ENGINE_CHOOSE_ERROR = GY_NETERROR_END,             //engine choose error
            GY_INVALID_FUNCTIONS_ERROR,                           //func's size is incorrect 
            GY_CONFIGERROR_END
        };

        enum SchedulerError{
            GY_SCHEDULER_ENGINE_CHECK_ERROR = GY_CONFIGERROR_END,  //scheduler's engine check error
            GY_SCHDULER_EXPIRED_ERROR,
            GY_SCHDULER_MOD_EVENT_ERROR,                                   //mod event error
            GY_SCHEDULERERROR_END,
        };

        enum ProtocolError{
            GY_PROTOCOL_INCOMPLETE = GY_SCHEDULERERROR_END,                                    //incomplete package
            GY_PROTOCOL_BAD_REQUEST,                                  //bad request
            GY_PROTOCOL_ERROR_END
        };

        extern const char* global_err_str[];
        extern const char* get_err_str(int error);
    }
}

#endif