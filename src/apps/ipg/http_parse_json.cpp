// #include"http_parse_json.h"


// int st_app_parse_json_object_add_tx(st_json_context_t* ctx, json_object tx_group_array,json_object root_object,int count)
// {
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;
  
//   if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
//     /* allocate tx source config*/
//     ctx->tx_source_cnt = json_object_array_length(tx_group_array);
//     ctx->tx_sources = (struct st_app_tx_source*)st_app_zmalloc(ctx->tx_source_cnt*sizeof(struct st_app_tx_source));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse tx video sessions */
//       num = parse_session_num(tx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_video_session_cnt += num;
//       /* parse tx audio sessions */
//       num = parse_session_num(tx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_audio_session_cnt += num;
//       /* parse tx ancillary sessions */
//       num = parse_session_num(tx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_anc_session_cnt += num;
//       /* parse tx st22p sessions */
//       num = parse_session_num(tx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st22p_session_cnt += num;
//       /* parse tx st20p sessions */
//       num = parse_session_num(tx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st20p_session_cnt += num;

//       // parse tx source config
//       json_object* source = st_json_object_object_get(tx_group, "source");
//       ctx->tx_sources[i].type = json_object_get_string(st_json_object_object_get(source, "type"));
//       ctx->tx_sources[i].device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
//       ctx->tx_sources[i].file_url = json_object_get_string(st_json_object_object_get(source, "file_url"));
//       ctx->tx_sources[i].video_format = json_object_get_string(st_json_object_object_get(source, "video_format"));
//     }
    
//     /* allocate tx sessions */
//     ctx->tx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->tx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->tx_video_sessions) {
//       logger->error("{}, failed to allocate tx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->tx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->tx_audio_sessions) {
//       logger->error("{}, failed to allocate tx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->tx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->tx_anc_sessions) {
//       logger->error("{}, failed to allocate tx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->tx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->tx_st22p_sessions) {
//       logger->error("{}, failed to allocate tx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->tx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->tx_st20p_sessions) {
//       logger->error("{}, failed to allocate tx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;


//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse destination ip */
//       json_object* dip_p = NULL;
//       json_object* dip_r = NULL;
//       json_object* dip_array = st_json_object_object_get(tx_group, "dip");
//       if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
//         int len = json_object_array_length(dip_array);
//         if (len < 1 || len > ST_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         dip_p = json_object_array_get_idx(dip_array, 0);
//         if (len == 2) {
//           dip_r = json_object_array_get_idx(dip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(tx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse tx video sessions */
//       json_object* video_array = st_json_object_object_get(tx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_video_sessions[num_video].base.ip[0]);
//             ctx->tx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_video_sessions[num_video].base.ip[1]);
//               ctx->tx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_tx_video(k, video_session,
//                                          &ctx->tx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video source handle id
//             ctx->tx_video_sessions[num_video].tx_source_id = i+count;
//             num_video++;
//           }
//         }
//       }

//       /* parse tx audio sessions */
//       json_object* audio_array = st_json_object_object_get(tx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_audio_sessions[num_audio].base.ip[0]);
//             ctx->tx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_audio_sessions[num_audio].base.ip[1]);
//               ctx->tx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_tx_audio(k, audio_session,
//                                          &ctx->tx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // audio source handle id
//             ctx->tx_audio_sessions[num_audio].tx_source_id = i+count;

//             num_audio++;
//           }
//         }
//       }

//       /* parse tx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(tx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_anc_sessions[num_anc].base.ip[0]);
//             ctx->tx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_anc_sessions[num_anc].base.ip[1]);
//               ctx->tx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_tx_anc(k, anc_session, &ctx->tx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse tx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(tx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->tx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->tx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st22p(k, st22p_session,
//                                          &ctx->tx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse tx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(tx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->tx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->tx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st20p(k, st20p_session,
//                                          &ctx->tx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
// }


// int st_app_parse_json_object_update_tx(st_json_context_t* ctx, json_object tx_group_array,json_object root_object)
// {
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;
  
//   if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
//     /* allocate tx source config*/
//     ctx->tx_source_cnt = json_object_array_length(tx_group_array);
//     ctx->tx_sources = (struct st_app_tx_source*)st_app_zmalloc(ctx->tx_source_cnt*sizeof(struct st_app_tx_source));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse tx video sessions */
//       num = parse_session_num(tx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_video_session_cnt += num;
//       /* parse tx audio sessions */
//       num = parse_session_num(tx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_audio_session_cnt += num;
//       /* parse tx ancillary sessions */
//       num = parse_session_num(tx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_anc_session_cnt += num;
//       /* parse tx st22p sessions */
//       num = parse_session_num(tx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st22p_session_cnt += num;
//       /* parse tx st20p sessions */
//       num = parse_session_num(tx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st20p_session_cnt += num;

//       // parse tx source config
//       json_object* source = st_json_object_object_get(tx_group, "source");
//       ctx->tx_sources[i].type = json_object_get_string(st_json_object_object_get(source, "type"));
//       ctx->tx_sources[i].device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
//       ctx->tx_sources[i].file_url = json_object_get_string(st_json_object_object_get(source, "file_url"));
//       ctx->tx_sources[i].video_format = json_object_get_string(st_json_object_object_get(source, "video_format"));
//     }
    
//     /* allocate tx sessions */
//     ctx->tx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->tx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->tx_video_sessions) {
//       logger->error("{}, failed to allocate tx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->tx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->tx_audio_sessions) {
//       logger->error("{}, failed to allocate tx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->tx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->tx_anc_sessions) {
//       logger->error("{}, failed to allocate tx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->tx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->tx_st22p_sessions) {
//       logger->error("{}, failed to allocate tx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->tx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->tx_st20p_sessions) {
//       logger->error("{}, failed to allocate tx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;


//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       int id = json_object_get_int(st_json_object_object_get(tx_group, "id"));
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse destination ip */
//       json_object* dip_p = NULL;
//       json_object* dip_r = NULL;
//       json_object* dip_array = st_json_object_object_get(tx_group, "dip");
//       if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
//         int len = json_object_array_length(dip_array);
//         if (len < 1 || len > ST_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         dip_p = json_object_array_get_idx(dip_array, 0);
//         if (len == 2) {
//           dip_r = json_object_array_get_idx(dip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(tx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }


//       /* parse tx video sessions */
//       json_object* video_array = st_json_object_object_get(tx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_video_sessions[num_video].base.ip[0]);
//             ctx->tx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_video_sessions[num_video].base.ip[1]);
//               ctx->tx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_tx_video(k, video_session,
//                                          &ctx->tx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video source handle id
//             ctx->tx_video_sessions[num_video].tx_source_id = id;
//             num_video++;
//           }
//         }
//       }

//       /* parse tx audio sessions */
//       json_object* audio_array = st_json_object_object_get(tx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_audio_sessions[num_audio].base.ip[0]);
//             ctx->tx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_audio_sessions[num_audio].base.ip[1]);
//               ctx->tx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_tx_audio(k, audio_session,
//                                          &ctx->tx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // audio source handle id
//             ctx->tx_audio_sessions[num_audio].tx_source_id = id;

//             num_audio++;
//           }
//         }
//       }

//       /* parse tx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(tx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_anc_sessions[num_anc].base.ip[0]);
//             ctx->tx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_anc_sessions[num_anc].base.ip[1]);
//               ctx->tx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_tx_anc(k, anc_session, &ctx->tx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }


//       /* parse tx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(tx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->tx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->tx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st22p(k, st22p_session,
//                                          &ctx->tx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse tx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(tx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->tx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->tx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st20p(k, st20p_session,
//                                          &ctx->tx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
// }
// }


// int st_app_parse_json_object_add_rx(st_json_context_t* ctx, json_object rx_group_array,json_object root_object,int count)
// {
//   /* parse interfaces for system */
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;

//     if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
//     /* allocate rx output config*/
//     ctx->rx_output_cnt = json_object_array_length(rx_group_array);
//     ctx->rx_output = (st_app_rx_output*)st_app_zmalloc(ctx->rx_output_cnt*sizeof(st_app_rx_output));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse rx video sessions */
//       num = parse_session_num(rx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_video_session_cnt += num;
//       /* parse rx audio sessions */
//       num = parse_session_num(rx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_audio_session_cnt += num;
//       /* parse rx ancillary sessions */
//       num = parse_session_num(rx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_anc_session_cnt += num;
//       /* parse rx st22p sessions */
//       num = parse_session_num(rx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st22p_session_cnt += num;
//       /* parse rx st20p sessions */
//       num = parse_session_num(rx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st20p_session_cnt += num;

//       // parse rx output config
//       json_object* output = st_json_object_object_get(rx_group, "output");
//       ctx->rx_output[i].type = json_object_get_string(st_json_object_object_get(output, "type"));
//       ctx->rx_output[i].device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
//       std::string fu = json_object_get_string(st_json_object_object_get(output, "file_url"));
//       if(!(fu.empty())) ctx->rx_output[i].file_url = fu;
//       ctx->rx_output[i].video_format = json_object_get_string(st_json_object_object_get(output, "video_format"));
//     }

//     /* allocate tx sessions */
//     ctx->rx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->rx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->rx_video_sessions) {
//       logger->error("{}, failed to allocate rx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->rx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->rx_audio_sessions) {
//       logger->error("{}, failed to allocate rx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->rx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->rx_anc_sessions) {
//       logger->error("{}, failed to allocate rx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->rx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->rx_st22p_sessions) {
//       logger->error("{}, failed to allocate rx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->rx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->rx_st20p_sessions) {
//       logger->error("{}, failed to allocate rx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;

//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse receiving ip */
//       json_object* ip_p = NULL;
//       json_object* ip_r = NULL;
//       json_object* ip_array = st_json_object_object_get(rx_group, "ip");
//       if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
//         int len = json_object_array_length(ip_array);
//         if (len < 1 || len > ST_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         ip_p = json_object_array_get_idx(ip_array, 0);
//         if (len == 2) {
//           ip_r = json_object_array_get_idx(ip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(rx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse rx video sessions */
//       json_object* video_array = st_json_object_object_get(rx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_video_sessions[num_video].base.ip[0]);
//             ctx->rx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_video_sessions[num_video].base.ip[1]);
//               ctx->rx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_rx_video(k, video_session,
//                                          &ctx->rx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_video_sessions[num_video].rx_output_id = i+count;

//             num_video++;
//           }
//         }
//       }

//       /* parse rx audio sessions */
//       json_object* audio_array = st_json_object_object_get(rx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_audio_sessions[num_audio].base.ip[0]);
//             ctx->rx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_audio_sessions[num_audio].base.ip[1]);
//               ctx->rx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_rx_audio(k, audio_session,
//                                          &ctx->rx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_audio_sessions[num_audio].rx_output_id = i+count;

//             num_audio++;
//           }
//         }
//       }

//       /* parse rx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(rx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_anc_sessions[num_anc].base.ip[0]);
//             ctx->rx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_anc_sessions[num_anc].base.ip[1]);
//               ctx->rx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_rx_anc(k, anc_session, &ctx->rx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse rx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(rx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->rx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->rx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st22p(k, st22p_session,
//                                          &ctx->rx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse rx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(rx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->rx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->rx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st20p(k, st20p_session,
//                                          &ctx->rx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
// }


// int st_app_parse_json_object_update_rx(st_json_context_t* ctx, json_object rx_group_array,json_object root_object)
// {
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;

// if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
//     /* allocate rx output config*/
//     ctx->rx_output_cnt = json_object_array_length(rx_group_array);
//     ctx->rx_output = (st_app_rx_output*)st_app_zmalloc(ctx->rx_output_cnt*sizeof(st_app_rx_output));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse rx video sessions */
//       num = parse_session_num(rx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_video_session_cnt += num;
//       /* parse rx audio sessions */
//       num = parse_session_num(rx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_audio_session_cnt += num;
//       /* parse rx ancillary sessions */
//       num = parse_session_num(rx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_anc_session_cnt += num;
//       /* parse rx st22p sessions */
//       num = parse_session_num(rx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st22p_session_cnt += num;
//       /* parse rx st20p sessions */
//       num = parse_session_num(rx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st20p_session_cnt += num;

//       // parse rx output config
//       json_object* output = st_json_object_object_get(rx_group, "output");
//       ctx->rx_output[i].type = json_object_get_string(st_json_object_object_get(output, "type"));
//       ctx->rx_output[i].device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
//       std::string fu = json_object_get_string(st_json_object_object_get(output, "file_url"));
//       if(!(fu.empty())) ctx->rx_output[i].file_url = fu;
//       ctx->rx_output[i].video_format = json_object_get_string(st_json_object_object_get(output, "video_format"));
//     }

//     /* allocate rx sessions */
//     ctx->rx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->rx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->rx_video_sessions) {
//       logger->error("{}, failed to allocate rx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->rx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->rx_audio_sessions) {
//       logger->error("{}, failed to allocate rx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->rx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->rx_anc_sessions) {
//       logger->error("{}, failed to allocate rx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->rx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->rx_st22p_sessions) {
//       logger->error("{}, failed to allocate rx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->rx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->rx_st20p_sessions) {
//       logger->error("{}, failed to allocate rx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;

//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       int id = json_object_get_int(st_json_object_object_get(rx_group, "id"));
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse receiving ip */
//       json_object* ip_p = NULL;
//       json_object* ip_r = NULL;
//       json_object* ip_array = st_json_object_object_get(rx_group, "ip");
//       if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
//         int len = json_object_array_length(ip_array);
//         if (len < 1 || len > ST_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         ip_p = json_object_array_get_idx(ip_array, 0);
//         if (len == 2) {
//           ip_r = json_object_array_get_idx(ip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(rx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse rx video sessions */
//       json_object* video_array = st_json_object_object_get(rx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_video_sessions[num_video].base.ip[0]);
//             ctx->rx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_video_sessions[num_video].base.ip[1]);
//               ctx->rx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_rx_video(k, video_session,
//                                          &ctx->rx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_video_sessions[num_video].rx_output_id = id;

//             num_video++;
//           }
//         }
//       }

//       /* parse rx audio sessions */
//       json_object* audio_array = st_json_object_object_get(rx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_audio_sessions[num_audio].base.ip[0]);
//             ctx->rx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_audio_sessions[num_audio].base.ip[1]);
//               ctx->rx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_rx_audio(k, audio_session,
//                                          &ctx->rx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_audio_sessions[num_audio].rx_output_id = id;

//             num_audio++;
//           }
//         }
//       }

//       /* parse rx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(rx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_anc_sessions[num_anc].base.ip[0]);
//             ctx->rx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_anc_sessions[num_anc].base.ip[1]);
//               ctx->rx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_rx_anc(k, anc_session, &ctx->rx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse rx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(rx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->rx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->rx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st22p(k, st22p_session,
//                                          &ctx->rx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse rx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(rx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->rx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->rx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st20p(k, st20p_session,
//                                          &ctx->rx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
// }


