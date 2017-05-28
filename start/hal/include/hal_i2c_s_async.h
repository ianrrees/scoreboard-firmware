/**
 * \file
 *
 * \brief Async I2C Slave Hardware Abstraction Layer(HAL) declaration.
 *
 * Copyright (C) 2015-2016 Atmel Corporation. All rights reserved.
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

#ifndef _HAL_I2C_S_ASYNC_H_INCLUDED
#define _HAL_I2C_S_ASYNC_H_INCLUDED

#include <hal_io.h>
#include <utils_ringbuffer.h>
#include <hpl_i2c_s_async.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup doc_driver_hal_i2c_slave_async
 *
 * @{
 */

/**
 * \brief Forward declaration of the descriptor type
 */
struct i2c_s_async_descriptor;

/**
 * \brief I2C slave callback function type
 */
typedef void (*i2c_s_async_cb_t)(const struct i2c_s_async_descriptor *const descr);

/**
 * \brief i2c callback types
 */
enum i2c_s_async_callback_type { I2C_S_ERROR, I2C_S_TX_PENDING, I2C_S_TX_COMPLETE, I2C_S_RX_COMPLETE };

/**
 * \brief i2c callback pointers structure
 */
struct i2c_s_async_callbacks {
	i2c_s_async_cb_t error;
	i2c_s_async_cb_t tx_pending;
	i2c_s_async_cb_t tx;
	i2c_s_async_cb_t rx;
};

/**
 * \brief I2C slave descriptor structure
 */
struct i2c_s_async_descriptor {
	struct _i2c_s_async_device   device;
	struct io_descriptor         io;
	struct i2c_s_async_callbacks cbs;
	struct ringbuffer            rx;
	uint8_t *                    tx_buffer;
	uint16_t                     tx_buffer_length;
	uint16_t                     tx_por;
};

/**
 * \brief Initialize asynchronous I2C slave interface
 *
 * This function initializes the given i2c descriptor to be used as asynchronous
 * I2C slave interface descriptor. It checks if the given hardware is not
 * initialized and if the given hardware is permitted to be initialized.
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 * \param[in] hw The pointer to hardware instance
 * \param[in] rx_buffer An rx buffer
 * \param[in] rx_buffer_length The length of the buffer above
 *
 * \return Initialization status.
 */
int32_t i2c_s_async_init(struct i2c_s_async_descriptor *const descr, void *const hw, uint8_t *const rx_buffer,
                         const uint16_t rx_buffer_length);

/**
 * \brief Deinitialize asynchronous I2C slave interface
 *
 * This function deinitializes the given asynchronous I2C slave descriptor.
 * It checks if the given hardware is initialized and if the given hardware is
 * permitted to be deinitialized.
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 *
 * \return De-initialization status.
 */
int32_t i2c_s_async_deinit(struct i2c_s_async_descriptor *const descr);

/**
 * \brief Set the device address
 *
 * This function sets the I2C slave device address.
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate  through
 *                I2C
 * \param[in] address An address
 *
 * \return Status of address setting.
 */
int32_t i2c_s_async_set_addr(struct i2c_s_async_descriptor *const descr, const uint16_t addr);

/**
 * \brief Register callback function
 *
 * This function registers callback functions to I2C slave device specified
 * by the given descriptor
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 * \param[in] type Type of a callback to set
 * \param[in] func Callback function pointer
 *
 * \return Callback setting status.
 * \retval -1 Passed parameters were invalid
 * \retval 0 The callback set is completed successfully
 */
int32_t i2c_s_async_register_callback(struct i2c_s_async_descriptor *const descr,
                                      const enum i2c_s_async_callback_type type, i2c_s_async_cb_t func);

/**
 * \brief Enable I2C slave communication
 *
 * This function enables the I2C slave device
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 *
 * \return Enabling status.
 */
int32_t i2c_s_async_enable(struct i2c_s_async_descriptor *const descr);

/**
 * \brief Disable I2C slave communication
 *
 * This function disables the I2C slave device
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 *
 * \return Disabling status
 */
int32_t i2c_s_async_disable(struct i2c_s_async_descriptor *const descr);

/**
 * \brief Retrieve I/O descriptor
 *
 * This function returns a I/O instance for the given I2C slave driver instance
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *                I2C
 * \param[in] io A pointer to an I/O descriptor pointer type
 *
 * \return I/O retrieving status.
 */
int32_t i2c_s_async_get_io_descriptor(struct i2c_s_async_descriptor *const descr, struct io_descriptor **io);

/**
 * \brief Retrieve the number of received bytes in the buffer
 *
 * This function retrieves the number of received bytes which were not read out
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *
 * \return The amount of bytes received
 */
int32_t i2c_s_async_get_bytes_received(const struct i2c_s_async_descriptor *const descr);

/**
 * \brief Retrieve the number of bytes sent
 *
 * This function retrieves the number of sent bytes for the last write operation
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *
 * \return The amount of bytes sent
 */
int32_t i2c_s_async_get_bytes_sent(const struct i2c_s_async_descriptor *const descr);

/**
 * \brief Flush received data
 *
 * This function flushes all received data
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *
 * \return The status of data flushing
 */
int32_t i2c_s_async_flush_rx_buffer(struct i2c_s_async_descriptor *const descr);

/**
 * \brief Abort sending
 *
 * This function aborts data sending
 *
 * \param[in] descr An I2C slave descriptor which is used to communicate through
 *
 * \return The amount of bytes received
 */
int32_t i2c_s_async_abort_tx(struct i2c_s_async_descriptor *const descr);

/**
 * \brief Retrieve the current interface status
 *
 * \param[in]  descr An i2c descriptor which is used to communicate via USART
 * \param[out] status The state of I2C slave
 *
 * \return The status of I2C status retrieving.
 */
int32_t i2c_s_async_get_status(const struct i2c_s_async_descriptor *const descr, i2c_s_status_t *const status);

/**
 * \brief Retrieve the current driver version
 *
 * \return Current driver version
 */
uint32_t i2c_s_async_get_version(void);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* _HAL_I2C_S_ASYNC_H_INCLUDED */
