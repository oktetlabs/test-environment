/** @file
 * @brief TAPI TAD Internal
 *
 * Internal definitions for TAPI TAD library.
 *
 *
 * Copyright (C) 2018 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_TAD_INTERNAL_H__
#define __TE_TAPI_TAD_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print error message, set return value, go to cleanup.
 *
 * @param rc_       Function's return value to set.
 * @param err_      Error to be returned.
 * @param format_   Format string.
 * @param args_     Arguments for format string (may be empty).
 */
#define ERROR_CLEANUP(rc_, err_, format_, args_...) \
    do {                                                    \
        rc_ = err_;                                         \
        ERROR("%s(): " format_ ", rc=%r",                   \
              __FUNCTION__, ##args_, rc_);                  \
        goto cleanup;                                       \
    } while (0)

/**
 * Check if rc is nonzero, print error message and go to cleanup
 * if it is.
 *
 * @param rc_       Return value to check.
 * @param msg_      Format string and arguments for error message.
 */
#define CHECK_ERROR_CLEANUP(rc_, msg_...) \
    do {                                                         \
        if (rc_ != 0)                                            \
            ERROR_CLEANUP(rc_, rc_, msg_);                       \
    } while (0)

/**
 * Read field from packet.
 *
 * @param rc_     Function's return value to set.
 * @param pdu_    Pointer to PDU from which to read a field.
 * @param dgr_    Pointer to structure describing network packet.
 * @param dir_    Direction (prefix) of the field (src, dst)
 * @param field_  Label of desired field (port, addr).
 */
#define READ_PACKET_FIELD(rc_, pdu_, dgr_, dir_, field_) \
    do {                                                              \
        size_t len_;                                                  \
                                                                      \
        len_ = sizeof((dgr_)-> dir_ ## _ ## field_ );                 \
        rc_ = asn_read_value_field(                                   \
                    pdu_,                                             \
                    &((dgr_)-> dir_ ## _ ## field_ ),                 \
                    &len_, # dir_ "-" # field_);                      \
        CHECK_ERROR_CLEANUP(rc_, "failed to read %s field",           \
                            # dir_ "-" # field_);                     \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAD_INTERNAL_H__ */
