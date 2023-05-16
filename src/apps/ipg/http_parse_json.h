// #include "json/json.h"
// #include "core/util/logger.h"
// #include <json-c/json.h>
// #include "httplib.h"
// #include "app_base.h"
// #include "parse_json.h"
// #include <fstream>
// using namespace Json;
// using namespace std;
// using namespace httplib;

// #ifndef _ST_APP_HTTP_PARSE_JSON_HEAD_H_
// #define _ST_APP_HTTP_PARSE_JSON_HEAD_H_

// int st_app_parse_json_object_add_tx(st_json_context_t* ctx, json_object tx_group_array,json_object root_object,int count);
// int st_app_parse_json_object_update_tx(st_json_context_t* ctx, json_object tx_group_array,json_object root_object);


// int st_app_parse_json_object_add_rx(st_json_context_t* ctx, json_object rx_group_array,json_object root_object,int count);
// int st_app_parse_json_object_update_rx(st_json_context_t* ctx, json_object rx_group_array,json_object root_object);


// #endif


// void do()
// {
//   httplib::Server server;
//   /*get JSON file*/
//    server.Get("/api/json",[&](const Request &req, Response &res)
//                { 
//                 std::string str;
//                 Json::FastWriter write;
//                 Value root;
//                 Reader r;
//                 Server server;
//                 ifstream ifs("tes_conf.json");
//                 r.parse(ifs, root);
//                 ifs.close();
//                 res.body = write.write(root);
//                 res.status = 200;
//                 res.set_content(res.body, "application/json");
//                });


//     /*add_tx_session*/
//     server.Post("/api/add_tx_json",[&](const Request &req, Response &res)
//           {
//                 int ret;
//                 ifstream ifs("tes_conf.json");
//                 Reader r;
//                 Json::FastWriter write;
//                 Value root;
//                 r.parse(ifs, root);
//                 ifs.close(); 
//                 string str = req.body;
//                 Value add_tx;
//                 Value root_tx_arr = root["tx_sessions"];

//                 int count = root_tx_arr.size();
//                 Json::Reader reader;
//                 reader.parse(str,add_tx);
                
//                 Value add_tx_array =  add_tx["tx_sessions"];
//                 for (size_t i = 0; i < add_tx_array.size(); i++)
//                 {
//                     count = count + i;
//                     Value add_tx_obj = add_tx_array[i];
//                     add_tx_obj["id"] = count;
//                     root_tx_arr[count] = add_tx_obj;
//                 }

//                 root["tx_sessions"] = root_tx_arr;
//                 res.body = write.write(root);

//                 string end = root.toStyledString();
//                 ofstream ofs("tes_conf.json");
//                 ofs << end;
//                 ofs.close();
//                 res.status = 200;
//                 res.set_content(res.body, "application/json");
                
//                 st_json_context_t * ctx_add = (st_json_context_t*)st_app_zmalloc(sizeof(st_json_context_t));
//                 ret = st_app_parse_json_object_add_tx(ctx_add, add_tx_array,root,count);
//                 ret = st_tx_video_source_init_add(ctx,ctx_add);
//                 ret = st_app_tx_video_sessions_init_add(ctx,ctx_add);
//                 ret = st_app_tx_audio_sessions_init_add(ctx,ctx_add);

//           });


//    /*update_tx_session*/      
//    server.Post("/api/update_tx_json",[&](const Request &req, Response &res)
//           {
//               int ret;
//               ifstream ifs("tes_conf.json");
//               Reader r;
//               Json::FastWriter write;
//               Value root;
//               r.parse(ifs, root);
//               ifs.close(); 
//               string str = req.body;
//               Value update_tx,update_tx_obj;
//               Value root_tx_arr = root["tx_sessions"];

//               int  count = root_tx_arr.size();
//               Json::Reader reader;
//               reader.parse(str,update_tx);
              
//               Value update_tx_array =  update_tx["tx_sessions"];
//               for (size_t i = 0; i < update_tx_array.size(); i++)
//               {
//                   update_tx_obj = update_tx_array[i];
//                   int update_id = update_tx_obj["id"].asInt();

//                   for (size_t j = 0; j < root_tx_arr.size(); j++)
//                   {
//                     int root_id = root_tx_arr[j]["id"].asInt();
//                     if ( update_id == root_id ){
//                         root_tx_arr[j] = update_tx_obj;
//                     }
//                   }
//               }
//               root["tx_sessions"] = root_tx_arr;
//               res.body = write.write(root);
//               string end = root.toStyledString();
//               ofstream ofs("tes_conf.json");
//               ofs << end;
//               ofs.close();
//               res.status = 200;
//               res.set_content(res.body, "application/json"); 

//             st_json_context_t * ctx_update = (st_json_context_t*)st_app_zmalloc(sizeof(st_json_context_t));
//             ret = st_app_parse_json_object_update_tx(ctx_update,update_tx_array,root);

//             for (size_t i = 0; i < update_tx_array.size(); i++)
//             {
//             Value update_tx_obj = update_tx_array[i];
//             int id = update_tx_obj["id"];
//             ret = st_tx_video_source_init_update( ctx,ctx_update,id);
//             ret = st_app_tx_video_sessions_uinit_update(ctx_update,id,ctx_update);
//             ret = st_app_tx_audio_sessions_uinit_update(ctx_update,id,ctx_update); 
//             } 
//           });    



//    /*add_rx_session*/
//     server.Post("/api/add_rx_json",[&](const Request &req, Response &res)
//           {
//                 int ret;
//                 ifstream ifs("tes_conf.json");
//                 Reader r;
//                 Json::FastWriter write;
//                 Value root;
//                 r.parse(ifs, root);
//                 ifs.close(); 
//                 string str = req.body;
//                 Value add_rx;
//                 Value root_rx_arr = root["rx_sessions"];

//                 int  count = root_rx_arr.size();
//                 Json::Reader reader;
//                 reader.parse(str,add_rx);
                
//                 Value add_rx_array =  add_rx["rx_sessions"];
//                 for (size_t i = 0; i < add_rx_array.size(); i++)
//                 {
//                     count = count + i;
//                     Value add_rx_obj = add_rx_array[i];
//                     add_rx_obj["id"] = count;
//                     root_rx_arr[count] = add_rx_obj;
//                 }
 
//                 root["rx_sessions"] = root_rx_arr;
//                 res.body = write.write(root);

//                 string end = root.toStyledString();
//                 ofstream ofs("tes_conf.json");
//                 ofs << end;
//                 ofs.close();
//                 res.status = 200;
//                 res.set_content(res.body, "application/json");
                
//                 st_json_context_t * ctx_add = (st_json_context_t*)st_app_zmalloc(sizeof(st_json_context_t));
//                 ret = st_app_parse_json_object_add_rx(ctx_add, add_rx_array,root,count);
//                 ret = st_rx_output_init_add(ctx,ctx_add);
//                 ret = st_app_rx_video_sessions_init_add(ctx,ctx_add);
//                 ret = st_app_rx_audio_sessions_init_add(ctx,ctx_add);
//           });



//   /*update_rx_session*/
//   server.Post("/api/update_rx_json",[&](const Request &req, Response &res)
//           {
//               int ret;
//               ifstream ifs("tes_conf.json");
//               Reader r;
//               Json::FastWriter write;
//               Value root;
//               r.parse(ifs, root);
//               ifs.close(); 
//               string str = req.body;
//               Value update_rx,update_rx_obj;
//               Value root_rx_arr = root["rx_sessions"];

//               int count = root_rx_arr.size();
//               Json::Reader reader;
//               reader.parse(str,update_rx);
              
//               Value update_rx_array =  update_rx["rx_sessions"];
//               for (size_t i = 0; i < update_rx_array.size(); i++)
//               {
//                   update_rx_obj = update_rx_array[i];
//                   int update_id = update_rx_obj["id"].asInt();
//                   for (size_t j = 0; j < root_rx_arr.size(); j++)
//                   {
//                     int root_id = root_rx_arr[j]["id"].asInt();
//                     if ( update_id == root_id ){
//                         root_rx_arr[j] = update_rx_obj;
//                     }
//                   }
//               }
//               root["rx_sessions"] = root_rx_arr;
//               res.body = write.write(root);
//               string end = root.toStyledString();
//               ofstream ofs("tes_conf.json");
//               ofs << end;
//               ofs.close();
//               res.status = 200;
//               res.set_content(res.body, "application/json"); 

//              st_json_context_t * ctx_update = (st_json_context_t*)st_app_zmalloc(sizeof(st_json_context_t));
//              ret = st_app_parse_json_object_update_rx(ctx_update, update_rx_array,root);

//              for (size_t i = 0; i < update_rx_array.size(); i++)
//              {
//                Value update_rx_obj = update_rx_array[i];
//                int id = update_rx_obj["id"];
//                ret = st_rx_output_init_update( ctx,ctx_update,id);
//                ret = st_app_rx_video_sessions_uinit_update(ctx,id,ctx_update);
//                ret = st_app_rx_audio_sessions_uinit_update(ctx,id,ctx_update); 
//              }
//           });   

//          server.listen("0.0.0.0", 8081);
// }

// int main(int argc, char **argv)
// {

//     ctx = (st_app_context *)st_app_zmalloc(sizeof(*ctx));
//     int ret = 0;
//     // seeder::config config;
//     try
//     {
//         // create logger
//         logger = create_logger("info", "error","./log/log.txt", 10, 30);
//     }
//     catch (std::exception &e)
//     {
//         std::cout << e.what() << std::endl;
//         return 0;
//     }


//     if(!ctx)
//     {
//         logger->error("{}, app_context alloc fail", __func__);
//         return 0;
//     }

//     st_app_ctx_init(ctx);
//     ret = st_app_parse_args(ctx, &ctx->para, argc, argv);
//     if(ret < 0)
//     {
//         logger->error("{}, parse arguments fail", __func__);
//         st_app_ctx_free(ctx);
//         return ret;
//     }

//     ret = st_app_args_check(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, session count invalid, pass the restriction {}", __func__,
//                       ST_APP_MAX_RX_VIDEO_SESSIONS);
//         st_app_ctx_free(ctx);
//         return -EINVAL;
//     }

//     ctx->st = st_init(&ctx->para);
//     if(!ctx->st)
//     {
//         logger->error("{}, st_init fail", __func__);
//         st_app_ctx_free(ctx);
//         return -ENOMEM;
//     }
    
//     g_app_ctx = ctx;
//     if(signal(SIGINT, st_app_sig_handler) == SIG_ERR)
//     {
//         logger->error("{}, cat SIGINT fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }

//     // init tx video source
//     ret = st_tx_video_source_init(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, st_tx_video_source_init fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }

//     // tx video
//     ret = st_app_tx_video_sessions_init(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, st_app_tx_video_session_init fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }



//     // start video source input stream
//     ret = st_tx_video_source_start(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, st_tx_video_source_start fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }


//     // init rx video source
//     ret = st_rx_output_init(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, st_rx_output_init fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }

//     // rx video
//     ret = st_app_rx_video_sessions_init(ctx);
//     if(ret < 0)
//     {
//         logger->error("{}, st_app_rx_video_sessions_init fail", __func__);
//         st_app_ctx_free(ctx);
//         return -EIO;
//     }



//     if(!ctx->runtime_session)
//     {
//         ret = st_start(ctx->st);
//         if(ret < 0 )
//         {
//             logger->error("{}, start device fail", __func__);
//             st_app_ctx_free(ctx);
//             return -EIO;
//         }
//     }

 
//     // //start video source input stream
//     // ret = st_rx_output_start(ctx);
//     // if(ret < 0)
//     // {
//     //     logger->error("{}, st_rx_output_start fail", __func__);
//     //     st_app_ctx_free(ctx);
//     //     return -EIO;
//     // }


//     // wait for stop
//     int test_time_s = ctx->test_time_s;
//     logger->debug("{}, app lunch success, test time {}", __func__, test_time_s);
//     while(!ctx->stop)
//     {
//         sleep(1);
//     }
//     logger->debug("{}, start to ending", __func__);

//     if(!ctx->runtime_session) {
//         /* stop st first */
//         if(ctx->st) st_stop(ctx->st);
//     }

//     //ret = st_app_result(ctx);

//     /* free */
//     st_app_ctx_free(ctx);
//     return ret;
// }


