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
