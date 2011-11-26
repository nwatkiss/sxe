/* Copyright 2010 Sophos Limited. All rights reserved. Sophos is a registered
 * trademark of Sophos Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMTPD_TEST_COMMON_H__
#define __SMTPD_TEST_COMMON_H__

#include "tap.h"
#include "sxe-smtpd.h"
#include "sxe-test.h"
#include "sxe-util.h"

#define TEST_WAIT 5.0

extern tap_ev_queue q_client;
extern tap_ev_queue q_smtpd;

void client_connect(SXE * this);
void client_read(SXE * this, int length);
void client_sent(SXE * this, SXE_RETURN result);
void client_close(SXE * this);

SXE_SMTPD_CLIENT * c_connect_wait_banner(SXE * this, SXE * server, const char *banner);
void c_send_expect_error(SXE * this, const char *s, const char *exp);
void c_send_wait(SXE * this, const char *s);

void h_connect(SXE_SMTPD_CLIENT *client);
void h_close(SXE_SMTPD_CLIENT *client);
void h_reset(struct SXE_SMTPD_CLIENT *client);
void h_helo(struct SXE_SMTPD_CLIENT *client, const char *helo, unsigned helo_len);
void h_from(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *from);
void h_rcpt(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *rcpt);
void h_data_start(struct SXE_SMTPD_CLIENT *client);
void h_data_chunk(struct SXE_SMTPD_CLIENT *client, const char *chunk, unsigned chunk_len);
void h_data_end(struct SXE_SMTPD_CLIENT *client);

#endif
