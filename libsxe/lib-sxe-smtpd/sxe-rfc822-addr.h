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

#ifndef __SXE_RFC822_ADDRESS_H__
#define __SXE_RFC822_ADDRESS_H__

#define SXE_RFC822_ADDRESS_LOCAL_LENGTH_LIMIT        64
#define SXE_RFC822_ADDRESS_DOMAIN_LENGTH_LIMIT      255

typedef struct SXE_RFC822_ADDRESS {
    int nullpath; /* <> */
    int angled;   /* did this come quoted? */
    int source_routes_dropped;
    char local[SXE_RFC822_ADDRESS_LOCAL_LENGTH_LIMIT + 1];
    char domain[SXE_RFC822_ADDRESS_DOMAIN_LENGTH_LIMIT + 1];
} SXE_RFC822_ADDRESS;

#include "sxe-rfc822-addr-proto.h"

#endif
