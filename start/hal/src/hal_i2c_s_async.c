/**
 * \file
 *
 * \brief I2C Slave related functionality implementation.
 *
 * Copyright (C) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#include <hal_i2c_s_async.h>
#include <hal_atomic.h>
#include <utils_ringbuffer.h>
#include <utils.h>

/**
 * \brief Driver version
 */
#define DRIVER_VERSION 0x00000001u

static int32_t i2c_s_async_write(struct io_descriptor *const io_descr, const uint8_t *const buf, const uint16_t length);
static int32_t i2c_s_async_read(struct io_descriptor *const io_descr, uint8_t *const buf, const uint16_t length);

static void i2c_s_async_tx(struct _i2c_s_async_device *const device);
static void i2c_s_async_byte_received(struct _i2c_s_async_device *const device, const uint8_t data);
static void i2c_s_async_error(struct _i2c_s_async_device *const device);

/**
 * \brief Initialize asynchronous i2c slave interface
 */
int32_t i2c_s_async_init(struct i2c_s_async_descriptor *const descr, void *const hw, uint8_t *const rx_buffer,
                         const uint16_t rx_buffer_length)
{
	int32_t init_status;

	ASSERT(descr && hw && rx_buffer && rx_buffer_length);

	if (ERR_NONE != ringbuffer_init(&descr->rx, rx_buffer, rx_buffer_length)) {
		return ERR_INVALID_ARG;
	}

	init_status = _i2c_s_async_init(&descr->device, hw);
	if (init_status) {
		return init_status;
	}

	descr->io.read  = i2c_s_async_read;
	descr->io.write = i2c_s_async_write;

	descr->device.cb.error   = i2c_s_async_error;
	descr->device.cb.tx      = i2c_s_async_tx;
	descr->device.cb.rx_done = i2c_s_async_byte_received;

	descr->tx_por           = 0;
	descr->tx_buffer_length = 0;

	return ERR_NONE;
}

/**
 * \brief Deinitialize asynchronous i2c slave interface
 */
int32_t i2c_s_async_deinit(struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return _i2c_s_async_deinit(&descr->device);
}

/**
 * \brief Enable I2C slave communication
 */
int32_t i2c_s_async_enable(struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return _i2c_s_async_enable(&descr->device);
}

/**
 * \brief Disable I2C slave communication
 */
int32_t i2c_s_async_disable(struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return _i2c_s_async_disable(&descr->device);
}

/**
 * \brief Set the device address
 */
int32_t i2c_s_async_set_addr(struct i2c_s_async_descriptor *const descr, const uint16_t address)
{
	ASSERT(descr);

	if (!_i2c_s_async_is_10bit_addressing_on(&descr->device)) {
		return _i2c_s_async_set_address(&descr->device, address & 0x7F);
	}

	return _i2c_s_async_set_address(&descr->device, address);
}

/**
 * \brief Register callback function
 */
int32_t i2c_s_async_register_callback(struct i2c_s_async_descriptor *const descr,
                                      const enum i2c_s_async_callback_type type, i2c_s_async_cb_t func)
{
	ASSERT(descr);

	switch (type) {
	case I2C_S_ERROR:
		descr->cbs.error = func;
		_i2c_s_async_set_irq_state(&descr->device, I2C_S_DEVICE_ERROR, func != NULL);
		break;
	case I2C_S_TX_PENDING:
		descr->cbs.tx_pending = func;
		_i2c_s_async_set_irq_state(&descr->device, I2C_S_DEVICE_TX, func != NULL);
		break;
	case I2C_S_TX_COMPLETE:
		descr->cbs.tx = func;
		_i2c_s_async_set_irq_state(&descr->device, I2C_S_DEVICE_TX, func != NULL);
		break;
	case I2C_S_RX_COMPLETE:
		descr->cbs.rx = func;
		_i2c_s_async_set_irq_state(&descr->device, I2C_S_DEVICE_RX_COMPLETE, func != NULL);
		break;
	default:
		return ERR_INVALID_DATA;
	}

	return ERR_NONE;
}

/**
 * \brief Retrieve I/O descriptor
 */
int32_t i2c_s_async_get_io_descriptor(struct i2c_s_async_descriptor *const descr, struct io_descriptor **io)
{
	ASSERT(descr && io);

	*io = &descr->io;

	return ERR_NONE;
}

/**
 * \brief Return the number of received bytes in the buffer
 */
int32_t i2c_s_async_get_bytes_received(const struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return ringbuffer_num((struct ringbuffer *)&descr->rx);
}

/**
 * \brief Retrieve the number of bytes sent
 */
int32_t i2c_s_async_get_bytes_sent(const struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return descr->tx_por;
}

/**
 * \brief Flush received data
 */
int32_t i2c_s_async_flush_rx_buffer(struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return ringbuffer_flush(&descr->rx);
}

/**
 * \brief Abort sending
 */
int32_t i2c_s_async_abort_tx(struct i2c_s_async_descriptor *const descr)
{
	ASSERT(descr);

	return _i2c_s_async_abort_transmission(&descr->device);
}

/**
 * \brief Retrieve the current interface status
 */
int32_t i2c_s_async_get_status(const struct i2c_s_async_descriptor *const descr, i2c_s_status_t *const status)
{
	ASSERT(descr && status);

	*status = _i2c_s_async_get_status(&descr->device);

	return ERR_NONE;
}

/**
 * \brief Retrieve the current driver version
 */
uint32_t i2c_s_async_get_version(void)
{
	return DRIVER_VERSION;
}

/**
 * \internal Callback function for data sending
 *
 * \param[in] device The pointer to i2c slave device
 */
static void i2c_s_async_tx(struct _i2c_s_async_device *const device)
{
	struct i2c_s_async_descriptor *descr = CONTAINER_OF(device, struct i2c_s_async_descriptor, device);

	if (!descr->tx_buffer_length) {
		if (descr->cbs.tx_pending) {
			descr->cbs.tx_pending(descr);
		}
	} else if (++descr->tx_por != descr->tx_buffer_length) {
		_i2c_s_async_write_byte(&descr->device, descr->tx_buffer[descr->tx_por]);
	} else {
		descr->tx_por           = 0;
		descr->tx_buffer_length = 0;
		if (descr->cbs.tx) {
			descr->cbs.tx(descr);
		}
	}
}

/**
 * \internal Callback function for data receipt
 *
 * \param[in] device The pointer to i2c slave device
 */
static void i2c_s_async_byte_received(struct _i2c_s_async_device *const device, const uint8_t data)
{
	struct i2c_s_async_descriptor *descr = CONTAINER_OF(device, struct i2c_s_async_descriptor, device);

	ringbuffer_put(&descr->rx, data);

	if (descr->cbs.rx) {
		descr->cbs.rx(descr);
	}
}

/**
 * \internal Callback function for error
 *
 * \param[in] device The pointer to i2c slave device
 */
static void i2c_s_async_error(struct _i2c_s_async_device *const device)
{
	struct i2c_s_async_descriptor *descr = CONTAINER_OF(device, struct i2c_s_async_descriptor, device);

	if (descr->cbs.error) {
		descr->cbs.error(descr);
	}
}

/*
 * \internal Read data from i2c slave interface
 *
 * \param[in] descr The pointer to an io descriptor
 * \param[in] buf A buffer to read data to
 * \param[in] length The size of a buffer
 *
 * \return The number of bytes read.
 */
static int32_t i2c_s_async_read(struct io_descriptor *const io, uint8_t *const buf, const uint16_t length)
{
	uint16_t                       was_read = 0;
	uint32_t                       num;
	struct i2c_s_async_descriptor *descr = CONTAINER_OF(io, struct i2c_s_async_descriptor, io);

	ASSERT(io && buf && length);

	CRITICAL_SECTION_ENTER()
	num = ringbuffer_num(&descr->rx);
	CRITICAL_SECTION_LEAVE()

	while ((was_read < num) && (was_read < length)) {
		ringbuffer_get(&descr->rx, &buf[was_read++]);
	}

	return (int32_t)was_read;
}

/*
 * \internal Write the given data to i2c slave interface
 *
 * \param[in] descr The pointer to an io descriptor
 * \param[in] buf Data to write to usart
 * \param[in] length The number of bytes to write
 *
 * \return The number of bytes written or -1 if another write operation is in
 *         progress.
 */
static int32_t i2c_s_async_write(struct io_descriptor *const io, const uint8_t *const buf, const uint16_t length)
{
	struct i2c_s_async_descriptor *descr = CONTAINER_OF(io, struct i2c_s_async_descriptor, io);

	ASSERT(io && buf && length);

	if (descr->tx_por != descr->tx_buffer_length) {
		return ERR_BUSY;
	}

	descr->tx_buffer        = (uint8_t *)buf;
	descr->tx_buffer_length = length;
	_i2c_s_async_write_byte(&descr->device, descr->tx_buffer[0]);

	return (int32_t)length;
}
