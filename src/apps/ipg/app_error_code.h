#pragma once

#include <system_error>

struct st_app_error_category : std::error_category
{
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};