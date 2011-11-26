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

#include <string.h>
#include <errno.h>

#include "test/common.h"

tap_ev_queue q_client;
tap_ev_queue q_smtpd;

void
client_connect(SXE * this)
{
    SXEE61I("%s()", __func__);
    tap_ev_queue_push(q_client, __func__, 1, "this", this);
    SXER60I("return");
}

void
client_read(SXE * this, int length)
{
    SXE_UNUSED_PARAMETER(length);
    SXEE62I("%s(length=%u)", __func__, length);
    tap_ev_queue_push(q_client, __func__, 3, "this", this, "buf", tap_dup(SXE_BUF(this), SXE_BUF_USED(this)), "used", SXE_BUF_USED(this));
    sxe_buf_clear(this);
    SXER60I("return");
}

void
client_sent(SXE * this, SXE_RETURN result)
{
    SXE_UNUSED_PARAMETER(result);
    SXEE63I("%s(this=%p,result=%s)", __func__, this, sxe_return_to_string(result));
    tap_ev_queue_push(q_client, __func__, 1, "this", this);
    SXER60I("return");
}

void
client_close(SXE * this)
{
    SXEE62I("%s(this=%p)", __func__, this);
    tap_ev_queue_push(q_client, __func__, 1, "this", this);
    SXER60("return");
}

void
c_send_wait(SXE * c, const char *s)
{
    tap_ev     ev;
    SXE_RETURN result = sxe_send(c, s, strlen(s), client_sent);

    if (result == SXE_RETURN_OK) {
        skip(1, "Client sent immediately");
    }
    else {
        is_eq(test_tap_ev_queue_identifier_wait(q_client, TEST_WAIT, &ev), "client_sent", "Client sent");
    }
}

SXE_SMTPD_CLIENT *
c_connect_wait_banner(SXE * c, SXE * listener, const char *banner)
{
    SXE_SMTPD_CLIENT * client;
    char buffer[4096];
    tap_ev ev;

    sxe_connect(c, "127.0.0.1", SXE_LOCAL_PORT(listener));
    is_eq(test_tap_ev_queue_identifier_wait(q_client, TEST_WAIT, &ev), "client_connect",      "Client connected to SMTPD");
    is_eq(test_tap_ev_queue_identifier_wait(q_smtpd, TEST_WAIT, &ev), "h_connect",            "SMTPD: connected");
    client = SXE_CAST_NOCONST(SXE_SMTPD_CLIENT *, tap_ev_arg(ev, "client"));

    /* server sends banner to client */
    sxe_smtpd_banner(client, banner, strlen(banner), NULL);
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, strlen(banner), "client");
    is_strncmp(buffer, banner, strlen(banner),                      "Client received correct banner");

    return client;
}

void
c_send_expect_error(SXE * c, const char *s, const char *expect)
{
    tap_ev ev;
    char buffer[4096];

    diag("sending %s", s);
    c_send_wait(c, s);
    diag("expecting %s", expect);
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, strlen(expect), "client");
    is_strncmp(buffer, expect, strlen(expect),                      "Client received expected response");
}

void
h_connect(SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    tap_ev_queue_push(q_smtpd, __func__, 1, "client", client);
    SXER90I("return");
}

void
h_close(SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    tap_ev_queue_push(q_smtpd, __func__, 1, "client", client);
    SXER90I("return");
}

void
h_reset(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    tap_ev_queue_push(q_smtpd, __func__, 1, "client", client);
    SXER90I("return");
}

void
h_helo(struct SXE_SMTPD_CLIENT *client, const char *helo, unsigned helo_len)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXE_UNUSED_PARAMETER(helo);
    SXE_UNUSED_PARAMETER(helo_len);
    SXEE94I("(client=%p,helo=%.*s,helo_len=%u)", client, helo_len, helo, helo_len);
    tap_ev_queue_push(q_smtpd, __func__, 2,
                      "client", client,
                      "helo", tap_dup(helo, helo_len));
    SXER90I("return");
}

void
h_from(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *from)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXE_UNUSED_PARAMETER(from);
    SXEE93I("(client=%p,from='%s@%s')", client, from->local, from->domain);
    tap_ev_queue_push(q_smtpd, __func__, 3,
                      "client", client,
                      "local", tap_dup(from->local, strlen(from->local)),
                      "domain", tap_dup(from->domain, strlen(from->domain)));
    SXER90I("return");
}

void
h_rcpt(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *rcpt)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXE_UNUSED_PARAMETER(rcpt);
    SXEE93I("(client=%p,rcpt='%s@%s')", client, rcpt->local, rcpt->domain);
    tap_ev_queue_push(q_smtpd, __func__, 3,
                      "client", client,
                      "local", tap_dup(rcpt->local, strlen(rcpt->local)),
                      "domain", tap_dup(rcpt->domain, strlen(rcpt->domain)));
    SXER90I("return");
}

void
h_data_start(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    tap_ev_queue_push(q_smtpd, __func__, 1, "client", client);
    SXER90I("return");
}

void
h_data_chunk(struct SXE_SMTPD_CLIENT *client, const char *chunk, unsigned chunk_len)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXE_UNUSED_PARAMETER(chunk);
    SXE_UNUSED_PARAMETER(chunk_len);
    SXEE94I("(client=%p,chunk=%.*s,chunk_len=%u)", client, chunk_len, chunk, chunk_len);
    tap_ev_queue_push(q_smtpd, __func__, 4,
                      "client", client,
                      "this", this,
                      "buf", tap_dup(chunk, chunk_len),
                      "used", chunk_len);
    SXER90I("return");
}

void
h_data_end(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    SXER90I("return");
}
