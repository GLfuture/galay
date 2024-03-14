#include "../galay/factory/factory.h"
#include <nlohmann/json.hpp>
#include <signal.h>
using namespace galay;

enum MODEL_TYPE
{
    MODEL_TYPE_BEG = 0,
    MODEL_LSTM = MODEL_TYPE_BEG,
    MODEL_GRU,
    MODEL_RNN,
    MODEL_TRANSFROM,
    MODEL_TYPE_END
};

enum MODEL_PARAM
{
    MODEL_PARAM_BEG = 0,
    MODEL_D_LAYERS = MODEL_PARAM_BEG,
    MODEL_DEC_IN,
    MODEL_E_LAYERS,
    MODEL_ENC_IN,
    MODEL_N_HEADERS,
    MODEL_PRED_LEN,
    MODEL_PARAM_END
};

enum MODEL_DATA
{
    MODEL_DATA_BEG = 0,
    LSTM_DATA1 = MODEL_DATA_BEG,
    LSTM_DATA2,
    LSTM_DATA3,
    GRU_DATA1,
    GRU_DATA2,
    GRU_DATA3,
    RNN_DATA1,
    RNN_DATA2,
    RNN_DATA3,
    TRANSFROM_DATA1,
    TRANSFROM_DATA2,
    TRANSFROM_DATA3,
    TRANSFROM_DATA4,
    TRANSFROM_DATA5,
    TRANSFROM_DATA6,
    MODEL_DATA_END
};

const char *global_model_type[4] = {
    "LSTM",
    "GRU",
    "RNN",
    "Transfrom"};

const char *global_model_params[6] = {
    "d_layers",
    "dec_in",
    "e_layers",
    "enc_in",
    "n_headers",
    "pred_len",
};

std::vector<std::vector<int>> global_data = {
    {1, 8, 2, 8, 8, 10},
    {4, 8, 2, 8, 8, 10},
    {4, 8, 4, 8, 16, 10},
    {1, 8, 2, 8, 8, 10},
    {2, 8, 4, 8, 8, 10},
    {4, 8, 4, 8, 8, 10},
    {1, 8, 2, 8, 8, 10},
    {4, 8, 4, 8, 8, 10},
    {1, 8, 4, 8, 8, 10},
    {1, 8, 2, 8, 8, 10},
    {1, 8, 2, 8, 8, 15},
    {1, 8, 3, 8, 8, 10},
    {1, 8, 2, 8, 16, 10},
    {1, 8, 4, 8, 8, 10},
    {1, 8, 4, 8, 16, 10},
};

const char* global_picture_id[] = {
    "p1",
    "p2",
    "p3",
    "p4",
    "p5",
    "p6"
};

Task<> func(Task_Base::wptr task)
{
    auto t_task = task.lock();
    auto req = std::dynamic_pointer_cast<protocol::Http1_1_Request>(t_task->get_req());
    auto resp = std::dynamic_pointer_cast<protocol::Http1_1_Response>(t_task->get_resp());

    if (req->get_method().compare("GET") == 0)
    {
        resp->get_status() = 200;
        resp->get_version() = "HTTP/1.1";
        resp->set_head_kv_pair({"Connection", "close"});
        nlohmann::json js;
        for (int i = 0; i < MODEL_DATA::MODEL_DATA_END; i++)
        {
            for (int j = 0; j < MODEL_PARAM::MODEL_PARAM_END; j++)
            {
                switch (i)
                {
                case MODEL_DATA::LSTM_DATA1... MODEL_DATA::LSTM_DATA3:
                {
                    js[global_model_type[MODEL_TYPE::MODEL_LSTM]][global_picture_id[i-LSTM_DATA1]][global_model_params[j]] = global_data[i][j];
                    break;
                }
                case MODEL_DATA::GRU_DATA1... MODEL_DATA::GRU_DATA3:
                {
                    js[global_model_type[MODEL_TYPE::MODEL_GRU]][global_picture_id[i-GRU_DATA1]][global_model_params[j]] = global_data[i][j];
                    break;
                }
                case MODEL_DATA::RNN_DATA1... MODEL_DATA::RNN_DATA3:
                {
                    js[global_model_type[MODEL_TYPE::MODEL_RNN]][global_picture_id[i-RNN_DATA1]][global_model_params[j]] = global_data[i][j];
                    break;
                }
                case MODEL_DATA::TRANSFROM_DATA1... MODEL_DATA::TRANSFROM_DATA6:
                {
                    js[global_model_type[MODEL_TYPE::MODEL_TRANSFROM]][global_picture_id[i-TRANSFROM_DATA1]][global_model_params[j]] = global_data[i][j];
                    break;
                }
                default:
                    break;
                }
            }
        }
        std::string body = js.dump();
        resp->set_head_kv_pair({"Content-Length", std::to_string(body.length())});
        resp->get_body() = body;
    }
    t_task->control_task_behavior(Task_Status::GY_TASK_WRITE);
    t_task->finish();
    return {};
}

HttpsServer https_server;

void sig_handle(int sig)
{
    https_server->stop();
}

int main()
{
    signal(SIGINT, sig_handle);
    Callback_ConnClose::set([](int fd){
        std::cout << "exit :" << fd << "\n";  
    });
    auto config = Config_Factory::create_https_server_config({8010,8011}, TLS1_2_VERSION, TLS1_3_VERSION, "../server.crt", "../server.key",Engine_Type::ENGINE_SELECT,5,5000);
    https_server = Server_Factory::create_https_server(config);
    https_server->start(func);
    return 0;
}