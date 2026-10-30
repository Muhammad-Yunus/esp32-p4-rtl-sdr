#pragma once
#include "driver/i2c_master.h"

esp_err_t ws_i2c_init(void);
i2c_master_bus_handle_t ws_i2c_bus(void);
