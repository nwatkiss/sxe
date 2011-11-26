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

#include "tap.h"
#include "sxe-smtpd.h"
#include "sxe-test.h"
#include "sxe-util.h"

#define INVALID(s)   is(sxe_rfc822_parse_address(&addr, s, strlen(s)),         0, #s " - invalid RFC822 address")
#define VALID(s)     is(sxe_rfc822_parse_address(&addr, s, strlen(s)), strlen(s), #s " - valid RFC822 address")
#define ANGLED(s,i)  VALID(s); is(addr.angled, i, #s " - address is %s<angled>", i ? "" : "not ")
#define LOCAL(s, l)  VALID(s); is_eq(addr.local, l, #s " - local part is " #l)
#define DOMAIN(s, d) VALID(s); is_eq(addr.domain, d, #s " - domain part is " #d)

int
main(void)
{
    SXE_RFC822_ADDRESS addr;

    tap_plan(114, TAP_FLAG_ON_FAILURE_EXIT, NULL);

    INVALID("");
    INVALID("foo");
    INVALID("foo@");
    INVALID("@bar");
    INVALID("foo bar");

    VALID("foo@bar");
    VALID("neil.watkiss@bar");
    VALID("neil.watkiss@ba\\r_baz.com");
    VALID(" < neil.watkiss@foo-bar.baz>");
    VALID("<bill.o'reilly@sophos.com>");
    VALID("\"Billy Graham\"@domain.com");
    VALID("\"Billy\\\" Graham\"@domain.com");

    INVALID("\"Billy\\\" Graham@domain.com");
    INVALID("Billy Graham@domain.com");
    INVALID("<Billy Graham@domain.com>");
    INVALID("<Billy Graham@domain.com>");

    VALID("Billy\\ Graham@domain.com");
    VALID("Billy\\ Graham   @  domain.com");

    INVALID("neil(that's watkiss, really).watkiss@bar");
    INVALID("(comment)neil.watkiss@bar");
    INVALID("neil.watkiss(really)@bar");
    INVALID("foo@[127.");
    INVALID("foo@abc,def[127.");
    INVALID("foo@[");
    INVALID("foo@[[]");
    INVALID("foo@[neil]");
    INVALID("<foo@[neil]>");

    VALID("<foo@[127.0.0.1]>");
    INVALID("<foo@[127.0.0.1]");
    VALID("foo@[127.\\0.0.1]");
    VALID("   foo@[127.0.0.1]");
    VALID("   foo@[127.0.0.1]   ");
    VALID("<@a,@c:foo@bar.com>");
    INVALID("<@a,@c;foo@bar.com>");

    ANGLED("foo@bar",                           0);
    ANGLED("<foo@bar>",                         1);
    ANGLED("neil.watkiss@bar",                  0);
    ANGLED("neil.watkiss@ba\\r_baz.com",        0);
    ANGLED(" < neil.watkiss@foo-bar.baz>",      1);
    ANGLED("<bill.o'reilly@sophos.com>",        1);
    ANGLED("<@a,@c:foo@bar.com>",               1);

    LOCAL("foo@bar",                            "foo");
    LOCAL("<foo@bar>",                          "foo");
    LOCAL("neil.watkiss@bar",                   "neil.watkiss");
    LOCAL("neil.watkiss @ bar",                 "neil.watkiss");
    LOCAL("<neil.watkiss@bar>",                 "neil.watkiss");
    LOCAL(" < neil.watkiss@foo-bar.baz>",       "neil.watkiss");
    LOCAL("<bill.o'reilly@sophos.com>",         "bill.o'reilly");
    LOCAL("\"Billy Graham\"@domain.com",        "Billy Graham");
    LOCAL("\"Billy\\\" Graham\"@domain.com",    "Billy\" Graham");
    LOCAL("Billy\\ Graham@domain.com",          "Billy Graham");
    LOCAL("Billy\\ Graham   @  domain.com",     "Billy Graham");
    LOCAL("<foo@[127.0.0.1]>",                  "foo");
    LOCAL("<foo@[127.0.0.1]>",                  "foo");
    LOCAL("foo@[127.\\0.0.1]",                  "foo");
    LOCAL("   foo@[127.0.0.1]",                 "foo");
    LOCAL("   foo@[127.0.0.1]   ",              "foo");
    LOCAL("<@a,@c:foo@bar.com>",                "foo");

    DOMAIN("foo@bar",                           "bar");
    DOMAIN("<foo@bar>",                         "bar");
    DOMAIN("neil.watkiss@ba\\r_baz.com",        "bar_baz.com");
    DOMAIN(" < neil.watkiss@foo-bar.baz>",      "foo-bar.baz");
    DOMAIN("<bill.o'reilly@sophos.com>",        "sophos.com");
    DOMAIN("\"Billy Graham\"@domain.com",       "domain.com");
    DOMAIN("\"Billy\\\" Graham\"@domain.com",   "domain.com");
    DOMAIN("\"Billy\\\" Graham\"@domain.com",   "domain.com");
    DOMAIN("Billy\\ Graham@domain.com",         "domain.com");
    DOMAIN("Billy\\ Graham   @  domain.com",    "domain.com");
    DOMAIN("<foo@[127.0.0.1]>",                 "[127.0.0.1]");
    DOMAIN("<foo@[127.0.0.1]>",                 "[127.0.0.1]");
    DOMAIN("foo@[127.\\0.0.1]",                 "[127.0.0.1]");
    DOMAIN("   foo@[127.0.0.1]",                "[127.0.0.1]");
    DOMAIN("   foo@[127.0.0.1]   ",             "[127.0.0.1]");
    DOMAIN("<@a,@c:foo@bar.com>",               "bar.com");

    return exit_status();
}

/* vim: set ft=c sw=4 sts=4 ts=8 listchars=tab\:^.,trail\:@ expandtab list: */
