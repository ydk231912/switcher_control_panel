#pragma once
#include <json/json.h>

void st_app_update_stat(struct st_app_context *ctx);

Json::Value st_app_get_status(struct st_app_context *ctx);