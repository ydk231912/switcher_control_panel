#pragma once

#include "app_base.h"

#include <system_error>

std::error_code st_app_add_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx);

std::error_code st_app_update_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx);

std::error_code st_app_remove_tx_session(st_app_context *ctx, const std::string &tx_source_id);

std::error_code st_app_add_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx);

std::error_code st_app_update_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx);

std::error_code st_app_remove_rx_session(st_app_context *ctx, const std::string &rx_output_id);