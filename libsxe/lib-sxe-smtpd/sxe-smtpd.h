/* Copyright 2011 Sophos Limited. All rights reserved. Sophos is a registered
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

#ifndef __SXE_SMTPD_H__
#define __SXE_SMTPD_H__

#include "sxe.h"
#include "sxe-list.h"
#include "sxe-util.h"

#include "sxe-rfc822-addr.h"

typedef enum {
    SXE_SMTPD_CLIENT_FREE = 0,           /* not connected */
    SXE_SMTPD_CLIENT_BANNER,             /* waiting to send initial banner */
    SXE_SMTPD_CLIENT_COMMAND,            /* waiting to read or respond to a command */
    SXE_SMTPD_CLIENT_DATA,               /* processing a 'DATA' command, or responding to it */
    SXE_SMTPD_CLIENT_CLOSING,            /* will close imminently */
    SXE_SMTPD_CLIENT_NUMBER_OF_STATES
} SXE_SMTPD_CLIENT_STATE;

struct SXE_SMTPD;
struct SXE_SMTPD_CLIENT;

typedef void (*sxe_smtpd_connect_handler)(struct SXE_SMTPD_CLIENT *);
typedef void (*sxe_smtpd_close_handler)(struct SXE_SMTPD_CLIENT *);
typedef void (*sxe_smtpd_reset_handler)(struct SXE_SMTPD_CLIENT *);
typedef void (*sxe_smtpd_helo_handler)(struct SXE_SMTPD_CLIENT *, const char *helo, unsigned helo_len);
typedef void (*sxe_smtpd_from_handler)(struct SXE_SMTPD_CLIENT *, const SXE_RFC822_ADDRESS *from);
typedef void (*sxe_smtpd_rcpt_handler)(struct SXE_SMTPD_CLIENT *, const SXE_RFC822_ADDRESS *rcpt);
typedef void (*sxe_smtpd_data_start_handler)(struct SXE_SMTPD_CLIENT *);
typedef void (*sxe_smtpd_data_chunk_handler)(struct SXE_SMTPD_CLIENT *, const char *chunk, unsigned chunk_len);
typedef void (*sxe_smtpd_data_end_handler)(struct SXE_SMTPD_CLIENT *);

typedef void (*sxe_smtpd_on_sent_handler)(struct SXE_SMTPD_CLIENT *, SXE_RETURN);

#define SXE_SMTPD_FLAG_EARLY_TALKER 0x00000001
#define SXE_SMTPD_FLAG_BAD_COMMAND  0x00000002
#define SXE_SMTPD_FLAG_HAVE_HELO    0x00000100
#define SXE_SMTPD_FLAG_HAVE_FROM    0x00000200
#define SXE_SMTPD_FLAG_HAVE_RCPT    0x00000400

#define SXE_SMTPD_MASK_TLS_CLEAR    0x00000F00

/* Limits from RFC 2821 */
#define SXE_SMTPD_COMMAND_LENGTH_LIMIT             512
#define SXE_SMTPD_RESPONSE_LENGTH_LIMIT            512
#define SXE_SMTPD_TEXT_LINE_LENGTH_LIMIT          1000
#define SXE_SMTPD_HELO_LENGTH_LIMIT                512   /* not used */

typedef struct SXE_SMTPD_CLIENT {
    struct SXE_SMTPD         * server;
    SXE                      * sxe;                  /* NOT FOR APPLICATION USE! */
    unsigned                   flags;
    unsigned                   errors;
    int                        extended;
    int                        bol;
    char                       response[SXE_SMTPD_RESPONSE_LENGTH_LIMIT + 1];
    sxe_smtpd_on_sent_handler  on_sent;
    union {
        void                 * as_ptr;            /* not used by sxe_smtpd -- to be used by applications */
        uintptr_t              as_int;            /* not used by sxe_smtpd -- to be used by applications */
    }                          user_data;
} SXE_SMTPD_CLIENT;

typedef struct SXE_SMTPD_EXTENSION {
    const char    * name;
    SXE_LIST_NODE   node;
} SXE_SMTPD_EXTENSION;

#define SXE_SMTPD_CMD_STARTTLS      0x00000001

typedef struct SXE_SMTPD {
    SXE_SMTPD_CLIENT              * clients;
    SXE_LIST                        extensions;
    unsigned                        commands;
    sxe_smtpd_connect_handler       on_connect;
    sxe_smtpd_reset_handler         on_reset;
    sxe_smtpd_close_handler         on_close;
    sxe_smtpd_helo_handler          on_helo;
    sxe_smtpd_from_handler          on_from;
    sxe_smtpd_rcpt_handler          on_rcpt;
    sxe_smtpd_data_start_handler    on_data_start;
    sxe_smtpd_data_chunk_handler    on_data_chunk;
    sxe_smtpd_data_end_handler      on_data_end;
    union {
        void                      * as_ptr;
        uintptr_t                   as_int;
    }                               user_data;
} SXE_SMTPD;

#define SXE_SMTP_SERVICEREADY   220     /* <domain> Service ready */
#define SXE_SMTP_CLOSING        221     /* <domain> Service closing transmission channel */
#define SXE_SMTP_ACTION_OK      250     /* Requested mail action okay, completed */
#define SXE_SMTP_VRFY_OK        252     /* Cannot VRFY, but you can try RCPT if you like */
#define SXE_SMTP_STARTDATA      354     /* Start mail input; end with <CRLF>.<CRLF> */
#define SXE_SMTP_CLOSING_TMP    421     /* <domain> Service closing transmission channel */
#define SXE_SMTP_EDISKFULL_TMP  450     /* Requested mail action not taken: mailbox unavailable */
#define SXE_SMTP_ISCREWEDUP_TMP 451     /* Requested action aborted: local error in processing */
#define SXE_SMTP_ESOMELIMIT_TMP 452     /* Requested action not taken: insufficient system storage */
#define SXE_SMTP_SYNCOMMAND     500     /* Syntax error, command unrecognised */
#define SXE_SMTP_SYNARG         501     /* Syntax error in parameters or arguments */
#define SXE_SMTP_NOSUCHCOMMAND  502     /* Command not implemented */
#define SXE_SMTP_BADSEQUENCE    503     /* Bad sequence of commands */
#define SXE_SMTP_NOSUCHARG      504     /* Command parameter not implemented */
#define SXE_SMTP_IDONTDOMAIL    521     /* <domain> does not accept mail (see rfc1846) */
#define SXE_SMTP_ACCESSDENIED   530     /* Access denied (???a Sendmailism) */
#define SXE_SMTP_NOSUCHUSER     550     /* Requested action not taken: mailbox unavailable */
#define SXE_SMTP_USERNOTLOCAL   551     /* User not local; please try <forward-path> */
#define SXE_SMTP_MSGTOOBIG      552     /* Requested mail action aborted: exceeded storage allocation */
#define SXE_SMTP_BADADDRESS     553     /* Requested action not taken: mailbox name not allowed */
#define SXE_SMTP_ACTIONFAILED   554     /* Transaction failed */

#define SXE_SMTP_TOO_MANY_ERRORS                "Error: too many errors"
#define SXE_SMTP_SYNTAX_ERROR                   "Error: bad syntax"
#define SXE_SMTP_STARTDATA_MSG                  "End data with <CR><LF>.<CR><LF>"

#define SXE_ESMTP_GENERIC_OK                    "2.0.0 Ok"
#define SXE_ESMTP_ACCEPTED_AS                   "2.0.0 Ok: queued as %s"
#define SXE_ESMTP_GENERIC_BYE                   "2.0.0 Bye"
#define SXE_ESMTP_STARTTLS_GOAHEAD              "2.0.0 Go ahead"    /* XXX: is 2.0.0 correct? */
#define SXE_ESMTP_GOOD_VRFY_ADDR                "2.1.0 VRFY is disabled"
#define SXE_ESMTP_GOOD_SENDER_ADDR              "2.1.0 Ok"      /* good sender address! */
#define SXE_ESMTP_GOOD_RECIPIENT_ADDR           "2.1.5 Ok"      /* good recipient address! */
#define SXE_ESMTP_NO_ROOM_FOR_RECIPS_TEMPFAIL   "4.5.3 Error: temporarily out of recipient buffers"
#define SXE_ESMTP_TOO_MANY_RECIPS_TEMPFAIL      "4.5.3 Error: too many recipients for local policy"
#define SXE_ESMTP_MESSAGE_TOO_BIG_TEMPFAIL      "4.3.4 Error: message too big, please try again later"
#define SXE_ESMTP_MESSAGE_TOO_BIG               "5.3.4 Error: message too big for mail system"
#define SXE_ESMTP_INVALID_COMMAND               "5.5.1 Error: invalid command"
#define SXE_ESMTP_SEQ_NEED_HELO                 "5.5.1 Error: send HELO/EHLO first"
#define SXE_ESMTP_SEQ_NESTED_MAIL               "5.5.1 Error: nested MAIL command"
#define SXE_ESMTP_SEQ_NEED_MAIL                 "5.5.1 Error: need MAIL command"
#define SXE_ESMTP_SEQ_NEED_RCPT                 "5.5.1 Error: need RCPT command"
#define SXE_ESMTP_SYNTAX_ERROR                  "5.5.2 Error: bad syntax"
#define SXE_ESMTP_COMMAND_NOT_RECOGNIZED        "5.5.2 Error: command not recognized"
#define SXE_ESMTP_DATA_LINE_TOO_LONG            "5.5.4 Error: data line too long"
#define SXE_ESMTP_BAD_RECIPIENT_ADDR            "5.1.3 Error: bad recipient address syntax"
#define SXE_ESMTP_BAD_SENDER_ADDR               "5.1.7 Error: bad sender address syntax"

#define SXE_SMTPD_SET_HANDLER(smtpd, handler, function) sxe_smtpd_set_ ## handler ## _handler((smtpd), function)

static inline int
sxe_smtpd_client_has_flag(SXE_SMTPD_CLIENT * client, unsigned flag)
{
    return (client->flags & flag) == flag;
}

static inline SXE *
sxe_smtpd_client_get_sxe(SXE_SMTPD_CLIENT * client)
{
    return client->sxe;
};

static inline int
sxe_smtpd_has_command(SXE_SMTPD * self, unsigned command)
{
    return (self->commands & command) == command;
}

#include "sxe-smtpd-proto.h"

#endif

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
