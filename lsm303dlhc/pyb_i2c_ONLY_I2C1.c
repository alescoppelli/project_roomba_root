#define PYB_I2C_MASTER (0)
#define PYB_I2C_SLAVE  (1)

#define PYB_I2C_SPEED_STANDARD (100000L)
#define PYB_I2C_SPEED_FULL     (400000L)
#define PYB_I2C_SPEED_FAST     (1000000L)


#if defined(MICROPY_HW_I2C1_SCL)
I2C_HandleTypeDef I2CHandle1 = {.Instance = NULL};
#endif

STATIC bool pyb_i2c_use_dma[4];

const pyb_i2c_obj_t pyb_i2c_obj[] = {
    #if defined(MICROPY_HW_I2C1_SCL)
    {{&pyb_i2c_type}, &I2CHandle1, &dma_I2C_1_TX, &dma_I2C_1_RX, &pyb_i2c_use_dma[0]},
    #endif
};

STATIC const struct {
    uint32_t baudrate;
    uint32_t timing;
} pyb_i2c_baudrate_timing[] = MICROPY_HW_I2C_BAUDRATE_TIMING;

#define NUM_BAUDRATE_TIMINGS MP_ARRAY_SIZE(pyb_i2c_baudrate_timing)

STATIC void i2c_set_baudrate(I2C_InitTypeDef *init, uint32_t baudrate) {
    for (int i = 0; i < NUM_BAUDRATE_TIMINGS; i++) {
        if (pyb_i2c_baudrate_timing[i].baudrate == baudrate) {
            init->Timing = pyb_i2c_baudrate_timing[i].timing;
            return;
        }
    }
    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("unsupported I2C baudrate: %u"), baudrate);
}

uint32_t pyb_i2c_get_baudrate(I2C_HandleTypeDef *i2c) {
    for (int i = 0; i < NUM_BAUDRATE_TIMINGS; i++) {
        if (pyb_i2c_baudrate_timing[i].timing == i2c->Init.Timing) {
            return pyb_i2c_baudrate_timing[i].baudrate;
        }
    }
    return 0;
}

void i2c_init0(void) {
    // Initialise the I2C handles.
    // The structs live on the BSS so all other fields will be zero after a reset.
    #if defined(MICROPY_HW_I2C1_SCL)
    I2CHandle1.Instance = I2C1;
    #endif
    #endif
}

void pyb_i2c_init(I2C_HandleTypeDef *i2c) {
    int i2c_unit;
    const pin_obj_t *scl_pin;
    const pin_obj_t *sda_pin;

    if (0) {
    #if defined(MICROPY_HW_I2C1_SCL)
    } else if (i2c == &I2CHandle1) {
        i2c_unit = 1;
        scl_pin = MICROPY_HW_I2C1_SCL;
        sda_pin = MICROPY_HW_I2C1_SDA;
        __HAL_RCC_I2C1_CLK_ENABLE();
    #endif

   } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

    // init the GPIO lines
    uint32_t mode = MP_HAL_PIN_MODE_ALT_OPEN_DRAIN;
    uint32_t pull = MP_HAL_PIN_PULL_NONE; // have external pull-up resistors on both lines
    mp_hal_pin_config_alt(scl_pin, mode, pull, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(sda_pin, mode, pull, AF_FN_I2C, i2c_unit);

    // init the I2C device
    if (HAL_I2C_Init(i2c) != HAL_OK) {
        // init error
        // TODO should raise an exception, but this function is not necessarily going to be
        // called via Python, so may not be properly wrapped in an NLR handler
        printf("OSError: HAL_I2C_Init failed\n");
        return;
    }

    // invalidate the DMA channels so they are initialised on first use
    const pyb_i2c_obj_t *self = &pyb_i2c_obj[i2c_unit - 1];
    dma_invalidate_channel(self->tx_dma_descr);
    dma_invalidate_channel(self->rx_dma_descr);


    if (0) {
    #if defined(MICROPY_HW_I2C1_SCL)
    } else if (i2c->Instance == I2C1) {
        HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    #endif
    #endif
    }
}

void i2c_deinit(I2C_HandleTypeDef *i2c) {
    HAL_I2C_DeInit(i2c);
    if (0) {
    #if defined(MICROPY_HW_I2C1_SCL)
    } else if (i2c->Instance == I2C1) {
        __HAL_RCC_I2C1_FORCE_RESET();
        __HAL_RCC_I2C1_RELEASE_RESET();
        __HAL_RCC_I2C1_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    #endif
    #endif
    }
}

void pyb_i2c_init_freq(const pyb_i2c_obj_t *self, mp_int_t freq) {
    I2C_InitTypeDef *init = &self->i2c->Init;

    init->AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    init->DualAddressMode = I2C_DUALADDRESS_DISABLED;
    init->GeneralCallMode = I2C_GENERALCALL_DISABLED;
    init->NoStretchMode = I2C_NOSTRETCH_DISABLE;
    init->OwnAddress1 = PYB_I2C_MASTER_ADDRESS;
    init->OwnAddress2 = 0;       // unused
    if (freq != -1) {
        i2c_set_baudrate(init, MIN(freq, MICROPY_HW_I2C_BAUDRATE_MAX));
    }

    *self->use_dma = false;

    // init the I2C bus
    i2c_deinit(self->i2c);
    pyb_i2c_init(self->i2c);
}

STATIC void i2c_reset_after_error(I2C_HandleTypeDef *i2c) {
    // wait for bus-busy flag to be cleared, with a timeout
    for (int timeout = 50; timeout > 0; --timeout) {
        if (!__HAL_I2C_GET_FLAG(i2c, I2C_FLAG_BUSY)) {
            // stop bit was generated and bus is back to normal
            return;
        }
        mp_hal_delay_ms(1);
    }
    // bus was/is busy, need to reset the peripheral to get it to work again
    i2c_deinit(i2c);
    pyb_i2c_init(i2c);
}


void i2c_ev_irq_handler(mp_uint_t i2c_id) {
    I2C_HandleTypeDef *hi2c;

    switch (i2c_id) {
        #if defined(MICROPY_HW_I2C1_SCL)
        case 1:
            hi2c = &I2CHandle1;
            break;
        #endif

        default:
            return;
    }

    #if defined(STM32F4)

    if (hi2c->Instance->SR1 & I2C_FLAG_BTF && hi2c->State == HAL_I2C_STATE_BUSY_TX) {
        if (hi2c->XferCount != 0U) {
            hi2c->Instance->DR = *hi2c->pBuffPtr++;
            hi2c->XferCount--;
        } else {
            __HAL_I2C_DISABLE_IT(hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
            if (hi2c->XferOptions != I2C_FIRST_FRAME) {
                hi2c->Instance->CR1 |= I2C_CR1_STOP;
            }
            hi2c->Mode = HAL_I2C_MODE_NONE;
            hi2c->State = HAL_I2C_STATE_READY;
        }
    }

    #else

    // if not an F4 MCU, use the HAL's IRQ handler
    HAL_I2C_EV_IRQHandler(hi2c);

    #endif
}

void i2c_er_irq_handler(mp_uint_t i2c_id) {
    I2C_HandleTypeDef *hi2c;

    switch (i2c_id) {
        #if defined(MICROPY_HW_I2C1_SCL)
        case 1:
            hi2c = &I2CHandle1;
            break;
        #endif
   

       #endif
        default:
            return;
    }
   #if defined(STM32F4)

    uint32_t sr1 = hi2c->Instance->SR1;

    // I2C Bus error
    if (sr1 & I2C_FLAG_BERR) {
        hi2c->ErrorCode |= HAL_I2C_ERROR_BERR;
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_BERR);
    }

    // I2C Arbitration Loss error
    if (sr1 & I2C_FLAG_ARLO) {
        hi2c->ErrorCode |= HAL_I2C_ERROR_ARLO;
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_ARLO);
    }

    // I2C Acknowledge failure
    if (sr1 & I2C_FLAG_AF) {
        hi2c->ErrorCode |= HAL_I2C_ERROR_AF;
        SET_BIT(hi2c->Instance->CR1,I2C_CR1_STOP);
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);
    }

    // I2C Over-Run/Under-Run
    if (sr1 & I2C_FLAG_OVR) {
        hi2c->ErrorCode |= HAL_I2C_ERROR_OVR;
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_OVR);
    }

    #else

    // if not an F4 MCU, use the HAL's IRQ handler
    HAL_I2C_ER_IRQHandler(hi2c);

    #endif
}




STATIC HAL_StatusTypeDef i2c_wait_dma_finished(I2C_HandleTypeDef *i2c, uint32_t timeout) {
    // Note: we can't use WFI to idle in this loop because the DMA completion
    // interrupt may occur before the WFI.  Hence we miss it and have to wait
    // until the next sys-tick (up to 1ms).
    uint32_t start = HAL_GetTick();
    while (HAL_I2C_GetState(i2c) != HAL_I2C_STATE_READY) {
        if (HAL_GetTick() - start >= timeout) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

/******************************************************************************/
/* MicroPython bindings                                                       */

static inline bool in_master_mode(pyb_i2c_obj_t *self) {
    return self->i2c->Init.OwnAddress1 == PYB_I2C_MASTER_ADDRESS;
}

STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint i2c_num = 0;
    if (0) {
    }
    #if defined(MICROPY_HW_I2C1_SCL)
    else if (self->i2c->Instance == I2C1) {
        i2c_num = 1;
    }
    #endif

    if (self->i2c->State == HAL_I2C_STATE_RESET) {
        mp_printf(print, "I2C(%u)", i2c_num);
    } else {
        if (in_master_mode(self)) {
            mp_printf(print, "I2C(%u, I2C.MASTER, baudrate=%u"
                #if PYB_I2C_TIMINGR
                ", timingr=0x%08x"
                #endif
                ")", i2c_num, pyb_i2c_get_baudrate(self->i2c)
                #if PYB_I2C_TIMINGR
                , self->i2c->Init.Timing
                #endif
                );
        } else {
            mp_printf(print, "I2C(%u, I2C.SLAVE, addr=0x%02x)", i2c_num, (self->i2c->Instance->OAR1 >> 1) & 0x7f);
        }
    }
}


/// \method init(mode, *, addr=0x12, baudrate=400000, gencall=False)
///
/// Initialise the I2C bus with the given parameters:
///
///   - `mode` must be either `I2C.MASTER` or `I2C.SLAVE`
///   - `addr` is the 7-bit address (only sensible for a slave)
///   - `baudrate` is the SCL clock rate (only sensible for a master)
///   - `gencall` is whether to support general call mode
STATIC mp_obj_t pyb_i2c_init_helper(const pyb_i2c_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_INT, {.u_int = PYB_I2C_MASTER} },
        { MP_QSTR_addr,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x12} },
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_HW_I2C_BAUDRATE_DEFAULT} },
        { MP_QSTR_gencall,  MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_dma,      MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        #if PYB_I2C_TIMINGR
        { MP_QSTR_timingr,  MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        #endif
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set the I2C configuration values
    I2C_InitTypeDef *init = &self->i2c->Init;

    if (args[0].u_int == PYB_I2C_MASTER) {
        // use a special address to indicate we are a master
        init->OwnAddress1 = PYB_I2C_MASTER_ADDRESS;
    } else {
        init->OwnAddress1 = (args[1].u_int << 1) & 0xfe;
    }

    // Set baudrate or timing value (if supported)
    #if PYB_I2C_TIMINGR
    if (args[5].u_obj != mp_const_none) {
        init->Timing = mp_obj_get_int_truncated(args[5].u_obj);
    } else
    #endif
    {
        i2c_set_baudrate(init, MIN(args[2].u_int, MICROPY_HW_I2C_BAUDRATE_MAX));
    }

    init->AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    init->DualAddressMode = I2C_DUALADDRESS_DISABLED;
    init->GeneralCallMode = args[3].u_bool ? I2C_GENERALCALL_ENABLED : I2C_GENERALCALL_DISABLED;
    init->OwnAddress2 = 0;     // unused
    init->NoStretchMode = I2C_NOSTRETCH_DISABLE;

    *self->use_dma = args[4].u_bool;

    // init the I2C bus
    i2c_deinit(self->i2c);
    pyb_i2c_init(self->i2c);

    return mp_const_none;
}



/// \classmethod \constructor(bus, ...)
///
/// Construct an I2C object on the given bus.  `bus` can be 1 or 2.
/// With no additional parameters, the I2C object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the I2C busses are:
///
///   - `I2C(1)` is on the X position: `(SCL, SDA) = (X9, X10) = (PB6, PB7)`
///   - `I2C(2)` is on the Y position: `(SCL, SDA) = (Y9, Y10) = (PB10, PB11)`
STATIC mp_obj_t pyb_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out i2c bus
    int i2c_id = 0;
    if (mp_obj_is_str(args[0])) {
        const char *port = mp_obj_str_get_str(args[0]);
        if (0) {
        #ifdef MICROPY_HW_I2C1_NAME
        } else if (strcmp(port, MICROPY_HW_I2C1_NAME) == 0) {
            i2c_id = 1;
        #endif


        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%s) doesn't exist"), port);
        }
    } else {
        i2c_id = mp_obj_get_int(args[0]);
        if (i2c_id < 1 || i2c_id > MP_ARRAY_SIZE(pyb_i2c_obj)
            || pyb_i2c_obj[i2c_id - 1].i2c == NULL) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
        }
    }
    // get I2C object
    const pyb_i2c_obj_t *i2c_obj = &pyb_i2c_obj[i2c_id - 1];

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_i2c_init_helper(i2c_obj, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(i2c_obj);
}

STATIC mp_obj_t pyb_i2c_init_(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_i2c_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_init_obj, 1, pyb_i2c_init_);

/// \method deinit()
/// Turn off the I2C bus.
STATIC mp_obj_t pyb_i2c_deinit(mp_obj_t self_in) {
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    i2c_deinit(self->i2c);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_deinit_obj, pyb_i2c_deinit);

/// \method is_ready(addr)
/// Check if an I2C device responds to the given address.  Only valid when in master mode.
STATIC mp_obj_t pyb_i2c_is_ready(mp_obj_t self_in, mp_obj_t i2c_addr_o) {
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!in_master_mode(self)) {
        mp_raise_TypeError(MP_ERROR_TEXT("I2C must be a master"));
    }

    mp_uint_t i2c_addr = mp_obj_get_int(i2c_addr_o) << 1;

    for (int i = 0; i < 10; i++) {
        HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(self->i2c, i2c_addr, 10, 200);
        if (status == HAL_OK) {
            return mp_const_true;
        }
    }

    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_i2c_is_ready_obj, pyb_i2c_is_ready);

/// \method scan()
/// Scan all I2C addresses from 0x08 to 0x77 and return a list of those that respond.
/// Only valid when in master mode.
STATIC mp_obj_t pyb_i2c_scan(mp_obj_t self_in) {
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!in_master_mode(self)) {
        mp_raise_TypeError(MP_ERROR_TEXT("I2C must be a master"));
    }

    mp_obj_t list = mp_obj_new_list(0, NULL);

    for (uint addr = 0x08; addr <= 0x77; addr++) {
        HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(self->i2c, addr << 1, 1, 200);
        if (status == HAL_OK) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }

    return list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_scan_obj, pyb_i2c_scan);

/// \method send(send, addr=0x00, timeout=5000)
/// Send data on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object)
///   - `addr` is the address to send to (only required in master mode)
///   - `timeout` is the timeout in milliseconds to wait for the send
///
/// Return value: `None`.
STATIC mp_obj_t pyb_i2c_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    // if option is set and IRQs are enabled then we can use DMA
    bool use_dma = *self->use_dma && query_irq() == IRQ_STATE_ENABLED;

    DMA_HandleTypeDef tx_dma;
    if (use_dma) {
        dma_init(&tx_dma, self->tx_dma_descr, DMA_MEMORY_TO_PERIPH, self->i2c);
        self->i2c->hdmatx = &tx_dma;
        self->i2c->hdmarx = NULL;
    }

    // send the data
    HAL_StatusTypeDef status;
    if (in_master_mode(self)) {
        if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
            if (use_dma) {
                dma_deinit(self->tx_dma_descr);
            }
            mp_raise_TypeError(MP_ERROR_TEXT("addr argument required"));
        }
        mp_uint_t i2c_addr = args[1].u_int << 1;
        if (!use_dma) {
            status = HAL_I2C_Master_Transmit(self->i2c, i2c_addr, bufinfo.buf, bufinfo.len, args[2].u_int);
        } else {
            MP_HAL_CLEAN_DCACHE(bufinfo.buf, bufinfo.len);
            status = HAL_I2C_Master_Transmit_DMA(self->i2c, i2c_addr, bufinfo.buf, bufinfo.len);
        }
    } else {
        if (!use_dma) {
            status = HAL_I2C_Slave_Transmit(self->i2c, bufinfo.buf, bufinfo.len, args[2].u_int);
        } else {
            MP_HAL_CLEAN_DCACHE(bufinfo.buf, bufinfo.len);
            status = HAL_I2C_Slave_Transmit_DMA(self->i2c, bufinfo.buf, bufinfo.len);
        }
    }

    // if we used DMA, wait for it to finish
    if (use_dma) {
        if (status == HAL_OK) {
            status = i2c_wait_dma_finished(self->i2c, args[2].u_int);
        }
        dma_deinit(self->tx_dma_descr);
    }

    if (status != HAL_OK) {
        i2c_reset_after_error(self->i2c);
        mp_hal_raise(status);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_send_obj, 1, pyb_i2c_send);


/// \method recv(recv, addr=0x00, timeout=5000)
///
/// Receive data on the bus:
///
///   - `recv` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes
///   - `addr` is the address to receive from (only required in master mode)
///   - `timeout` is the timeout in milliseconds to wait for the receive
///
/// Return value: if `recv` is an integer then a new buffer of the bytes received,
/// otherwise the same buffer that was passed in to `recv`.
STATIC mp_obj_t pyb_i2c_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_recv,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    // if option is set and IRQs are enabled then we can use DMA
    bool use_dma = *self->use_dma && query_irq() == IRQ_STATE_ENABLED;

    DMA_HandleTypeDef rx_dma;
    if (use_dma) {
        dma_init(&rx_dma, self->rx_dma_descr, DMA_PERIPH_TO_MEMORY, self->i2c);
        self->i2c->hdmatx = NULL;
        self->i2c->hdmarx = &rx_dma;
    }

    // receive the data
    HAL_StatusTypeDef status;
    if (in_master_mode(self)) {
        if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
            mp_raise_TypeError(MP_ERROR_TEXT("addr argument required"));
        }
        mp_uint_t i2c_addr = args[1].u_int << 1;
        if (!use_dma) {
            status = HAL_I2C_Master_Receive(self->i2c, i2c_addr, (uint8_t *)vstr.buf, vstr.len, args[2].u_int);
        } else {
            MP_HAL_CLEANINVALIDATE_DCACHE(vstr.buf, vstr.len);
            status = HAL_I2C_Master_Receive_DMA(self->i2c, i2c_addr, (uint8_t *)vstr.buf, vstr.len);
        }
    } else {
        if (!use_dma) {
            status = HAL_I2C_Slave_Receive(self->i2c, (uint8_t *)vstr.buf, vstr.len, args[2].u_int);
        } else {
            MP_HAL_CLEANINVALIDATE_DCACHE(vstr.buf, vstr.len);
            status = HAL_I2C_Slave_Receive_DMA(self->i2c, (uint8_t *)vstr.buf, vstr.len);
        }
    }

    // if we used DMA, wait for it to finish
    if (use_dma) {
        if (status == HAL_OK) {
            status = i2c_wait_dma_finished(self->i2c, args[2].u_int);
        }
        dma_deinit(self->rx_dma_descr);
    }

    if (status != HAL_OK) {
        i2c_reset_after_error(self->i2c);
        mp_hal_raise(status);
    }

    // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_recv_obj, 1, pyb_i2c_recv);

/// \method mem_read(data, addr, memaddr, timeout=5000, addr_size=8)
///
/// Read from the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to read into
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `timeout` is the timeout in milliseconds to wait for the read
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns the read data.
/// This is only valid in master mode.
STATIC const mp_arg_t pyb_i2c_mem_read_allowed_args[] = {
    { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    { MP_QSTR_addr_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

STATIC mp_obj_t pyb_i2c_mem_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

    if (!in_master_mode(self)) {
        mp_raise_TypeError(MP_ERROR_TEXT("I2C must be a master"));
    }

    // get the buffer to read into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    // get the addresses
    mp_uint_t i2c_addr = args[1].u_int << 1;
    mp_uint_t mem_addr = args[2].u_int;
    // determine width of mem_addr; default is 8 bits, entering any other value gives 16 bit width
    mp_uint_t mem_addr_size = I2C_MEMADD_SIZE_8BIT;
    if (args[4].u_int != 8) {
        mem_addr_size = I2C_MEMADD_SIZE_16BIT;
    }

    // if option is set and IRQs are enabled then we can use DMA
    bool use_dma = *self->use_dma && query_irq() == IRQ_STATE_ENABLED;

    HAL_StatusTypeDef status;
    if (!use_dma) {
        status = HAL_I2C_Mem_Read(self->i2c, i2c_addr, mem_addr, mem_addr_size, (uint8_t *)vstr.buf, vstr.len, args[3].u_int);
    } else {
        DMA_HandleTypeDef rx_dma;
        dma_init(&rx_dma, self->rx_dma_descr, DMA_PERIPH_TO_MEMORY, self->i2c);
        self->i2c->hdmatx = NULL;
        self->i2c->hdmarx = &rx_dma;
        MP_HAL_CLEANINVALIDATE_DCACHE(vstr.buf, vstr.len);
        status = HAL_I2C_Mem_Read_DMA(self->i2c, i2c_addr, mem_addr, mem_addr_size, (uint8_t *)vstr.buf, vstr.len);
        if (status == HAL_OK) {
            status = i2c_wait_dma_finished(self->i2c, args[3].u_int);
        }
        dma_deinit(self->rx_dma_descr);
    }

    if (status != HAL_OK) {
        i2c_reset_after_error(self->i2c);
        mp_hal_raise(status);
    }

    // return the read data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_mem_read_obj, 1, pyb_i2c_mem_read);

/// \method mem_write(data, addr, memaddr, timeout=5000, addr_size=8)
///
/// Write to the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to write from
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `timeout` is the timeout in milliseconds to wait for the write
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns `None`.
/// This is only valid in master mode.
STATIC mp_obj_t pyb_i2c_mem_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args (same as mem_read)
    pyb_i2c_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

    if (!in_master_mode(self)) {
        mp_raise_TypeError(MP_ERROR_TEXT("I2C must be a master"));
    }

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    // get the addresses
    mp_uint_t i2c_addr = args[1].u_int << 1;
    mp_uint_t mem_addr = args[2].u_int;
    // determine width of mem_addr; default is 8 bits, entering any other value gives 16 bit width
    mp_uint_t mem_addr_size = I2C_MEMADD_SIZE_8BIT;
    if (args[4].u_int != 8) {
        mem_addr_size = I2C_MEMADD_SIZE_16BIT;
    }

    // if option is set and IRQs are enabled then we can use DMA
    bool use_dma = *self->use_dma && query_irq() == IRQ_STATE_ENABLED;

    HAL_StatusTypeDef status;
    if (!use_dma) {
        status = HAL_I2C_Mem_Write(self->i2c, i2c_addr, mem_addr, mem_addr_size, bufinfo.buf, bufinfo.len, args[3].u_int);
    } else {
        DMA_HandleTypeDef tx_dma;
        dma_init(&tx_dma, self->tx_dma_descr, DMA_MEMORY_TO_PERIPH, self->i2c);
        self->i2c->hdmatx = &tx_dma;
        self->i2c->hdmarx = NULL;
        MP_HAL_CLEAN_DCACHE(bufinfo.buf, bufinfo.len);
        status = HAL_I2C_Mem_Write_DMA(self->i2c, i2c_addr, mem_addr, mem_addr_size, bufinfo.buf, bufinfo.len);
        if (status == HAL_OK) {
            status = i2c_wait_dma_finished(self->i2c, args[3].u_int);
        }
        dma_deinit(self->tx_dma_descr);
    }

    if (status != HAL_OK) {
        i2c_reset_after_error(self->i2c);
        mp_hal_raise(status);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_mem_write_obj, 1, pyb_i2c_mem_write);

STATIC const mp_rom_map_elem_t pyb_i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_ready), MP_ROM_PTR(&pyb_i2c_is_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&pyb_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_i2c_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_i2c_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_read), MP_ROM_PTR(&pyb_i2c_mem_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_write), MP_ROM_PTR(&pyb_i2c_mem_write_obj) },

    // class constants
    /// \constant MASTER - for initialising the bus to master mode
    /// \constant SLAVE - for initialising the bus to slave mode
    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(PYB_I2C_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE), MP_ROM_INT(PYB_I2C_SLAVE) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_i2c_locals_dict, pyb_i2c_locals_dict_table);

const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = pyb_i2c_print,
    .make_new = pyb_i2c_make_new,
    .locals_dict = (mp_obj_dict_t *)&pyb_i2c_locals_dict,
};

#endif // MICROPY_PY_PYB_LEGACY && MICROPY_HW_ENABLE_HW_I2C
















































































--------------micropython/ports/stm32/bufhelper.h--------------------
#ifndef MICROPY_INCLUDED_STM32_BUFHELPER_H
#define MICROPY_INCLUDED_STM32_BUFHELPER_H

void pyb_buf_get_for_send(mp_obj_t o, mp_buffer_info_t *bufinfo, byte *tmp_data);
mp_obj_t pyb_buf_get_for_recv(mp_obj_t o, vstr_t *vstr);

#endif // MICROPY_INCLUDED_STM32_BUFHELPER_H
---------------------------------------------------------------------

-INIZIO mp_buffer_info_t----------micropython/py/obj.h---------------

// Buffer protocol
typedef struct _mp_buffer_info_t {
    void *buf;      // can be NULL if len == 0
    size_t len;     // in bytes
    int typecode;   // as per binary.h
} mp_buffer_info_t;

-FINE mp_buffer_info_t------------------------------------------------


-INIZIO mp_buffer_p_t--------------micropython/py/obj.h---------------


...
...
typedef struct _mp_buffer_p_t {
    mp_int_t (*get_buffer)(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
} mp_buffer_p_t;
...
...
...

-FINE mp_buffer_p_t-------------micropython/py/obj.h--------------------


-INIZIO mp_obj_t--------------------micropython/py/obj.h---------------

// This is the definition of the opaque MicroPython object type.
// All concrete objects have an encoding within this type and the
// particular encoding is specified by MICROPY_OBJ_REPR.
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
typedef uint64_t mp_obj_t;
typedef uint64_t mp_const_obj_t;
#else
typedef void *mp_obj_t;
typedef const void *mp_const_obj_t;
#endif

// This mp_obj_type_t struct is a concrete MicroPython object which holds info
// about a type.  See below for actual definition of the struct.
typedef struct _mp_obj_type_t mp_obj_type_t;

// Anything that wants to be a concrete MicroPython object must have mp_obj_base_t
// as its first member (small ints, qstr objs and inline floats are not concrete).
struct _mp_obj_base_t {
    const mp_obj_type_t *type MICROPY_OBJ_BASE_ALIGNMENT;
};
typedef struct _mp_obj_base_t mp_obj_base_t;

// These fake objects are used to indicate certain things in arguments or return
// values, and should only be used when explicitly allowed.
//
//  - MP_OBJ_NULL : used to indicate the absence of an object, or unsupported operation.
//  - MP_OBJ_STOP_ITERATION : used instead of throwing a StopIteration, for efficiency.
//  - MP_OBJ_SENTINEL : used for various internal purposes where one needs
//    an object which is unique from all other objects, including MP_OBJ_NULL.
//
// For debugging purposes they are all different.  For non-debug mode, we alias
// as many as we can to MP_OBJ_NULL because it's cheaper to load/compare 0.

#if MICROPY_DEBUG_MP_OBJ_SENTINELS
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void *)4))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void *)8))
#else
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void *)4))
#endif

-FINE mp_obj_t-----------------------------------------------------------


-INIZIO  mp_fun_table.type_type ------py/nativeglue.h--------------------

typedef struct _mp_fun_table_t {
    mp_const_obj_t const_none;
    mp_const_obj_t const_false;
    mp_const_obj_t const_true;
    ...
    ...
    const mp_obj_type_t *type_type;
    const mp_obj_type_t *type_str;
    const mp_obj_type_t *type_list;
    const mp_obj_type_t *type_dict;
    const mp_obj_type_t *type_fun_builtin_0;
    const mp_obj_type_t *type_fun_builtin_1;
    const mp_obj_type_t *type_fun_builtin_2;
    const mp_obj_type_t *type_fun_builtin_3;
    const mp_obj_type_t *type_fun_builtin_var;
    ...
    ...
    ...
} mp_fun_table_t;



-FINE  mp_fun_table.type_type -------------------------------------------


--INIZIO------BUFFER PROTOCOL FROM PYTHON 3 DOCUMENTATION------------------
Buffer Protocol

Certain objects available in Python wrap access to an underlying memory 
array or buffer. Such objects include the built-in bytes and bytearray, 
and some extension types like array.array. Third-party libraries may define 
their own types for special purposes, such as image processing or numeric analysis.

While each of these types have their own semantics, they share the common 
characteristic of being backed by a possibly large memory buffer. 
It is then desirable, in some situations, to access that buffer directly and 
without intermediate copying.

Python provides such a facility at the C level in the form of the buffer protocol. 
This protocol has two sides:

    on the producer side, a type can export a “buffer interface” which allows 
objects of that type to expose information about their underlying buffer. 
This interface is described in the section Buffer Object Structures;

    on the consumer side, several means are available to obtain a pointer to 
the raw underlying data of an object (for example a method parameter).

Simple objects such as bytes and bytearray expose their underlying buffer 
in byte-oriented form. Other forms are possible; for example, 
the elements exposed by an array.array can be multi-byte values.

An example consumer of the buffer interface is the write() method of file objects: 
any object that can export a series of bytes through the buffer interface can be 
written to a file. While write() only needs read-only access to the internal 
contents of the object passed to it, other methods such as readinto() need write 
access to the contents of their argument. 
The buffer interface allows objects to selectively allow or reject exporting 
of read-write and read-only buffers.

------FINE-----------------------------------------------------------------

--INIZIO  OGGETTI BUFFER PROTOCOL------------------------------------------

-------------------------py/objstr.h--------------------------------------
typedef struct _mp_obj_str_t {
    mp_obj_base_t base;
    mp_uint_t hash;
    // len == number of bytes used in data, alloc = len + 1 
    //because (at the moment) we also append a null byte
    size_t len;
    const byte *data;
} mp_obj_str_t;
-----------------------
------------------------py/objarray.h-----------------------------------------
// This structure is used for all of bytearray, array.array, memoryview
// objects.  Note that memoryview has different meaning for some fields,
// see comment at the beginning of objarray.c.
typedef struct _mp_obj_array_t {
    mp_obj_base_t base;
    size_t typecode : 8;
    // free is number of unused elements after len used elements
    // alloc size = len + free
    // But for memoryview, 'free' is reused as offset (in elements) into the
    // parent object. (Union is not used to not go into a complication of
    // union-of-bitfields with different toolchains). See comments in
    // objarray.c.
    size_t free : (8 * sizeof(size_t) - 8);
    size_t len; // in elements
    void *items;
} mp_obj_array_t;
------------------------

---------py/binary.h-------------------------------------------------
// Use special typecode to differentiate repr() of bytearray vs array.array('B')
// (underlyingly they're same).  Can't use 0 here because that's used to detect
// type-specification errors due to end-of-string.
#define BYTEARRAY_TYPECODE 1
------------------


--FNE  OGGETTI BUFFER PROTOCOL------------------------------------------



--INIZIO I2C1 lib/stm32lib/CMSIS/STM32Fxx/Include/stm32f411xe.h --------

...
#define I2C1_BASE             (APB1PERIPH_BASE + 0x5400U)
...
#define I2C1                ((I2C_TypeDef *) I2C1_BASE)

--FINE ----------------------------------------------------------------

--INIZIO I2C_TypeDef---lib/stm32lib/CMSIS/STM32Fxx/Include/stm32f411xe.h--

/**
  * @brief Inter-integrated Circuit Interface
  */

typedef struct
{
  __IO uint32_t CR1;        /*!< I2C Control register 1,     Address offset: 0x00 */
  __IO uint32_t CR2;        /*!< I2C Control register 2,     Address offset: 0x04 */
  __IO uint32_t OAR1;       /*!< I2C Own address register 1, Address offset: 0x08 */
  __IO uint32_t OAR2;       /*!< I2C Own address register 2, Address offset: 0x0C */
  __IO uint32_t DR;         /*!< I2C Data register,          Address offset: 0x10 */
  __IO uint32_t SR1;        /*!< I2C Status register 1,      Address offset: 0x14 */
  __IO uint32_t SR2;        /*!< I2C Status register 2,      Address offset: 0x18 */
  __IO uint32_t CCR;        /*!< I2C Clock control register, Address offset: 0x1C */
  __IO uint32_t TRISE;      /*!< I2C TRISE register,         Address offset: 0x20 */
  __IO uint32_t FLTR;       /*!< I2C FLTR register,          Address offset: 0x24 */
} I2C_TypeDef;


--FINE -----------------------------------------------------------------

-INIZIO I2C_HandleTypeDef--lib/stm32lib/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_i2c.h


/**
  * @brief  I2C handle Structure definition
  */
typedef struct
{
  I2C_TypeDef   *Instance;      /*!< I2C registers base address   */

  I2C_InitTypeDef   Init;     /*!< I2C communication parameters   */

  uint8_t      *pBuffPtr;  /*!< Pointer to I2C transfer buffer */

  uint16_t    XferSize;    /*!< I2C transfer size  */

  __IO uint16_t  XferCount;      /*!< I2C transfer counter  */

  __IO uint32_t  XferOptions;    /*!< I2C transfer options  */

  __IO uint32_t  PreviousState;  /*!< I2C communication Previous state and mode
                                 context for internal usage   */

  DMA_HandleTypeDef  *hdmatx;   /*!< I2C Tx DMA handle parameters  */

  DMA_HandleTypeDef  *hdmarx;    /*!< I2C Rx DMA handle parameters */

  HAL_LockTypeDef  Lock;          /*!< I2C locking object  */

  __IO HAL_I2C_StateTypeDef  State; /*!< I2C communication state */

  __IO HAL_I2C_ModeTypeDef   Mode;  /*!< I2C communication mode*/

  __IO uint32_t      ErrorCode;      /*!< I2C Error code */

  __IO uint32_t      Devaddress;     /*!< I2C Target device address */

  __IO uint32_t     Memaddress;/*!< I2C Target memory address */

  __IO uint32_t    MemaddSize; /*!< I2C Target memory address size*/

  __IO uint32_t   EventCount;  /*!< I2C Event counter*/

}I2C_HandleTypeDef;


-FINE I2C_HandleTypeDef---------------------------------------------------------------------


-INIZIO mp_print_fun_t-----------------micropython/py/obj.h---------------

typedef void (*mp_print_fun_t)(const mp_print_t *print, mp_obj_t o, mp_print_kind_t kind);


-FINE mp_print_fun_t-------------------------------------------------------

-INIZIO mp_obj_type_t-----------------micropython/py/obj.h---------------

struct _mp_obj_type_t {
    // A type is an object so must start with this entry, which points to mp_type_type.
    mp_obj_base_t base;

    // Flags associated with this type.
    uint16_t flags;

    // The name of this type, a qstr.
    uint16_t name;

    // Corresponds to __repr__ and __str__ special methods.
    mp_print_fun_t print;

    // Corresponds to __new__ and __init__ special methods, to make an instance of the type.
    mp_make_new_fun_t make_new;

    // Corresponds to __call__ special method, ie T(...).
    mp_call_fun_t call;

    // Implements unary and binary operations.
    // Can return MP_OBJ_NULL if the operation is not supported.
    mp_unary_op_fun_t unary_op;
    mp_binary_op_fun_t binary_op;

    // Implements load, store and delete attribute.
    //
    // dest[0] = MP_OBJ_NULL means load
    //  return: for fail, do nothing
    //          for attr, dest[0] = value
    //          for method, dest[0] = method, dest[1] = self
    //
    // dest[0,1] = {MP_OBJ_SENTINEL, MP_OBJ_NULL} means delete
    // dest[0,1] = {MP_OBJ_SENTINEL, object} means store
    //  return: for fail, do nothing
    //          for success set dest[0] = MP_OBJ_NULL
    mp_attr_fun_t attr;

    // Implements load, store and delete subscripting:
    //  - value = MP_OBJ_SENTINEL means load
    //  - value = MP_OBJ_NULL means delete
    //  - all other values mean store the value
    // Can return MP_OBJ_NULL if operation not supported.
    mp_subscr_fun_t subscr;

    // Corresponds to __iter__ special method.
    // Can use the given mp_obj_iter_buf_t to store iterator object,
    // otherwise can return a pointer to an object on the heap.
    mp_getiter_fun_t getiter;

    // Corresponds to __next__ special method.  May return MP_OBJ_STOP_ITERATION
    // as an optimisation instead of raising StopIteration() with no args.
    mp_fun_1_t iternext;

    // Implements the buffer protocol if supported by this type.
    mp_buffer_p_t buffer_p;

    // One of disjoint protocols (interfaces), like mp_stream_p_t, etc.
    const void *protocol;

    // A pointer to the parents of this type:
    //  - 0 parents: pointer is NULL (object is implicitly the single parent)
    //  - 1 parent: a pointer to the type of that parent
    //  - 2 or more parents: pointer to a tuple object containing the parent types
    const void *parent;

    // A dict mapping qstrs to objects local methods/constants/etc.
    struct _mp_obj_dict_t *locals_dict;
};

-FNE mp_obj_type_t-----------------micropython/py/obj.h---------------



--------------micropython/py/obj.c-----------------------------------

bool mp_get_buffer(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    const mp_obj_type_t *type = mp_obj_get_type(obj);
    if (type->buffer_p.get_buffer == NULL) {
        return false;
    }
    int ret = type->buffer_p.get_buffer(obj, bufinfo, flags);
    if (ret != 0) {
        return false;
    }
    return true;
}

void mp_get_buffer_raise(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    if (!mp_get_buffer(obj, bufinfo, flags)) {
        mp_raise_TypeError(MP_ERROR_TEXT("object with buffer protocol required"));
    }
}

---------------------------------------------------------------------



--------------micropython/ports/stm32/bufhelper.c--------------------
#include "py/obj.h"
#include "bufhelper.h"

void pyb_buf_get_for_send(mp_obj_t o, mp_buffer_info_t *bufinfo, byte *tmp_data) {
    if (mp_obj_is_int(o)) {
        tmp_data[0] = mp_obj_get_int(o);
        bufinfo->buf = tmp_data;
        bufinfo->len = 1;
        bufinfo->typecode = 'B';
    } else {
        mp_get_buffer_raise(o, bufinfo, MP_BUFFER_READ);
    }
}

mp_obj_t pyb_buf_get_for_recv(mp_obj_t o, vstr_t *vstr) {
    if (mp_obj_is_int(o)) {
        // allocate a new bytearray of given length
        vstr_init_len(vstr, mp_obj_get_int(o));
        return MP_OBJ_NULL;
    } else {
        // get the existing buffer
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(o, &bufinfo, MP_BUFFER_WRITE);
        vstr->buf = bufinfo.buf;
        vstr->len = bufinfo.len;
        return o;
    }
}
---------------------------------------------------------------------


-INIZIO mp_uint_t-------micropython/ports/stm32/mpconfigport.h---------

// Assume that if we already defined the obj repr then we also defined these items
#ifndef MICROPY_OBJ_REPR
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned int mp_uint_t; // must be pointer size
#endif

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

-FINE mp_uint_t------------------------------------------------------------




-INIZIO mp_type_type-------------micropython/py/dynruntime.h---------
/********************************************************************/
// Types and objects

#define MP_OBJ_NEW_QSTR(x) MP_OBJ_NEW_QSTR_##x

#define mp_type_type                        (*mp_fun_table.type_type)
#define mp_type_str                         (*mp_fun_table.type_str)
#define mp_type_list                        (*mp_fun_table.type_list)
...
...
-FINE mp_type_type------------------------------------------------

--------------micropython/py/obj.h-----------------------------------
// Constant types, globally accessible
extern const mp_obj_type_t mp_type_type;
extern const mp_obj_type_t mp_type_object;
extern const mp_obj_type_t mp_type_NoneType;
extern const mp_obj_type_t mp_type_bool;
extern const mp_obj_type_t mp_type_int;

---------------------------------------------------------------------


--------------micropython/py/obj.h-----------------------------------

typedef struct _mp_buffer_p_t {
    mp_int_t (*get_buffer)(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
} mp_buffer_p_t;
bool mp_get_buffer(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
void mp_get_buffer_raise(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);

struct _mp_obj_type_t {
    // A type is an object so must start with this entry, which points to mp_type_type.
    mp_obj_base_t base;

    // Flags associated with this type.
    uint16_t flags;

    // The name of this type, a qstr.
    uint16_t name;

    // Corresponds to __repr__ and __str__ special methods.
    mp_print_fun_t print;

    // Corresponds to __new__ and __init__ special methods, to make an instance of the type.
    mp_make_new_fun_t make_new;

    // Corresponds to __call__ special method, ie T(...).
    mp_call_fun_t call;

    // Implements unary and binary operations.
    // Can return MP_OBJ_NULL if the operation is not supported.
    mp_unary_op_fun_t unary_op;
    mp_binary_op_fun_t binary_op;

    // Implements load, store and delete attribute.
    //
    // dest[0] = MP_OBJ_NULL means load
    //  return: for fail, do nothing
    //          for attr, dest[0] = value
    //          for method, dest[0] = method, dest[1] = self
    //
    // dest[0,1] = {MP_OBJ_SENTINEL, MP_OBJ_NULL} means delete
    // dest[0,1] = {MP_OBJ_SENTINEL, object} means store
    //  return: for fail, do nothing
    //          for success set dest[0] = MP_OBJ_NULL
    mp_attr_fun_t attr;

    // Implements load, store and delete subscripting:
    //  - value = MP_OBJ_SENTINEL means load
    //  - value = MP_OBJ_NULL means delete
    //  - all other values mean store the value
    // Can return MP_OBJ_NULL if operation not supported.
    mp_subscr_fun_t subscr;

    // Corresponds to __iter__ special method.
    // Can use the given mp_obj_iter_buf_t to store iterator object,
    // otherwise can return a pointer to an object on the heap.
    mp_getiter_fun_t getiter;

    // Corresponds to __next__ special method.  May return MP_OBJ_STOP_ITERATION
    // as an optimisation instead of raising StopIteration() with no args.
    mp_fun_1_t iternext;

    // Implements the buffer protocol if supported by this type.
    mp_buffer_p_t buffer_p;

    // One of disjoint protocols (interfaces), like mp_stream_p_t, etc.
    const void *protocol;

    // A pointer to the parents of this type:
    //  - 0 parents: pointer is NULL (object is implicitly the single parent)
    //  - 1 parent: a pointer to the type of that parent
    //  - 2 or more parents: pointer to a tuple object containing the parent types
    const void *parent;

    // A dict mapping qstrs to objects local methods/constants/etc.
    struct _mp_obj_dict_t *locals_dict;
};

---------------------------------------------------------------------



--------------micropython/py/dynruntime.h-----------------------------------

#define mp_get_buffer_raise(o, bufinfo, fl) (mp_fun_table.get_buffer_raise((o), (bufinfo), (fl)))





--------------micropython/py/misc.h-----------------------------------
typedef unsigned char byte;
typedef unsigned int uint;

/** variable string *********************************************/

typedef struct _vstr_t {
    size_t alloc;
    size_t len;
    char *buf;
    bool fixed_buf : 1;
} vstr_t;

// convenience macro to declare a vstr with a fixed size buffer on the stack
#define VSTR_FIXED(vstr, alloc) 
	vstr_t vstr; 
	char vstr##_buf[(alloc)]; 
	vstr_init_fixed_buf(&vstr, (alloc), vstr##_buf);


void vstr_init_len(vstr_t *vstr, size_t len);
static inline char *vstr_str(vstr_t *vstr) {
    return vstr->buf;
}
static inline size_t vstr_len(vstr_t *vstr) {
    return vstr->len;
}
---------------------------------------------------------------------


--------------micropython/py/vstr.c-----------------------------------
// Init the vstr so it allocs exactly given number of bytes.  Set length to zero.
void vstr_init(vstr_t *vstr, size_t alloc) {
    if (alloc < 1) {
        alloc = 1;
    }
    vstr->alloc = alloc;
    vstr->len = 0;
    vstr->buf = m_new(char, vstr->alloc);
    vstr->fixed_buf = false;
}

// Init the vstr so it allocs exactly enough ram to hold a null-terminated
// string of the given length, and set the length.
void vstr_init_len(vstr_t *vstr, size_t len) {
    vstr_init(vstr, len + 1);
    vstr->len = len;
}

---------------------------------------------------------------------


--------------micropython/py/misc.h-----------------------------------

#define m_new(type, num) ((type *)(m_malloc(sizeof(type) * (num))))
#define m_new_maybe(type, num) ((type *)(m_malloc_maybe(sizeof(type) * (num))))
#define m_new0(type, num) ((type *)(m_malloc0(sizeof(type) * (num))))
#define m_new_obj(type) (m_new(type, 1))
#define m_new_obj_maybe(type) (m_new_maybe(type, 1))
#define m_new_obj_var(obj_type, var_type, var_num) ((obj_type *)m_malloc(sizeof(obj_type) + sizeof(var_type) * (var_num)))
#define m_new_obj_var_maybe(obj_type, var_type, var_num) ((obj_type *)m_malloc_maybe(sizeof(obj_type) + sizeof(var_type) * (var_num)))

---------------------------------------------------------------------

--------------micropython/py/mpconfig.h-----------------------------------
// Don't use alloca calls. As alloca() is not part of ANSI C, this
// workaround option is provided for compilers lacking this de-facto
// standard function. The way it works is allocating from heap, and
// relying on garbage collection to free it eventually. This is of
// course much less optimal than real alloca().
#if defined(MICROPY_NO_ALLOCA) && MICROPY_NO_ALLOCA
#undef alloca
#define alloca(x) m_malloc(x)
#endif

SYNOPSIS         

    #include <alloca.h>

    void *alloca(size_t size);

DESCRIPTION         

    The alloca() function allocates size bytes of space in the stack
    frame of the caller.  This temporary space is automatically freed
    when the function that called alloca() returns to its caller.


--------------micropython/py/misc.h-----------------------------------

void *m_malloc(size_t num_bytes);
void *m_malloc_maybe(size_t num_bytes);
void *m_malloc_with_finaliser(size_t num_bytes);
void *m_malloc0(size_t num_bytes);

---------------------------------------------------------------------

--------------micropython/py/malloca.c-------------------------------

#if MICROPY_ENABLE_GC
#include "py/gc.h"

// We redirect standard alloc functions to GC heap - just for the rest of
// this module. In the rest of MicroPython source, system malloc can be
// freely accessed - for interfacing with system and 3rd-party libs for
// example. On the other hand, some (e.g. bare-metal) ports may use GC
// heap as system heap, so, to avoid warnings, we do undef's first.
#undef malloc
#undef free
#undef realloc
#define malloc(b) gc_alloc((b), false)
#define malloc_with_finaliser(b) gc_alloc((b), true)
#define free gc_free
#define realloc(ptr, n) gc_realloc(ptr, n, true)
#define realloc_ext(ptr, n, mv) gc_realloc(ptr, n, mv)
#else

---------------------------------------------------------------------



--------------micropython/py/nativeglue.h-------------------------------

typedef struct _mp_fun_table_t {
    mp_const_obj_t const_none;
    mp_const_obj_t const_false;
    mp_const_obj_t const_true;
    mp_uint_t (*native_from_obj)(mp_obj_t obj, mp_uint_t type);
    mp_obj_t (*native_to_obj)(mp_uint_t val, mp_uint_t type);
    mp_obj_dict_t *(*swap_globals)(mp_obj_dict_t * new_globals);
    mp_obj_t (*load_name)(qstr qst);
    mp_obj_t (*load_global)(qstr qst);
    mp_obj_t (*load_build_class)(void);
    mp_obj_t (*load_attr)(mp_obj_t base, qstr attr);
    void (*load_method)(mp_obj_t base, qstr attr, mp_obj_t *dest);
    void (*load_super_method)(qstr attr, mp_obj_t *dest);
    void (*store_name)(qstr qst, mp_obj_t obj);
    void (*store_global)(qstr qst, mp_obj_t obj);
    void (*store_attr)(mp_obj_t base, qstr attr, mp_obj_t val);
    mp_obj_t (*obj_subscr)(mp_obj_t base, mp_obj_t index, mp_obj_t val);
    bool (*obj_is_true)(mp_obj_t arg);
    mp_obj_t (*unary_op)(mp_unary_op_t op, mp_obj_t arg);
    mp_obj_t (*binary_op)(mp_binary_op_t op, mp_obj_t lhs, mp_obj_t rhs);
    mp_obj_t (*new_tuple)(size_t n, const mp_obj_t *items);
    mp_obj_t (*new_list)(size_t n, mp_obj_t *items);
    mp_obj_t (*new_dict)(size_t n_args);
    mp_obj_t (*new_set)(size_t n_args, mp_obj_t *items);
    void (*set_store)(mp_obj_t self_in, mp_obj_t item);
    mp_obj_t (*list_append)(mp_obj_t self_in, mp_obj_t arg);
    mp_obj_t (*dict_store)(mp_obj_t self_in, mp_obj_t key, mp_obj_t value);
    mp_obj_t (*make_function_from_raw_code)(const mp_raw_code_t *rc, mp_obj_t def_args, mp_obj_t def_kw_args);
    mp_obj_t (*call_function_n_kw)(mp_obj_t fun_in, size_t n_args_kw, const mp_obj_t *args);
    mp_obj_t (*call_method_n_kw)(size_t n_args, size_t n_kw, const mp_obj_t *args);
    mp_obj_t (*call_method_n_kw_var)(bool have_self, size_t n_args_n_kw, const mp_obj_t *args);
    mp_obj_t (*getiter)(mp_obj_t obj, mp_obj_iter_buf_t *iter);
    mp_obj_t (*iternext)(mp_obj_iter_buf_t *iter);
    unsigned int (*nlr_push)(nlr_buf_t *);
    void (*nlr_pop)(void);
    void (*raise)(mp_obj_t o);
    mp_obj_t (*import_name)(qstr name, mp_obj_t fromlist, mp_obj_t level);
    mp_obj_t (*import_from)(mp_obj_t module, qstr name);
    void (*import_all)(mp_obj_t module);
    mp_obj_t (*new_slice)(mp_obj_t start, mp_obj_t stop, mp_obj_t step);
    void (*unpack_sequence)(mp_obj_t seq, size_t num, mp_obj_t *items);
    void (*unpack_ex)(mp_obj_t seq, size_t num, mp_obj_t *items);
    void (*delete_name)(qstr qst);
    void (*delete_global)(qstr qst);
    mp_obj_t (*make_closure_from_raw_code)(const mp_raw_code_t *rc, mp_uint_t n_closed_over, const mp_obj_t *args);
    void (*arg_check_num_sig)(size_t n_args, size_t n_kw, uint32_t sig);
    void (*setup_code_state)(mp_code_state_t *code_state, size_t n_args, size_t n_kw, const mp_obj_t *args);
    mp_int_t (*small_int_floor_divide)(mp_int_t num, mp_int_t denom);
    mp_int_t (*small_int_modulo)(mp_int_t dividend, mp_int_t divisor);
    bool (*yield_from)(mp_obj_t gen, mp_obj_t send_value, mp_obj_t *ret_value);
    void *setjmp_;
    // Additional entries for dynamic runtime, starts at index 50
    void *(*memset_)(void *s, int c, size_t n);
    void *(*memmove_)(void *dest, const void *src, size_t n);
    void *(*realloc_)(void *ptr, size_t n_bytes, bool allow_move);
    int (*printf_)(const mp_print_t *print, const char *fmt, ...);
    int (*vprintf_)(const mp_print_t *print, const char *fmt, va_list args);
    #if defined(__GNUC__)
    NORETURN // Only certain compilers support no-return attributes in function pointer declarations
    #endif
    void (*raise_msg)(const mp_obj_type_t *exc_type, mp_rom_error_text_t msg);
    const mp_obj_type_t *(*obj_get_type)(mp_const_obj_t o_in);
    mp_obj_t (*obj_new_str)(const char *data, size_t len);
    mp_obj_t (*obj_new_bytes)(const byte *data, size_t len);
    mp_obj_t (*obj_new_bytearray_by_ref)(size_t n, void *items);
    mp_obj_t (*obj_new_float_from_f)(float f);
    mp_obj_t (*obj_new_float_from_d)(double d);
    float (*obj_get_float_to_f)(mp_obj_t o);
    double (*obj_get_float_to_d)(mp_obj_t o);
    void (*get_buffer_raise)(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
    const mp_stream_p_t *(*get_stream_raise)(mp_obj_t self_in, int flags);
    const mp_print_t *plat_print;
    const mp_obj_type_t *type_type;
    const mp_obj_type_t *type_str;
    const mp_obj_type_t *type_list;
    const mp_obj_type_t *type_dict;
    const mp_obj_type_t *type_fun_builtin_0;
    const mp_obj_type_t *type_fun_builtin_1;
    const mp_obj_type_t *type_fun_builtin_2;
    const mp_obj_type_t *type_fun_builtin_3;
    const mp_obj_type_t *type_fun_builtin_var;
    const mp_obj_fun_builtin_var_t *stream_read_obj;
    const mp_obj_fun_builtin_var_t *stream_readinto_obj;
    const mp_obj_fun_builtin_var_t *stream_unbuffered_readline_obj;
    const mp_obj_fun_builtin_var_t *stream_write_obj;
} mp_fun_table_t;

extern const mp_fun_table_t mp_fun_table;

---------------------------------------------------------------------

--------------micropython/py/nativeglue.c-------------------------------

// these must correspond to the respective enum in nativeglue.h
const mp_fun_table_t mp_fun_table = {
    mp_const_none,
    mp_const_false,
    mp_const_true,
    mp_native_from_obj,
    mp_native_to_obj,
    mp_native_swap_globals,
    mp_load_name,
    mp_load_global,
    mp_load_build_class,
    mp_load_attr,
    mp_load_method,
    mp_load_super_method,
    mp_store_name,
    mp_store_global,
    mp_store_attr,
    mp_obj_subscr,
    mp_obj_is_true,
    mp_unary_op,
    mp_binary_op,
    mp_obj_new_tuple,
    mp_obj_new_list,
    mp_obj_new_dict,
    mp_obj_new_set,
    mp_obj_set_store,
    mp_obj_list_append,
    mp_obj_dict_store,
    mp_make_function_from_raw_code,
    mp_native_call_function_n_kw,
    mp_call_method_n_kw,
    mp_call_method_n_kw_var,
    mp_native_getiter,
    mp_native_iternext,
    #if MICROPY_NLR_SETJMP
    nlr_push_tail,
    #else
    nlr_push,
    #endif
    nlr_pop,
    mp_native_raise,
    mp_import_name,
    mp_import_from,
    mp_import_all,
    mp_obj_new_slice,
    mp_unpack_sequence,
    mp_unpack_ex,
    mp_delete_name,
    mp_delete_global,
    mp_make_closure_from_raw_code,
    mp_arg_check_num_sig,
    mp_setup_code_state,
    mp_small_int_floor_divide,
    mp_small_int_modulo,
    mp_native_yield_from,
    #if MICROPY_NLR_SETJMP
    setjmp,
    #else
    NULL,
    #endif
    // Additional entries for dynamic runtime, starts at index 50
    memset,
    memmove,
    gc_realloc,
    mp_printf,
    mp_vprintf,
    mp_raise_msg,
    mp_obj_get_type,
    mp_obj_new_str,
    mp_obj_new_bytes,
    mp_obj_new_bytearray_by_ref,
    mp_obj_new_float_from_f,
    mp_obj_new_float_from_d,
    mp_obj_get_float_to_f,
    mp_obj_get_float_to_d,
    mp_get_buffer_raise,
    mp_get_stream_raise,
    &mp_plat_print,
    &mp_type_type,
    &mp_type_str,
    &mp_type_list,
    &mp_type_dict,
    &mp_type_fun_builtin_0,
    &mp_type_fun_builtin_1,
    &mp_type_fun_builtin_2,
    &mp_type_fun_builtin_3,
    &mp_type_fun_builtin_var,
    &mp_stream_read_obj,
    &mp_stream_readinto_obj,
    &mp_stream_unbuffered_readline_obj,
    &mp_stream_write_obj,
};
---------------------------------------------------------------------


---------------------------------------------------------------------

I2C.init(mode, *, addr=18, baudrate=400000, gencall=False, dma=False)

    Initialise the I2C bus with the given parameters:

            `mode` must be either I2C.MASTER or I2C.SLAVE

            `addr` is the 7-bit address (only sensible for a slave)

            `baudrate` is the SCL clock rate (only sensible for a master)

            `gencall` is whether to support general call mode

            `dma` is whether to allow the use of DMA for the 
            I2C transfers (note that DMA transfers have more 
            precise timing but currently do not handle bus errors properly)

I2C.is_ready(addr)

    Check if an I2C device responds to the given address. 
    Only valid when in master mode.

I2C.mem_read(data, addr, memaddr, *, timeout=5000, addr_size=8)

    Read from the memory of an I2C device:

            `data` can be an integer (number of bytes to read) 
                   or a buffer to read into

            `addr` is the I2C device address

            `memaddr` is the memory location within the I2C device

            `timeout` is the timeout in milliseconds to wait for the read

            `addr_size` selects width of memaddr: 8 or 16 bits

    Returns the read data. This is only valid in master mode.

I2C.mem_write(data, addr, memaddr, *, timeout=5000, addr_size=8)

    Write to the memory of an I2C device:

            `data` can be an integer or a buffer to write from

            `addr` is the I2C device address

            `memaddr` is the memory location within the I2C device

           `timeout` is the timeout in milliseconds to wait for the write

            `addr_size` selects width of memaddr: 8 or 16 bits

    Returns None. This is only valid in master mode.




---------------------------------------------------------------------
https://www.fe.infn.it/u/spizzo/prog09/lezioni04/direttive_def.html

La direttiva #define

La direttiva serve a definire una MACRO, ovvero un simbolo.
La sintassi con cui utilizzare la direttiva è la seguente:

#define  nome_macro  valore_macro

Convenzionalmente i nomi delle macro vengono scritti con delle lettere 
MAIUSCOLE. Il preprocessore legge la definizione della MACRO e, ogni 
volta che ne incontra il nome all'interno del file sorgente SOSTITUISCE 
al simbolo il corrispondente valore, SENZA verificare la correttezza 
sintattica dell'espressione risultante.


#include <stdio.h>
#define    AVVISO    printf("\nil programma, fino a qui, funziona in modo corretto");


int main()
{char c;
  c = getchar();
... // istruzioni
AVVISO
... // altre istruzioni
AVVISO
... // istruzioni
}

Grazie al preprocessore, ovvero effettuando la sostituzione, questo codice 
si trasforma in :


int main()
{char c;
  c = getchar();
... // istruzioni
printf("\nil programma, fino a qui, funziona in modo corretto");
... // altre istruzioni
printf("\nil programma, fino a qui, funziona in modo corretto");
... // istruzioni
}

ATTENZIONE: la sostituzione può nascondere dei problemi.

#include <stdio.h>
#define   MOLTIPLICA(a,b)   a*b

int main()
{int a=5,b=3,c;

 c = MOLTIPLICA(a,b);  // tutto OK ! diventa c = a * b;
 
 c = MOLTIPLICA( a + b, b );// ?? diventa c = a + b * b;

 return 0; }


Per ovviare al problema basta inserire delle parentesi ...


#define   MOLTIPLICA(a,b)   (a) * (b)

-----------------------------------------------------------
		REGULAR EXPRESSIONS
----------------------------------------------------------- 
Symbol 	Descriptions

. 	replaces any character

^ 	matches start of string

$ 	matches end of string

* 	matches up zero or more times the preceding character

\ 	Represent special characters

() 	Groups regular expressions

? 	Matches up exactly one character 

Le Regular Expressions (RegExp) sono un sistema di regole 
rivolte alla creazione di pattern di ricerca utili a trovare 
occorenze all'interno di un testo e a modificarle con altro testo. 
Da oltre vent'anni vengono usate in parecchi tool ed utility Unix 
come grep, sed oppure awk, spesso con piccole variazioni nella sintassi.

Le Regular Expressions sono case sensitive 
(ossia distinguono le maiuscole dalle minuscole) 
e seguono delle semplici regole di sintassi.

Forniscono la possibiltà di specificare quante volte 
può riscontrarsi il testo specificato preservando la validità della ricerca.
Le espressioni regolari operano sulle stringhe, e le stringhe più semplici 
sono costituite da singoli caratteri. 
La maggior parte dei caratteri corrisponde semplicemente a se stessa.
Alcuni caratteri assumono invece significati particolari 
e vengono chiamati metacaratteri.

Metacarattere 	Definizione

. 	identifica un singolo carattere

^ 	identifica l'inizio riga

\ 	neutralizza il significato del metacarattere seguente

$	identifica la fine della stringa

[] 	identifica qualsiasi carattere indicato tra parentesi

[^] 	identifica tutti i caratteri non specificati nell'insieme

[0-9] 	identifica ogni carattere compreso nell'intervallo (numeri o lettere)

Modificatori

? 	zero o una volta

+ 	una o più volte

* 	zero o più volte

| 	uno o l'altro elemento prima e dopo il segno |


------I2C HAL FUNCIONS-------------------------------------------

(1) HAL_StatusTypeDef HAL_I2C_IsDeviceReady(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint32_t  	Trials,
		uint32_t  	Timeout 
	) 		

Checks if target device is ready for communication.

Note:
    This function is used with Memory devices 

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure that 
                contains the configuration information for 
                the specified I2C.
    DevAddress	Target device address: The device 7 bits address 
                value in datasheet must be shifted to the 
                left before calling the interface
    Trials	Number of trials
    Timeout	Timeout duration

Return values:
    HAL	status 


(2)HAL_StatusTypeDef HAL_I2C_Master_Transmit(
		I2C_HandleTypeDef * hi2c,
		uint16_t  	DevAddress,
		uint8_t *  	pData,
		uint16_t  	Size,
		uint32_t  	Timeout 
	) 		

Transmits in master mode an amount of data in blocking mode.

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure 
                that contains the configuration information 
                for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value 
                in datasheet must be shifted to the left before calling the interface
    pData	Pointer to data buffer
    Size	Amount of data to be sent
    Timeout	Timeout duration

Return values:
    HAL	status 


(3) HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint8_t *  	pData,
		uint16_t  	Size 
	) 		

Transmit in master mode an amount of data in non-blocking mode with DMA.

Parameters:

    hi2c	Pointer to a I2C_HandleTypeDef structure that contains
                the configuration information for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value in 
                datasheet must be shifted to the left before calling the interface
    pData	Pointer to data buffer
    Size	Amount of data to be sent

Return values:
    HAL	status 


(4)HAL_StatusTypeDef HAL_I2C_Master_Receive(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint8_t *  	pData,
		uint16_t  	Size,
		uint32_t  	Timeout 
	) 		

Receives in master mode an amount of data in blocking mode.

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure that contains 
                the configuration information for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value 
                in datasheet must be shifted to the left before 
                calling the interface
    pData	Pointer to data buffer
    Size	Amount of data to be sent
    Timeout	Timeout duration

Return values:
    HAL	status 





(5)HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint8_t *  	pData,
		uint16_t  	Size 
	) 		

Receive in master mode an amount of data in non-blocking mode with DMA.

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure that contains 
 		the configuration information for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value 
                in datasheet must be shifted to the left before calling 
                the interface
    pData	Pointer to data buffer
    Size	Amount of data to be sent

Return values:
    HAL	status 

(6)HAL_StatusTypeDef HAL_I2C_Mem_Read(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint16_t  	MemAddress,
		uint16_t  	MemAddSize,
		uint8_t *  	pData,
		uint16_t  	Size,
		uint32_t  	Timeout 
	) 		

Read an amount of data in blocking mode from a specific memory address.

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure 
                that contains the configuration information for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value in 
                datasheet must be shifted to the left before calling the interface
    MemAddress	Internal memory address
    MemAddSize	Size of internal memory address
    pData	Pointer to data buffer
    Size	Amount of data to be sent
    Timeout	Timeout duration

Return values:
    HAL	status 



(7)HAL_StatusTypeDef HAL_I2C_Mem_Write(
		I2C_HandleTypeDef *  	hi2c,
		uint16_t  	DevAddress,
		uint16_t  	MemAddress,
		uint16_t  	MemAddSize,
		uint8_t *  	pData,
		uint16_t  	Size,
		uint32_t  	Timeout 
	) 		

Write an amount of data in blocking mode to a specific memory address.

Parameters:
    hi2c	Pointer to a I2C_HandleTypeDef structure that 
                contains the configuration information for the specified I2C.
    DevAddress	Target device address: The device 7 bits address value 
                in datasheet must be shifted to the left before calling the interface
    MemAddress	Internal memory address
    MemAddSize	Size of internal memory address
    pData	Pointer to data buffer
    Size	Amount of data to be sent
    Timeout	Timeout duration

Return values:
    HAL	status 
 

