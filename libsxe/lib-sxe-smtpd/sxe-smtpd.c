#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "sxe.h"
#include "sxe-log.h"
#include "sxe-pool.h"

#include "sxe-smtpd.h"

static inline unsigned
sxe_smtpd_client_id(SXE_SMTPD_CLIENT * pool, SXE_SMTPD_CLIENT * client)
{
    unsigned client_id = (SXE_CAST(uintptr_t, client) - SXE_CAST(uintptr_t, pool)) / sizeof(*client);
    SXEL93("sxe_smtpd_client_id(pool=%p,client=%p) // return %u", pool, client, client_id);
    return client_id;
}

/*******
 * Debugging: convert states into names.
 */
const char *
sxe_smtpd_state_to_string(SXE_SMTPD_CLIENT_STATE state)
{
    switch (state) {
        case SXE_SMTPD_CLIENT_FREE:     return "FREE";
        case SXE_SMTPD_CLIENT_BANNER:   return "BANNER";
        case SXE_SMTPD_CLIENT_COMMAND:  return "COMMAND";
        case SXE_SMTPD_CLIENT_DATA:     return "DATA";
        case SXE_SMTPD_CLIENT_CLOSING:  return "CLOSING";
        default:
            return NULL;
    }
}

/*******
 * Default handlers, and their set methods
 */

static void
sxe_smtpd_default_connect_handler(SXE_SMTPD_CLIENT *client)
{
    SXE        * this     = client->sxe;
    const char   banner[] = "localhost ESMTP lib-sxe-smtpd\r\n";

    SXE_UNUSED_PARAMETER(this);
    SXEE91I("(client=%p)", client);
    sxe_smtpd_banner(client, banner, sizeof(banner) - 1, NULL);
    SXER90I("return");
}

sxe_smtpd_connect_handler
sxe_smtpd_set_connect_handler(SXE_SMTPD *self, sxe_smtpd_connect_handler new_handler)
{
    sxe_smtpd_connect_handler old_handler = self->on_connect;
    self->on_connect = new_handler ? new_handler : sxe_smtpd_default_connect_handler;
    return old_handler;
}

static void
sxe_smtpd_default_close_handler(SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    SXER90I("return");
}

sxe_smtpd_close_handler
sxe_smtpd_set_close_handler(SXE_SMTPD *self, sxe_smtpd_close_handler new_handler)
{
    sxe_smtpd_close_handler old_handler = self->on_close;
    self->on_close = new_handler ? new_handler : sxe_smtpd_default_close_handler;
    return old_handler;
}

static void
sxe_smtpd_default_reset_handler(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXEE91I("(client=%p)", client);
    sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, SXE_ESMTP_GENERIC_OK);
    SXER90I("return");
}

sxe_smtpd_reset_handler
sxe_smtpd_set_reset_handler(struct SXE_SMTPD * self, sxe_smtpd_reset_handler new_handler)
{
    sxe_smtpd_reset_handler old_handler = self->on_reset;
    self->on_reset = new_handler ? new_handler : sxe_smtpd_default_reset_handler;
    return old_handler;
}

static void
sxe_smtpd_default_helo_handler(struct SXE_SMTPD_CLIENT *client, const char *helo, unsigned helo_len)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(helo);
    SXE_UNUSED_PARAMETER(helo_len);
    SXEE94I("(client=%p,helo=%.*s,helo_len=%u)", client, helo_len, helo, helo_len);
    sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, "localhost");
    SXER90I("return");
}

sxe_smtpd_helo_handler
sxe_smtpd_set_helo_handler(struct SXE_SMTPD * self, sxe_smtpd_helo_handler new_handler)
{
    sxe_smtpd_helo_handler old_handler = self->on_helo;
    self->on_helo = new_handler ? new_handler : sxe_smtpd_default_helo_handler;
    return old_handler;
}

static void
sxe_smtpd_default_from_handler(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *from)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(from);
    SXEE93I("(client=%p,from='%s@%s')", client, from->local, from->domain);
    sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, SXE_ESMTP_GOOD_SENDER_ADDR);
    SXER90I("return");
}

sxe_smtpd_from_handler
sxe_smtpd_set_from_handler(struct SXE_SMTPD * self, sxe_smtpd_from_handler new_handler)
{
    sxe_smtpd_from_handler old_handler = self->on_from;
    self->on_from = new_handler ? new_handler : sxe_smtpd_default_from_handler;
    return old_handler;
}

static void
sxe_smtpd_default_rcpt_handler(struct SXE_SMTPD_CLIENT *client, const SXE_RFC822_ADDRESS *rcpt)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(rcpt);
    SXEE93I("(client=%p,rcpt='%s@%s')", client, rcpt->local, rcpt->domain);
    sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, SXE_ESMTP_GOOD_RECIPIENT_ADDR);
    SXER90I("return");
}

sxe_smtpd_rcpt_handler
sxe_smtpd_set_rcpt_handler(struct SXE_SMTPD * self, sxe_smtpd_rcpt_handler new_handler)
{
    sxe_smtpd_rcpt_handler old_handler = self->on_rcpt;
    self->on_rcpt = new_handler ? new_handler : sxe_smtpd_default_rcpt_handler;
    return old_handler;
}

static void
sxe_smtpd_default_data_start_handler(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXEE91I("(client=%p)", client);
    sxe_smtpd_respond(client, SXE_SMTP_STARTDATA, SXE_SMTP_STARTDATA_MSG);
    SXER90I("return");
}

sxe_smtpd_data_start_handler
sxe_smtpd_set_data_start_handler(struct SXE_SMTPD * self, sxe_smtpd_data_start_handler new_handler)
{
    sxe_smtpd_data_start_handler old_handler = self->on_data_start;
    self->on_data_start = new_handler ? new_handler : sxe_smtpd_default_data_start_handler;
    return old_handler;
}

static void
sxe_smtpd_default_data_chunk_handler(struct SXE_SMTPD_CLIENT *client, const char *chunk, unsigned chunk_len)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXE_UNUSED_PARAMETER(client);
    SXE_UNUSED_PARAMETER(chunk);
    SXE_UNUSED_PARAMETER(chunk_len);
    SXEE94I("(client=%p,chunk=%.*s,chunk_len=%u)", client, chunk_len, chunk, chunk_len);
    SXER90I("return");
}

sxe_smtpd_data_chunk_handler
sxe_smtpd_set_data_chunk_handler(struct SXE_SMTPD * self, sxe_smtpd_data_chunk_handler new_handler)
{
    sxe_smtpd_data_chunk_handler old_handler = self->on_data_chunk;
    self->on_data_chunk = new_handler ? new_handler : sxe_smtpd_default_data_chunk_handler;
    return old_handler;
}

static void
sxe_smtpd_default_data_end_handler(struct SXE_SMTPD_CLIENT *client)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(this);
    SXEE91I("(client=%p)", client);
    sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, SXE_ESMTP_ACCEPTED_AS, "deadbeefdeadbeefdeadbeefdeadbeef");
    SXER90I("return");
}

sxe_smtpd_data_end_handler
sxe_smtpd_set_data_end_handler(struct SXE_SMTPD * self, sxe_smtpd_data_end_handler new_handler)
{
    sxe_smtpd_data_end_handler old_handler = self->on_data_end;
    self->on_data_end = new_handler ? new_handler : sxe_smtpd_default_data_end_handler;
    return old_handler;
}

static void
sxe_smtpd_event_connect(SXE * this)
{
    SXE_SMTPD         * self = SXE_USER_DATA(this);
    SXE_SMTPD_CLIENT  * client;
    unsigned            id;

    SXE_UNUSED_PARAMETER(this);
    SXEE80I("()");

    id = sxe_pool_set_oldest_element_state(self->clients, SXE_SMTPD_CLIENT_FREE, SXE_SMTPD_CLIENT_BANNER);
    SXEA10I(id != SXE_POOL_NO_INDEX, "Ran out of connections; should not happen");

    client = &self->clients[id];
    memset(client, '\0', sizeof(*client));
    SXE_USER_DATA(this) = client;
    client->sxe = this;
    client->server = self;

    (*self->on_connect)(client);

    /* TODO: reap oldest connection to make room for next new client */

    SXER80I("return");
}

#define SKIPWS(ptr, end)        while ((ptr) < (end) && isspace(*ptr)) ++ptr

static bool
parse_cmd(SXE_SMTPD_CLIENT * client, const char ** ptr, const char * crlf, const char * cmd)
{
    SXE        * this   = client->sxe;
    bool         result = false;
    int          arg    = 0;
    const char * syntax = (*cmd == 'E' || *cmd == 'H') ? SXE_SMTP_SYNTAX_ERROR : SXE_ESMTP_SYNTAX_ERROR;

    SXE_UNUSED_PARAMETER(this);
    SXEE81I("(cmd='%s')", cmd);

    for (; *cmd && *ptr < crlf; cmd++) {
        if (isspace(*cmd)) {
            arg = 1;
            if (!isspace(**ptr)) {
                sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
                goto SXE_EARLY_OUT;
            }
            SKIPWS(*ptr, crlf);
        }
        else if ('$' == *cmd) {
            if (isspace(**ptr)) {
                SKIPWS(*ptr, crlf);
                /* AHA! We should never hit a '$' unless there is only
                 * whitespace until the crlf! */
                if (*ptr != crlf) {
                    sxe_smtpd_respond(client, SXE_SMTP_SYNARG, "%s", syntax);
                    goto SXE_EARLY_OUT;
                }
            }
            else {
                sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
                goto SXE_EARLY_OUT;
            }
        }
        else if ('.' == *cmd) {
            if (isspace(**ptr)) {
                SKIPWS(*ptr, crlf);
                --cmd; /* redo */
            }
        }
        else if (tolower(*cmd) != tolower(**ptr)) {
            goto SXE_ERROR_OUT;
        }
        else
            ++*ptr;
    }

    /* buffer shorter than required */
    if (*cmd && *cmd != '$') {
        if (isspace(*cmd)) {
            arg = 1;
        }
        goto SXE_ERROR_OUT;
    }

    result = true;

SXE_ERROR_OUT:
    if (!result) {
        if (arg)
            sxe_smtpd_respond(client, SXE_SMTP_SYNARG, "%s", syntax);
        else
            sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
    }

SXE_EARLY_OUT:
    SXER81I("return result=%d", result);
    return result;
}

static int
parse_helo(const char *ptr, const char *end, unsigned *helo_len)
{
    while (ptr > end && isspace(ptr[-1])) --ptr;
    *helo_len = (unsigned)(end - ptr);
    return (*helo_len >= 1 && *helo_len <= SXE_SMTPD_HELO_LENGTH_LIMIT);
}

static void
sxe_smtpd_client_starttls_done(SXE * this)
{
    SXE_SMTPD_CLIENT * client = SXE_USER_DATA(this);
    SXE_SMTPD        * self   = client->server;
    SXEE80I("()");
    SXEL50I("TLS session established");

    client->flags &= ~SXE_SMTPD_MASK_TLS_CLEAR;

    (*self->on_reset)(client);

    SXER80I("return");
}

static void
sxe_smtpd_client_starttls(SXE_SMTPD_CLIENT * client, SXE_RETURN result)
{
    SXE * this = client->sxe;
    SXE_UNUSED_PARAMETER(result); /* XXX really? */
    SXEE81I("(result=%s)", sxe_return_to_string(result));
    this->in_event_connected = sxe_smtpd_client_starttls_done;
    sxe_ssl_accept(this);
    SXER80("return");
}

static void
sxe_smtpd_handle_command(SXE_SMTPD_CLIENT * client)
{
    SXE_RFC822_ADDRESS  addr;
    SXE_SMTPD          * self = client->server;
    SXE                * this = client->sxe;
    unsigned             used = SXE_BUF_USED(this);
    const char         * buf  = SXE_BUF(this);
    const char         * crlf = sxe_strnstr(buf, "\r\n", used);
    unsigned             len;

    SXEE80I("()");

    if (crlf == NULL) {
        /* Didn't find \r\n: need to read more data. Check whether
         * we're out of space: if so, we throw out the entire
         * thing and set the "bad_command" flag so that we won't
         * accept the command whenever it finally finishes. */
        if (used >= SXE_BUF_SIZE) {
            client->flags |= SXE_SMTPD_FLAG_BAD_COMMAND;
            sxe_buf_clear(this);
        }

        SXEL60I("CRLF not found; waiting for more data");
        goto SXE_EARLY_OUT;
    }

    /* Pre-consume the line we're parsing. This leaves the data there, but
     * marks it as writable during the next read event. Since we're not doing
     * true asynchronous reads, it's fine to refer to this data until this
     * function returns. */
    sxe_buf_consume(this, crlf - buf + SXE_LITERAL_LENGTH("\r\n"));
    sxe_pause(this);

    if (sxe_smtpd_client_has_flag(client, SXE_SMTPD_FLAG_BAD_COMMAND)) {
        sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_SYNTAX_ERROR);
        goto SXE_EARLY_OUT;
    }

    if (used < 4 || used > SXE_SMTPD_COMMAND_LENGTH_LIMIT) {
        sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
        goto SXE_EARLY_OUT;
    }

    SKIPWS(buf, crlf);
    if (buf == crlf) {
        sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_SYNTAX_ERROR);
        goto SXE_EARLY_OUT;
    }

    switch (*buf) {
    case 'D':
    case 'd':
        if (!parse_cmd(client, &buf, crlf, "DATA$"))
            goto SXE_EARLY_OUT;
        if (!sxe_smtpd_client_has_flag(client, SXE_SMTPD_FLAG_HAVE_RCPT)) {
            sxe_smtpd_respond(client, SXE_SMTP_BADSEQUENCE, SXE_ESMTP_SEQ_NEED_RCPT);
            goto SXE_EARLY_OUT;
        }

        (*self->on_data_start)(client);
        break;

    case 'E':
    case 'e':
        if (!parse_cmd(client, &buf, crlf, "EHLO "))
            goto SXE_EARLY_OUT;
        if (!parse_helo(buf, crlf, &len)) {
            sxe_smtpd_respond(client, SXE_SMTP_SYNARG, SXE_SMTP_SYNTAX_ERROR);
            goto SXE_EARLY_OUT;
        }

        client->flags |= SXE_SMTPD_FLAG_HAVE_HELO;
        client->extended = 1;
        (*self->on_helo)(client, buf, len);
        break;

    case 'H':
    case 'h':
        if (!parse_cmd(client, &buf, crlf, "HELO "))
            goto SXE_EARLY_OUT;
        if (!parse_helo(buf, crlf, &len)) {
            sxe_smtpd_respond(client, SXE_SMTP_SYNARG, SXE_SMTP_SYNTAX_ERROR);
            goto SXE_EARLY_OUT;
        }

        client->flags |= SXE_SMTPD_FLAG_HAVE_HELO;
        (*self->on_helo)(client, buf, len);
        break;

    case 'M':
    case 'm':
        if (!parse_cmd(client, &buf, crlf, "MAIL FROM:."))
            goto SXE_EARLY_OUT;

        /* parse address */
        memset(&addr, 0, sizeof(addr));
        if ((buf + 1) < crlf && 0 == strncmp(buf, "<>", 2))
            addr.nullpath = 1;
        else {
            used = sxe_rfc822_parse_address(&addr, buf, crlf - buf);
            if (!used) {
                sxe_smtpd_respond(client, SXE_SMTP_SYNARG, SXE_ESMTP_BAD_SENDER_ADDR);
                goto SXE_EARLY_OUT;
            }
        }

        if (!sxe_smtpd_client_has_flag(client, SXE_SMTPD_FLAG_HAVE_HELO)) {
            sxe_smtpd_respond(client, SXE_SMTP_BADSEQUENCE, SXE_ESMTP_SEQ_NEED_HELO);
            goto SXE_EARLY_OUT;
        }
        if (sxe_smtpd_client_has_flag(client, SXE_SMTPD_FLAG_HAVE_FROM)) {
            sxe_smtpd_respond(client, SXE_SMTP_BADSEQUENCE, SXE_ESMTP_SEQ_NESTED_MAIL);
            goto SXE_EARLY_OUT;
        }

        client->flags |= SXE_SMTPD_FLAG_HAVE_FROM;
        (*self->on_from)(client, &addr);
        break;

    case 'N':
    case 'n':
        if (parse_cmd(client, &buf, crlf, "NOOP$")) {
            sxe_smtpd_respond(client, SXE_SMTP_ACTION_OK, SXE_ESMTP_GENERIC_OK);
        }
        goto SXE_EARLY_OUT;
        break;

    case 'R':
    case 'r':
        if (buf[1] == 'S') {
            if (!parse_cmd(client, &buf, crlf, "RSET$"))
                goto SXE_EARLY_OUT;

            (*self->on_reset)(client);
            break;
        }

        if (!parse_cmd(client, &buf, crlf, "RCPT TO:."))
            goto SXE_EARLY_OUT;

        memset(&addr, '\0', sizeof addr);
        used = sxe_rfc822_parse_address(&addr, buf, crlf - buf);
        if (!used) {
            sxe_smtpd_respond(client, SXE_SMTP_SYNARG, SXE_ESMTP_BAD_RECIPIENT_ADDR);
            goto SXE_EARLY_OUT;
        }

        if (!sxe_smtpd_client_has_flag(client, SXE_SMTPD_FLAG_HAVE_FROM)) {
            sxe_smtpd_respond(client, SXE_SMTP_BADSEQUENCE, SXE_ESMTP_SEQ_NEED_MAIL);
            goto SXE_EARLY_OUT;
        }

        client->flags |= SXE_SMTPD_FLAG_HAVE_RCPT;
        (*self->on_rcpt)(client, &addr);
        break;

    case 'q':
    case 'Q':
        if (!parse_cmd(client, &buf, crlf, "QUIT$"))
            goto SXE_EARLY_OUT;
        sxe_smtpd_respond(client, SXE_SMTP_CLOSING, SXE_ESMTP_GENERIC_BYE);
        break;

    case 's':
    case 'S':
        if (!parse_cmd(client, &buf, crlf, "STARTTLS$"))
            goto SXE_EARLY_OUT;
        if (!sxe_smtpd_has_command(self, SXE_SMTPD_CMD_STARTTLS)) {
            sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
            goto SXE_EARLY_OUT;
        }

        client->on_sent = sxe_smtpd_client_starttls;
        sxe_smtpd_respond(client, SXE_SMTP_SERVICEREADY, SXE_ESMTP_STARTTLS_GOAHEAD);

        break;

    case 'v':
    case 'V':
        if (!parse_cmd(client, &buf, crlf, "VRFY ."))
            goto SXE_EARLY_OUT;

        memset(&addr, '\0', sizeof addr);
        used = sxe_rfc822_parse_address(&addr, buf, crlf - buf);
        if (!used) {
             sxe_smtpd_respond(client, SXE_SMTP_SYNARG, SXE_ESMTP_BAD_RECIPIENT_ADDR);
             goto SXE_EARLY_OUT;
        }

        sxe_smtpd_respond(client, SXE_SMTP_VRFY_OK, SXE_ESMTP_GOOD_VRFY_ADDR);
        break;

    default:
        sxe_smtpd_respond(client, SXE_SMTP_SYNCOMMAND, SXE_ESMTP_COMMAND_NOT_RECOGNIZED);
        break;
    }

SXE_EARLY_OUT:
    SXER80I("return");
}

static void
sxe_smtpd_handle_data_chunk(SXE_SMTPD_CLIENT * client)
{
    SXE              * this = client->sxe;
    SXE_SMTPD        * self = client->server;
    SXE_SMTPD_CLIENT * pool = self->clients;
    unsigned           client_id = sxe_smtpd_client_id(pool, client);
    const char       * ptr;
    const char       * copyfrom;
    const char       * end;
    int                bol;

    SXEE80I("()");

    bol      = client->bol;
    ptr      = SXE_BUF(this);
    end      = ptr + SXE_BUF_USED(this);
    copyfrom = ptr;

    while (ptr < end) {
        if (*ptr == '.' && bol) {
            if ((ptr + 2) < end && 0 == strncmp(ptr, ".\r\n", 3)) {
                ptr += 3;
                sxe_buf_consume(this, ptr - SXE_BUF(this));
                sxe_pause(this);

                sxe_pool_set_indexed_element_state(pool, client_id, SXE_SMTPD_CLIENT_DATA, SXE_SMTPD_CLIENT_COMMAND);

                (*self->on_data_end)(client);

                break;
            }

            if (ptr == end - 1 || ((ptr + 1) == end && *(ptr + 1) == '\r')) {
                /* wait for more data: this might be the EOM */
                break;
            }

            ++copyfrom; /* skip '.' */
            bol = 0;
        }

        if (*ptr == '\r' && (ptr + 1) < end && 0 == strncmp(ptr, "\r\n", 2)) {
            size_t len;

            ptr += 2;
            len = ptr - copyfrom;

            (*self->on_data_chunk)(client, copyfrom, len);

            sxe_buf_consume(this, ptr - SXE_BUF(this));

            bol      = 1;
            ptr      = SXE_BUF(this);
            end      = ptr + SXE_BUF_USED(this);
            copyfrom = ptr;
        }
        else {
            bol = 0; /* next character is not at the beginning of a line */
            ++ptr;
        }
    }

    client->bol = bol;

    SXER80I("return");
}

static void
sxe_smtpd_event_read(SXE * this, int additional_length)
{
    SXE_SMTPD_CLIENT * client = SXE_USER_DATA(this);
    SXE_SMTPD        * server;
    SXE_SMTPD_CLIENT * client_pool;
    unsigned           client_id;
    unsigned           state;

    SXE_UNUSED_PARAMETER(additional_length);
    SXEE82I("(client=%p,additional_length=%u)", client, additional_length);
    server      = client->server;
    client_pool = server->clients;
    client_id   = sxe_smtpd_client_id(client_pool, client);
    state       = sxe_pool_index_to_state(client_pool, client_id);

    switch (state) {
    case SXE_SMTPD_CLIENT_BANNER:
        client->flags |= SXE_SMTPD_FLAG_EARLY_TALKER;
        /* fall through */
    case SXE_SMTPD_CLIENT_COMMAND:
        sxe_smtpd_handle_command(client);
        break;
    case SXE_SMTPD_CLIENT_DATA:
        sxe_smtpd_handle_data_chunk(client);
        break;

    case SXE_SMTPD_CLIENT_CLOSING:
        /*
         * The 'closing' state is only entered when sxe_smtpd_respond() is
         * passed SXE_SMTP_CLOSING or SXE_SMTP_CLOSING_TMP, and only stays in
         * that state while sxe_send() is in progress. Since we disable read
         * events while we're sending, and the on_complete() function closes
         * the socket if the state is SXE_SMTPD_CLIENT_CLOSING, it is an
         * internal error to receive read events in the CLOSING state.
         */
    case SXE_SMTPD_CLIENT_FREE:
        /* Obviously it is an error to receive events on a free socket. */
    default:
        SXEA11I(0, "Unexpected read event in state %s", sxe_smtpd_state_to_string(state));  /* Coverage Exclusion: no need to test read in invalid state */
    }

    SXER80I("return");
}

static void
sxe_smtpd_event_close(SXE * this)
{
    SXE_SMTPD_CLIENT * client = SXE_USER_DATA(this);
    SXE_SMTPD        * self;
    SXE_SMTPD_CLIENT * client_pool;
    unsigned           client_id;
    unsigned           state;

    SXEE81I("(client=%p)", client);
    self        = client->server;
    client_pool = self->clients;
    client_id   = sxe_smtpd_client_id(client_pool, client);
    state       = sxe_pool_index_to_state(client_pool, client_id);

    (*self->on_close)(client);

    sxe_pool_set_indexed_element_state(client_pool, client_id, state, SXE_SMTPD_CLIENT_FREE);

    SXER80I("return");
}

static void
sxe_smtpd_close(SXE_SMTPD_CLIENT * client)
{
    SXE              * this = client->sxe;
    SXE_SMTPD        * self = client->server;
    SXE_SMTPD_CLIENT * pool = self->clients;
    unsigned           client_id;
    unsigned           state;

    SXEE80I("()");

    client_id  = sxe_smtpd_client_id(pool, client);
    state      = sxe_pool_index_to_state(pool, client_id);
    sxe_pool_set_indexed_element_state(pool, client_id, state, SXE_SMTPD_CLIENT_FREE);

    (*self->on_close)(client);

    sxe_close(this);

    SXER80I("return");
}

static void
sxe_smtpd_resume(SXE_SMTPD_CLIENT * client)
{
    SXE              * this = client->sxe;
    SXE_SMTPD        * self = client->server;
    SXE_SMTPD_CLIENT * pool = self->clients;
    unsigned           client_id;
    unsigned           state;

    SXE_UNUSED_PARAMETER(this);
    SXEE80I("()");
    client_id  = sxe_smtpd_client_id(pool, client);
    state      = sxe_pool_index_to_state(pool, client_id);

    if (state == SXE_SMTPD_CLIENT_CLOSING) {
        sxe_smtpd_close(client);
        goto SXE_EARLY_OUT;
    }

    sxe_buf_resume(this, SXE_BUF_RESUME_IMMEDIATE);

    client->flags &= ~SXE_SMTPD_FLAG_BAD_COMMAND;

SXE_EARLY_OUT:
    SXER80I("return");
}

static void
sxe_smtpd_response_sent(SXE * this, SXE_RETURN result)
{
    SXE_SMTPD_CLIENT * client = SXE_USER_DATA(this);

    SXE_UNUSED_PARAMETER(this);
    SXEE81I("(result=%s)", sxe_return_to_string(result));

    if (result != SXE_RETURN_OK) {
        /* close */
    }

    if (client->on_sent) {
        (*client->on_sent)(client, result);
        client->on_sent = NULL;
    }

    sxe_smtpd_resume(client);

    SXER80I("return");
}

/**
 * Sends the SMTP banner greeting to the SMTP client.
 *
 * @param client     The SMTP client
 * @param banner     The banner to send to the client.
 * @param banner_len The banner length.
 * @param on_sent    A callback to invoke when sending the banner is complete
 *                   (can be used to free application resources -- although
 *                   presumably the banner is a fixed resource).
 *
 * @return           The result of sxe_send(). SXE_RETURN_OK indicates that
 *                   the banner was sent immediately -- the callback will not
 *                   be invoked in that case.
 */
SXE_RETURN
sxe_smtpd_banner(SXE_SMTPD_CLIENT * client, const char *banner, unsigned banner_len, sxe_smtpd_on_sent_handler on_sent)
{
    SXE              * this = client->sxe;
    SXE_SMTPD        * self = client->server;
    SXE_SMTPD_CLIENT * pool = self->clients;
    SXE_RETURN         result;
    unsigned           client_id;
    unsigned           state;

    SXE_UNUSED_PARAMETER(this);
    SXEE83I("(banner=%p,banner_len=%u,on_sent=%p)", banner, banner_len, on_sent);
    SXEA10I(banner != NULL, "Banner cannot be NULL");
    SXEA10I(banner_len != 0, "Banner length cannot be zero");

    client_id  = sxe_smtpd_client_id(pool, client);
    state      = sxe_pool_index_to_state(pool, client_id);

    SXEA12I(state == SXE_SMTPD_CLIENT_BANNER, "sxe_smtpd_banner() called in state %s, not state %s",
            sxe_smtpd_state_to_string(state), sxe_smtpd_state_to_string(SXE_SMTPD_CLIENT_BANNER));

    sxe_pool_set_indexed_element_state(pool, client_id, SXE_SMTPD_CLIENT_BANNER, SXE_SMTPD_CLIENT_COMMAND);

    client->on_sent = on_sent;
    result = sxe_send(this, banner, banner_len, sxe_smtpd_response_sent);

    if (result == SXE_RETURN_OK) {
        sxe_smtpd_response_sent(this, result);
    }

    SXER81I("return %s", sxe_return_to_string(result));
    return result;
}

void
sxe_smtpd_register_extension(SXE_SMTPD * self, SXE_SMTPD_EXTENSION * extension, const char * name)
{
    SXEE81("(name=%s)", name);
    extension->name = name;
    sxe_list_push(&self->extensions, extension);
    if (strcmp(name, "STARTTLS") == 0) {
        self->commands |= SXE_SMTPD_CMD_STARTTLS;
    }
    SXER81("return // extensions=%u", SXE_LIST_GET_LENGTH(&self->extensions));
}

/**
 * Sends a response to an SMTP client's EHLO command, using the provided
 * format string as the first line, and adding one extra line per registered
 * extension.
 *
 * @param client       The SMTP client
 * @param fmt          The response format (use SXE_ESMTP_{SOMETHING}).
 * @param ...          The response format arguments.
 *
 * @return             The result of sxe_send(). SXE_RETURN_OK indicates that
 *                     the response was sent immediately; otherwise, the data
 *                     is queued and will be sent asynchronously. If the send
 *                     eventually fails, the connection will be closed, and
 *                     the close callback will be invoked.
 *
 * @note               Register SMTP extensions with sxe_smtpd_register_extension().
 */

__printflike(2, 3)
SXE_RETURN
sxe_smtpd_respond_ehlo(SXE_SMTPD_CLIENT * client, const char *fmt, ...)
{
    SXE_LIST_WALKER       ext_walker;
    SXE_SMTPD_EXTENSION * ext;
    SXE_SMTPD_EXTENSION * last;
    SXE                 * this = client->sxe;
    SXE_SMTPD           * self = client->server;
    char                * resp = client->response;
    unsigned              rlen = sizeof(client->response);
    unsigned              ltot = 0;
    unsigned              len;
    SXE_RETURN            result;
    va_list               ap;

    SXE_UNUSED_PARAMETER(this);
    SXEE81I("(fmt=%s,...)", fmt);
    SXEA10I(fmt != NULL, "format string cannot be NULL");

    len = snprintf(resp, rlen, "%d-", SXE_SMTP_ACTION_OK);
    resp += len;
    ltot += len;
    rlen -= len;

    va_start(ap, fmt);
    len = vsnprintf(resp, rlen, fmt, ap);
    resp += len;
    ltot += len;
    rlen -= len;
    va_end(ap);

    len = snprintf(resp, rlen, "\r\n%d-ENHANCEDSTATUSCODES\r\n%d-PIPELINING\r\n",
                   SXE_SMTP_ACTION_OK, SXE_SMTP_ACTION_OK);
    resp += len;
    ltot += len;
    rlen -= len;

    last = sxe_list_peek_tail(&self->extensions);
    sxe_list_walker_construct(&ext_walker, &self->extensions);
    for (ext = sxe_list_walker_step(&ext_walker);
         ext != NULL && ltot <= sizeof(client->response);
         ext = sxe_list_walker_step(&ext_walker))
    {
        len = snprintf(resp, rlen, "%d%c%s\r\n",
                       SXE_SMTP_ACTION_OK,
                       ext == last ? ' ' : '-',
                       ext->name);
        resp += len;
        ltot += len;
        rlen -= len;
    }

    SXEA10I(ltot <= sizeof(client->response), "Too many extensions registered to fit in a response!");

    result = sxe_send(this, client->response, ltot, sxe_smtpd_response_sent);

    if (result == SXE_RETURN_OK) {
        sxe_smtpd_response_sent(this, result);
    }

    SXER81I("return %s", sxe_return_to_string(result));
    return result;
}

/**
 * Sends a response to an SMTP client.
 *
 * @param client       The SMTP client
 * @param code         The response code (use SXE_SMTP_{SOMETHING}).
 * @param fmt          The response format (use SXE_ESMTP_{SOMETHING}).
 * @param ...          The response format arguments.
 *
 * @return             The result of sxe_send(). SXE_RETURN_OK indicates that
 *                     the response was sent immediately; otherwise, the data
 *                     is queued and will be sent asynchronously. If the send
 *                     eventually fails, the connection will be closed, and
 *                     the close callback will be invoked.
 */
__printflike(3, 4)
SXE_RETURN
sxe_smtpd_respond(SXE_SMTPD_CLIENT * client, int code, const char *fmt, ...)
{
    SXE              * this = client->sxe;
    SXE_SMTPD        * self = client->server;
    SXE_SMTPD_CLIENT * pool = self->clients;
    char             * resp = client->response;
    unsigned           rlen = sizeof(client->response);
    unsigned           ltot = 0;
    unsigned           len;
    SXE_RETURN         result;
    va_list            ap;
    unsigned           client_id;
    unsigned           state;

    SXE_UNUSED_PARAMETER(this);
    SXEE82I("(code=%03d,fmt=%s,...)", code, fmt);
    SXEA10I(code != 0, "code cannot be zero");
    SXEA10I(fmt != NULL, "format string cannot be NULL");

    if (code >= 400) {
        ++client->errors;
    }

    if (code == SXE_SMTP_CLOSING || code == SXE_SMTP_CLOSING_TMP) {
        client_id = sxe_smtpd_client_id(pool, client);
        state     = sxe_pool_index_to_state(pool, client_id);
        sxe_pool_set_indexed_element_state(pool, client_id, state, SXE_SMTPD_CLIENT_CLOSING);
    }
    else if (code == SXE_SMTP_STARTDATA) {
        client_id = sxe_smtpd_client_id(pool, client);
        state     = sxe_pool_index_to_state(pool, client_id);
        sxe_pool_set_indexed_element_state(pool, client_id, state, SXE_SMTPD_CLIENT_DATA);
    }

    len = snprintf(resp, rlen, "%d ", code);
    resp += len;
    ltot += len;
    rlen -= len;

    va_start(ap, fmt);
    len = vsnprintf(resp, rlen, fmt, ap);
    resp += len;
    ltot += len;
    rlen -= len;
    va_end(ap);

    ltot += snprintf(resp, rlen, "\r\n");

    result = sxe_send(this, client->response, ltot, sxe_smtpd_response_sent);

    if (result == SXE_RETURN_OK) {
        sxe_smtpd_response_sent(this, result);
    }

    SXER81I("return %s", sxe_return_to_string(result));
    return result;
}

SXE *
sxe_smtpd_listen(SXE_SMTPD * self, const char *address, unsigned short port)
{
    SXE * this;

    SXEE82("(address=%s, port=%hu)", address, port);

    if ((this = sxe_new_tcp(NULL, address, port, sxe_smtpd_event_connect, sxe_smtpd_event_read, sxe_smtpd_event_close)) == NULL) {
        SXEL20("sxe_smtpd_listen: Failed to allocate a SXE for TCP");           /* Coverage Exclusion: No need to test */
        goto SXE_ERROR_OUT;                                                     /* Coverage Exclusion: No need to test */
    }

    SXE_USER_DATA(this) = self;

    if (sxe_listen(this) != SXE_RETURN_OK) {
        SXEL22("sxe_smtpd_listen: Failed to listen on address %s, port %hu", address, port);    /* Coverage Exclusion: No need to test */
        sxe_close(this);                                                        /* Coverage Exclusion: No need to test */
        this = NULL;                                                            /* Coverage Exclusion: No need to test */
    }

SXE_ERROR_OUT:
    SXER81("return %p", this);
    return this;
}

/**
 * Construct an SMTPD server
 *
 * @param self         Pointer to an SMTPD server object
 * @param connections  Number of connections to support; must be >= 2
 * @param options      Currently unused;                 must be 0
 *
 * @exception Aborts if input preconditions are violated
 */
void
sxe_smtpd_construct(SXE_SMTPD * self, int connections, unsigned options)
{
    SXEE83("(self=%p, connections=%u, options=%x)", self, connections, options);
    SXEA10(connections  >=   2, "Requires at least 2 connections");
    SXEA10(options      ==   0, "Options must be 0");

    self->clients = sxe_pool_new("smtpd", connections, sizeof(SXE_SMTPD_CLIENT), SXE_SMTPD_CLIENT_NUMBER_OF_STATES, SXE_POOL_OPTION_UNLOCKED | SXE_POOL_OPTION_TIMED);
    sxe_pool_set_state_to_string(self->clients, sxe_smtpd_state_to_string);

    SXE_LIST_CONSTRUCT(&self->extensions, 0, SXE_SMTPD_EXTENSION, node);

    SXE_SMTPD_SET_HANDLER(self, connect,    sxe_smtpd_default_connect_handler);
    SXE_SMTPD_SET_HANDLER(self, close,      sxe_smtpd_default_close_handler);
    SXE_SMTPD_SET_HANDLER(self, reset,      sxe_smtpd_default_reset_handler);
    SXE_SMTPD_SET_HANDLER(self, helo,       sxe_smtpd_default_helo_handler);
    SXE_SMTPD_SET_HANDLER(self, from,       sxe_smtpd_default_from_handler);
    SXE_SMTPD_SET_HANDLER(self, rcpt,       sxe_smtpd_default_rcpt_handler);
    SXE_SMTPD_SET_HANDLER(self, data_start, sxe_smtpd_default_data_start_handler);
    SXE_SMTPD_SET_HANDLER(self, data_chunk, sxe_smtpd_default_data_chunk_handler);
    SXE_SMTPD_SET_HANDLER(self, data_end,   sxe_smtpd_default_data_end_handler);

    SXER80("return");
}

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
