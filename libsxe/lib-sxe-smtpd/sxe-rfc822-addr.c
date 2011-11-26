#include <string.h>

#include "sxe.h"
#include "sxe-rfc822-addr.h"

static const char atext[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789!#$%&'*+-/=?^_`{|}~"
    ;

static const char dtext[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-"
    "_"        /* in direct violation of RFC2821 */
    ;

static void
ws(size_t *iptr, const char *address, size_t addrlen)
{
    while (*iptr < addrlen && NULL != strchr("\t ", address[*iptr])) ++*iptr;
}

static void
quoted_string(size_t *iptr, char *out, char *maxout, const char *address, size_t addrlen)
{
    size_t icopy;

    ws(iptr, address, addrlen);
    if (address[*iptr] != '"')
        return;

    icopy = *iptr;
    ++*iptr;
    while (*iptr < addrlen && out < maxout) {
        if (address[*iptr] == '\\' && (*iptr + 1) < addrlen) {
            ++*iptr;
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else if (NULL == strchr("\"\\", address[*iptr])) {
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else
            break;
    }
    if (address[*iptr] != '"') {
        *iptr = icopy;
        return; /* FAILED */
    }
    ++*iptr;

    ws(iptr, address, addrlen);
}

static void
dot_atom(size_t *iptr, char *out, char *maxout, const char *address, size_t addrlen)
{
    ws(iptr, address, addrlen);

    while (*iptr < addrlen && out < maxout) {
        if (address[*iptr] == '\\' && (*iptr + 1) < addrlen) {
            ++*iptr;
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else if (strchr(atext, address[*iptr]) || address[*iptr] == '.') {
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else
            break;
    }

    ws(iptr, address, addrlen);
}

static int
local_part(size_t *iptr, SXE_RFC822_ADDRESS *dest, const char *address, size_t addrlen)
{
    size_t icopy;

    SXEA10(*iptr == 0, "Internal error");

    /* '<' */
    ws(iptr, address, addrlen);
    if (*iptr < addrlen && address[*iptr] == '<') {
        dest->angled = 1;
        ++*iptr;
    }

    /* source routes? */
    ws(iptr, address, addrlen);
    if (*iptr < addrlen && address[*iptr] == '@') {
        while (*iptr < addrlen && address[*iptr] != ':') ++*iptr;
        if (*iptr < addrlen) ++*iptr;
    }

    icopy = *iptr;
    dot_atom(iptr, dest->local, dest->local + SXE_RFC822_ADDRESS_LOCAL_LENGTH_LIMIT, address, addrlen);
    if (*iptr > icopy && *iptr < addrlen && address[*iptr] == '@') {
        ++*iptr;
        return 0;
    }

    *iptr = icopy;
    quoted_string(iptr, dest->local, dest->local + SXE_RFC822_ADDRESS_LOCAL_LENGTH_LIMIT, address, addrlen);
    if (*iptr > icopy && *iptr < addrlen && address[*iptr] == '@') {
        ++*iptr;
        return 0;
    }

    return -1;
}

static void
domain(size_t *iptr, char *out, char *maxout, const char *address, size_t addrlen)
{
    ws(iptr, address, addrlen);

    while (*iptr < addrlen && out < maxout) {
        if (address[*iptr] == '\\' && (*iptr + 1) < addrlen) {
            ++*iptr;
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else if (strchr(dtext, address[*iptr]) || address[*iptr] == '.') {
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else
            break;
    }

    ws(iptr, address, addrlen);
}

static void
domain_literal(size_t *iptr, char *out, char *maxout, const char *address, size_t addrlen)
{
    size_t icopy;
    ws(iptr, address, addrlen);
    if (address[*iptr] != '[')
        return;

    icopy = *iptr;
    *out++ = address[*iptr];
    *out = '\0';
    ++*iptr;
    while (*iptr < addrlen && out < maxout) {
        if (address[*iptr] == '\\' && (*iptr + 1) < addrlen) {
            ++*iptr;
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else if (NULL != strchr("0123456789.", address[*iptr])) {
            *out++ = address[*iptr];
            *out = '\0';
            ++*iptr;
        }
        else
            break;
    }
    if (address[*iptr] != ']') {
        *iptr = icopy;
        return; /* FAILED */
    }
    *out++ = address[*iptr];
    *out = '\0';
    ++*iptr;

    ws(iptr, address, addrlen);
}

static int
domain_part(size_t *iptr, SXE_RFC822_ADDRESS *dest, const char *address, size_t addrlen)
{
    size_t icopy;

    if (*iptr == addrlen)
        return -1;

    icopy = *iptr;
    domain(iptr, dest->domain, dest->domain + SXE_RFC822_ADDRESS_DOMAIN_LENGTH_LIMIT, address, addrlen);
    if (dest->angled) {
        if (*iptr > icopy && *iptr < addrlen && address[*iptr] == '>') {
            ++*iptr;
            return 0;
        }
    }
    else if (*iptr == addrlen)
        return 0;

    *iptr = icopy;
    domain_literal(iptr, dest->domain, dest->domain + SXE_RFC822_ADDRESS_DOMAIN_LENGTH_LIMIT, address, addrlen);
    if (dest->angled) {
        if (*iptr > icopy && *iptr < addrlen && address[*iptr] == '>') {
            ++*iptr;
            return 0;
        }
    }
    else if (*iptr == addrlen)
        return 0;

    return -1;
}

size_t
sxe_rfc822_parse_address(SXE_RFC822_ADDRESS *dest, const char *address, size_t addrlen)
{
    size_t retval = 0;
    size_t i      = 0;

    SXEE93("(address='%.*s',len=%d)", (int)addrlen, address, (int)addrlen);

    dest->angled = 0;
    dest->local[0] = '\0';
    dest->domain[0] = '\0';

    /* local-part "@" */
    if (-1 == local_part(&i, dest, address, addrlen))
        goto SXE_EARLY_OUT;

    /* domain */
    if (-1 == domain_part(&i, dest, address, addrlen))
        goto SXE_EARLY_OUT;

    retval = i;

SXE_EARLY_OUT:
    SXER91("return retval=%d", (int)retval);
    return retval;
}

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
