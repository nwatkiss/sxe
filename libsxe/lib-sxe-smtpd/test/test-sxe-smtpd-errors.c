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

#define TXT150  "abcdefghijklmnopqrstuvwxyzaaabacadaeafagahaiajakalamanaoapaqarasatauavawaxa" \
                "yazbabbbcbdbebfbgbhbibjbkblbmbnbobpbqbrbsbtbubvbwbxbybzcacbcccdcecfcgchcicj"
#define TXT1500 TXT150 TXT150 TXT150 TXT150 TXT150 TXT150 TXT150 TXT150 TXT150 TXT150

int
main(void)
{
    SXE_SMTPD                smtpd;
    SXE_SMTPD_CLIENT       * client;
    SXE                    * listener;
    SXE                    * c;
    tap_ev                   ev;

    tap_plan(293, TAP_FLAG_ON_FAILURE_EXIT, NULL);
    sxe_register(4, 0);        /* smtp listener and connections */
    sxe_register(8, 0);        /* smtp clients */
    sxe_init();

    q_client = tap_ev_queue_new();
    q_smtpd = tap_ev_queue_new();

    sxe_smtpd_construct(&smtpd, 3, 0);

    SXE_SMTPD_SET_HANDLER(&smtpd, connect,    h_connect);
    SXE_SMTPD_SET_HANDLER(&smtpd, close,      h_close);
    SXE_SMTPD_SET_HANDLER(&smtpd, reset,      h_reset);
    SXE_SMTPD_SET_HANDLER(&smtpd, helo,       h_helo);
    SXE_SMTPD_SET_HANDLER(&smtpd, from,       h_from);
    SXE_SMTPD_SET_HANDLER(&smtpd, rcpt,       h_rcpt);
    SXE_SMTPD_SET_HANDLER(&smtpd, data_start, h_data_start);
    SXE_SMTPD_SET_HANDLER(&smtpd, data_chunk, h_data_chunk);
    SXE_SMTPD_SET_HANDLER(&smtpd, data_end,   h_data_end);

    listener = sxe_smtpd_listen(&smtpd, "0.0.0.0", 0);

    c = sxe_new_tcp(NULL, "0.0.0.0", 0, client_connect, client_read, client_close);

    /* new SMTP client connects */
    client = c_connect_wait_banner(c, listener, "test-sxe-smtpd-events/0.01 ESMTP blah blah\r\n");

#define TC(s, r) c_send_expect_error(c, s "\r\n", r "\r\n")

    TC("HELO"                           , "501 Error: bad syntax");
    TC("HEL"                            , "500 5.5.2 Error: command not recognized");
    TC("\t \t\t"                        , "500 5.5.2 Error: bad syntax");
    TC("STOP"                           , "500 5.5.2 Error: command not recognized");
    TC("VRFY"                           , "501 5.5.2 Error: bad syntax");
    TC("VRFY arg"                       , "501 5.1.3 Error: bad recipient address syntax");
    TC("VRFY <a>"                       , "501 5.1.3 Error: bad recipient address syntax");
    TC("VRFY <@foo>"                    , "501 5.1.3 Error: bad recipient address syntax");
    TC("VRFY <bar@foo>"                 , "252 2.1.0 VRFY is disabled");
    TC("VERIFY"                         , "500 5.5.2 Error: command not recognized");
    TC("VERY"                           , "500 5.5.2 Error: command not recognized");
    TC("VRFYY"                          , "500 5.5.2 Error: command not recognized");
    TC("vrfy"                           , "501 5.5.2 Error: bad syntax");
    TC("vrfy arg"                       , "501 5.1.3 Error: bad recipient address syntax");
    TC("vrfy <a>"                       , "501 5.1.3 Error: bad recipient address syntax");
    TC("vrfy <@foo>"                    , "501 5.1.3 Error: bad recipient address syntax");
    TC("vrfy <bar@foo>"                 , "252 2.1.0 VRFY is disabled");
    TC("verify"                         , "500 5.5.2 Error: command not recognized");
    TC("very"                           , "500 5.5.2 Error: command not recognized");
    TC("vrfyy"                          , "500 5.5.2 Error: command not recognized");
    TC("RSET arg"                       , "501 5.5.2 Error: bad syntax");
    TC("RSETTLER"                       , "500 5.5.2 Error: command not recognized");
    TC("ROSE"                           , "500 5.5.2 Error: command not recognized");
    TC("EhLo"                           , "501 Error: bad syntax");
    TC("eHlO   \t\t \t "                , "501 Error: bad syntax");
    TC("EHLOdude"                       , "500 5.5.2 Error: command not recognized");
    TC("EXPRESS"                        , "500 5.5.2 Error: command not recognized");
    TC("\t \t helO   \t\t \t "          , "501 Error: bad syntax");
    TC("HeLOdude"                       , "500 5.5.2 Error: command not recognized");
    TC("HELL"                           , "500 5.5.2 Error: command not recognized");
    TC("MAIL"                           , "501 5.5.2 Error: bad syntax");
    TC("\t \t maIL   \t\t \t "          , "501 5.5.2 Error: bad syntax");
    TC("mail,man"                       , "500 5.5.2 Error: command not recognized");
    TC("MASK"                           , "500 5.5.2 Error: command not recognized");
    TC("MAILFROM"                       , "500 5.5.2 Error: command not recognized");
    TC("MAIL from"                      , "501 5.5.2 Error: bad syntax");
    TC("mail FROM:"                     , "501 5.5.2 Error: bad syntax");
    TC("MAIL FROMMISH:"                 , "501 5.5.2 Error: bad syntax");
    TC("mail FRoMmIsH:"                 , "501 5.5.2 Error: bad syntax");
    TC("mail FrOmPy:"                   , "501 5.5.2 Error: bad syntax");
    TC("mail from:<"                    , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from: <"                   , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:\t<"                  , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:>"                    , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from: >"                   , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:\t>"                  , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:+"                    , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:-"                    , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:@$!#$*&)@(*#&$)(*@&#)*&@)#*&)@#&$)*@&#$",
                                          "501 5.1.7 Error: bad sender address syntax");
    TC("mail from:\01\02\03\04\05\06\07", "501 5.1.7 Error: bad sender address syntax");
    TC("mail from: <a>"                 , "501 5.1.7 Error: bad sender address syntax");
    TC("mail from: <@foo>"              , "501 5.1.7 Error: bad sender address syntax");
    TC("rcpt"                           , "501 5.5.2 Error: bad syntax");
    TC("\t \t rcpt   \t\t \t "          , "501 5.5.2 Error: bad syntax");
    TC("rcpt,man"                       , "500 5.5.2 Error: command not recognized");
    TC("MASK"                           , "500 5.5.2 Error: command not recognized");
    TC("rcptFROM"                       , "500 5.5.2 Error: command not recognized");
    TC("rcpt from"                      , "501 5.5.2 Error: bad syntax");
    TC("rcpt FROM:"                     , "501 5.5.2 Error: bad syntax");
    TC("rcpt FROMMISH:"                 , "501 5.5.2 Error: bad syntax");
    TC("rcpt FRoMmIsH:"                 , "501 5.5.2 Error: bad syntax");
    TC("rcpt topPy:"                    , "501 5.5.2 Error: bad syntax");
    TC("rcpt to: <a>"                   , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to: <@foo>"                , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:"                       , "501 5.5.2 Error: bad syntax");
    TC("rcpt to:"                       , "501 5.5.2 Error: bad syntax");
    TC("rcpt to:<"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:>"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:a"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:@"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:."                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:\r"                     , "501 5.5.2 Error: bad syntax");
    TC("rcpt to:\n"                     , "501 5.5.2 Error: bad syntax");
    TC("rcpt to:,"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("rcpt to:+"                      , "501 5.1.3 Error: bad recipient address syntax");
    TC("DAPPER"                         , "500 5.5.2 Error: command not recognized");
    TC("day"                            , "500 5.5.2 Error: command not recognized");
    TC("DATABASE"                       , "500 5.5.2 Error: command not recognized");
    TC("DATA BASE"                      , "501 5.5.2 Error: bad syntax");
    TC("QUITE"                          , "500 5.5.2 Error: command not recognized");
    TC("qui"                            , "500 5.5.2 Error: command not recognized");
    TC("quiet"                          , "500 5.5.2 Error: command not recognized");
    TC("quit arg"                       , "501 5.5.2 Error: bad syntax");
    TC("ziggy"                          , "500 5.5.2 Error: command not recognized");
    TC("NOOP arg"                       , "501 5.5.2 Error: bad syntax");
    TC("NOOPLET"                        , "500 5.5.2 Error: command not recognized");
    TC("nOPE"                           , "500 5.5.2 Error: command not recognized");
    TC("STARTTLS"                       , "500 5.5.2 Error: command not recognized");

    TC("DATA"                           , "503 5.5.1 Error: need RCPT command");
    TC("RCPT TO:<a@example.com>"        , "503 5.5.1 Error: need MAIL command");
    TC("MAIL FROM:<a@example.com>"      , "503 5.5.1 Error: send HELO/EHLO first");

    client->flags |= SXE_SMTPD_FLAG_HAVE_HELO;
    client->flags |= SXE_SMTPD_FLAG_HAVE_FROM;
    TC("MAIL FROM:<a@example.com>"      , "503 5.5.1 Error: nested MAIL command");

    /* Command too short to make sense */
    TC("A"                              , "500 5.5.2 Error: command not recognized");
    TC("AB"                             , "500 5.5.2 Error: command not recognized");
    TC("ABC"                            , "500 5.5.2 Error: command not recognized");

    /* Longer than 1500 bytes without a \r\n: error */
    TC(TXT1500 TXT150                   , "500 5.5.2 Error: bad syntax");

    /* Rudely close the client connection */
    sxe_close(c);
    is_eq(test_tap_ev_queue_identifier_wait(q_smtpd, TEST_WAIT, &ev), "h_close", "SMTPD: close");

    return exit_status();
}

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
