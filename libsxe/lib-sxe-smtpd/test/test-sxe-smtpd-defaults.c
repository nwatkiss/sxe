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

int
main(void)
{
    SXE_SMTPD                smtpd;
    tap_ev                   ev;
    SXE                    * listener;
    SXE                    * c;
    char                     buffer[4096];

    tap_plan(21, TAP_FLAG_ON_FAILURE_EXIT, NULL);
    sxe_register(4, 0);        /* smtp listener and connections */
    sxe_register(8, 0);        /* smtp clients */
    sxe_init();

    q_client = tap_ev_queue_new();
    q_smtpd = tap_ev_queue_new();

    sxe_smtpd_construct(&smtpd, 3, 0);

    listener = sxe_smtpd_listen(&smtpd, "0.0.0.0", 0);

    c = sxe_new_tcp(NULL, "0.0.0.0", 0, client_connect, client_read, client_close);

    /* new SMTP client connects */
    sxe_connect(c, "127.0.0.1", SXE_LOCAL_PORT(listener));
    is_eq(test_tap_ev_queue_identifier_wait(q_client, TEST_WAIT, &ev), "client_connect",      "Client connected to SMTPD");
#define BANNER "localhost ESMTP lib-sxe-smtpd\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(BANNER), "client");
    is_strncmp(buffer, BANNER, SXE_LITERAL_LENGTH(BANNER),                      "Client received correct response");

    /* RSET */
    SXE_WRITE_LITERAL(c, "RSET\r\n");
#define RSET_RESPONSE "250 2.0.0 Ok\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(RSET_RESPONSE), "client");
    is_strncmp(buffer, RSET_RESPONSE, SXE_LITERAL_LENGTH(RSET_RESPONSE),        "Client received correct response");

    /* HELO */
    SXE_WRITE_LITERAL(c, "HELO localhost\r\n");
#define HELO_RESPONSE "250 localhost\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(HELO_RESPONSE), "client");
    is_strncmp(buffer, HELO_RESPONSE, SXE_LITERAL_LENGTH(HELO_RESPONSE),        "Client received correct response");

    /* MAIL FROM */
    SXE_WRITE_LITERAL(c, "MAIL FROM:<>\r\n");
#define FROM_RESPONSE "250 2.1.0 Ok\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(FROM_RESPONSE), "client");
    is_strncmp(buffer, FROM_RESPONSE, SXE_LITERAL_LENGTH(FROM_RESPONSE),        "Client received correct response");

    /* RCPT TO */
    SXE_WRITE_LITERAL(c, "RCPT TO:test.o'user@example.com\r\n");
#define RCPT_RESPONSE "250 2.1.5 Ok\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(RCPT_RESPONSE), "client");
    is_strncmp(buffer, RCPT_RESPONSE, SXE_LITERAL_LENGTH(RCPT_RESPONSE),        "Client received correct response");

    /* NOOP */
    SXE_WRITE_LITERAL(c, "NOOP\r\n");
#define NOOP_RESPONSE "250 2.0.0 Ok\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(NOOP_RESPONSE), "client");
    is_strncmp(buffer, NOOP_RESPONSE, SXE_LITERAL_LENGTH(NOOP_RESPONSE),        "Client received correct response");


    /* DATA */
    SXE_WRITE_LITERAL(c, "DATA\r\n");
#define DATA_RESPONSE "354 End data with <CR><LF>.<CR><LF>\r\n"
    test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(DATA_RESPONSE), "client");
    is_strncmp(buffer, DATA_RESPONSE, SXE_LITERAL_LENGTH(DATA_RESPONSE),        "Client received correct response");

    /* Message */
    {
        const char message[] =
            "From: Test O'User <test.o'user@example.com>\r\n"
            "To: Test O'User <test.o'user@example.com\r\n"
            "Subject: This is a test\r\n"
            "Message-Id: <9608ebc12b0dcfac257dd071357e3c2c@example.com>\r\n"
            "Content-Type: text/plain; charset=\"UTF-8\""
            "\r\n"
            "SMTP is an interesting protocol. It has enabled countless trillions of\r\n"
            "electronic messages to wing their way around the internet, yet its\r\n"
            "designers never considered many critical realities of the current\r\n"
            "world: security, identity, and unsolicited commercial mail (SPAM).\r\n"
            "\r\n"
            "Despite these shortcomings, SMTP has been extended in ways that do\r\n"
            "mitigate its most severe shortcomings somewhat. Some of the most\r\n"
            ".promising extensions are:\r\n"
            "\r\n"
            "   - DomainKeys\r\n"
            "   - SPF\r\n"
            "   - S/MIME and PGP\r\n"
            "   - TLS (SMTP over encrypted communications)\r\n"
            "   - SMTP AUTH\r\n"
            "\r\n"
            "Several prominent critics take the position that fixing SMTP is the\r\n"
            "wrong goal: SMTP is so broken, they say, that it cannot be fixed. The\r\n"
            "reality, however, is that SMTP is here to stay for the forseeable\r\n"
            "future, so we had better deal with it, and ensure our SMTP technology\r\n"
            "is up to the challenge.\r\n"
            ".\r\n"
            ;
        c_send_wait(c, message);

#define MESSAGE_RESPONSE "250 2.0.0 Ok: queued as deadbeefdeadbeefdeadbeefdeadbeef\r\n"
        test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(MESSAGE_RESPONSE), "client");
        is_strncmp(buffer, MESSAGE_RESPONSE, SXE_LITERAL_LENGTH(MESSAGE_RESPONSE),        "Client received correct response");
    }

    /* QUIT */
    {
        SXE_WRITE_LITERAL(c, "QUIT\r\n");
#define QUIT_RESPONSE "221 2.0.0 Bye\r\n"
        test_ev_queue_wait_read(q_client, TEST_WAIT, &ev, c, "client_read", buffer, SXE_LITERAL_LENGTH(QUIT_RESPONSE), "client");
        is_strncmp(buffer, QUIT_RESPONSE, SXE_LITERAL_LENGTH(QUIT_RESPONSE),        "Client received correct response");
        is_eq(test_tap_ev_queue_identifier_wait(q_client, TEST_WAIT, &ev), "client_close", "Client closed");
    }

    return exit_status();
}

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
