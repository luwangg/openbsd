/*
 * Copyright (c) 2001 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of KTH nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "krb5_locl.h"

RCSID("$KTH: derived-key-test.c,v 1.1 2001/03/12 07:44:52 assar Exp $");

enum { MAXSIZE = 24 };

static struct testcase {
    krb5_enctype enctype;
    unsigned char constant[MAXSIZE];
    size_t constant_len;
    unsigned char key[MAXSIZE];
    unsigned char res[MAXSIZE];
} tests[] = {
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0x55}, 5,
    {0xdc, 0xe0, 0x6b, 0x1f, 0x64, 0xc8, 0x57, 0xa1, 0x1c, 0x3d, 0xb5, 0x7c, 0x51, 0x89, 0x9b, 0x2c, 0xc1, 0x79, 0x10, 0x08, 0xce, 0x97, 0x3b, 0x92},
    {0x92, 0x51, 0x79, 0xd0, 0x45, 0x91, 0xa7, 0x9b, 0x5d, 0x31, 0x92, 0xc4, 0xa7, 0xe9, 0xc2, 0x89, 0xb0, 0x49, 0xc7, 0x1f, 0x6e, 0xe6, 0x04, 0xcd}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0xaa}, 5,
     {0x5e, 0x13, 0xd3, 0x1c, 0x70, 0xef, 0x76, 0x57, 0x46, 0x57, 0x85, 0x31, 0xcb, 0x51, 0xc1, 0x5b, 0xf1, 0x1c, 0xa8, 0x2c, 0x97, 0xce, 0xe9, 0xf2},
     {0x9e, 0x58, 0xe5, 0xa1, 0x46, 0xd9, 0x94, 0x2a, 0x10, 0x1c, 0x46, 0x98, 0x45, 0xd6, 0x7a, 0x20, 0xe3, 0xc4, 0x25, 0x9e, 0xd9, 0x13, 0xf2, 0x07}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0x55}, 5,
     {0x98, 0xe6, 0xfd, 0x8a, 0x04, 0xa4, 0xb6, 0x85, 0x9b, 0x75, 0xa1, 0x76, 0x54, 0x0b, 0x97, 0x52, 0xba, 0xd3, 0xec, 0xd6, 0x10, 0xa2, 0x52, 0xbc},
     {0x13, 0xfe, 0xf8, 0x0d, 0x76, 0x3e, 0x94, 0xec, 0x6d, 0x13, 0xfd, 0x2c, 0xa1, 0xd0, 0x85, 0x07, 0x02, 0x49, 0xda, 0xd3, 0x98, 0x08, 0xea, 0xbf}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0xaa}, 5,
     {0x62, 0x2a, 0xec, 0x25, 0xa2, 0xfe, 0x2c, 0xad, 0x70, 0x94, 0x68, 0x0b, 0x7c, 0x64, 0x94, 0x02, 0x80, 0x08, 0x4c, 0x1a, 0x7c, 0xec, 0x92, 0xb5},
     {0xf8, 0xdf, 0xbf, 0x04, 0xb0, 0x97, 0xe6, 0xd9, 0xdc, 0x07, 0x02, 0x68, 0x6b, 0xcb, 0x34, 0x89, 0xd9, 0x1f, 0xd9, 0xa4, 0x51, 0x6b, 0x70, 0x3e}},
    {ETYPE_DES3_CBC_SHA1, {0x6b, 0x65, 0x72, 0x62, 0x65, 0x72, 0x6f, 0x73}, 8,
     {0xd3, 0xf8, 0x29, 0x8c, 0xcb, 0x16, 0x64, 0x38, 0xdc, 0xb9, 0xb9, 0x3e, 0xe5, 0xa7, 0x62, 0x92, 0x86, 0xa4, 0x91, 0xf8, 0x38, 0xf8, 0x02, 0xfb},
     {0x23, 0x70, 0xda, 0x57, 0x5d, 0x2a, 0x3d, 0xa8, 0x64, 0xce, 0xbf, 0xdc, 0x52, 0x04, 0xd5, 0x6d, 0xf7, 0x79, 0xa7, 0xdf, 0x43, 0xd9, 0xda, 0x43}},
    {ETYPE_DES3_CBC_SHA1, {0x63, 0x6f, 0x6d, 0x62, 0x69, 0x6e, 0x65}, 7,
     {0xb5, 0x5e, 0x98, 0x34, 0x67, 0xe5, 0x51, 0xb3, 0xe5, 0xd0, 0xe5, 0xb6, 0xc8, 0x0d, 0x45, 0x76, 0x94, 0x23, 0xa8, 0x73, 0xdc, 0x62, 0xb3, 0x0e},
     {0x01, 0x26, 0x38, 0x8a, 0xad, 0xc8, 0x1a, 0x1f, 0x2a, 0x62, 0xbc, 0x45, 0xf8, 0xd5, 0xc1, 0x91, 0x51, 0xba, 0xcd, 0xd5, 0xcb, 0x79, 0x8a, 0x3e}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0x55}, 5,
     {0xc1, 0x08, 0x16, 0x49, 0xad, 0xa7, 0x43, 0x62, 0xe6, 0xa1, 0x45, 0x9d, 0x01, 0xdf, 0xd3, 0x0d, 0x67, 0xc2, 0x23, 0x4c, 0x94, 0x07, 0x04, 0xda},
     {0x34, 0x80, 0x57, 0xec, 0x98, 0xfd, 0xc4, 0x80, 0x16, 0x16, 0x1c, 0x2a, 0x4c, 0x7a, 0x94, 0x3e, 0x92, 0xae, 0x49, 0x2c, 0x98, 0x91, 0x75, 0xf7}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0xaa}, 5,
     {0x5d, 0x15, 0x4a, 0xf2, 0x38, 0xf4, 0x67, 0x13, 0x15, 0x57, 0x19, 0xd5, 0x5e, 0x2f, 0x1f, 0x79, 0x0d, 0xd6, 0x61, 0xf2, 0x79, 0xa7, 0x91, 0x7c},
     {0xa8, 0x80, 0x8a, 0xc2, 0x67, 0xda, 0xda, 0x3d, 0xcb, 0xe9, 0xa7, 0xc8, 0x46, 0x26, 0xfb, 0xc7, 0x61, 0xc2, 0x94, 0xb0, 0x13, 0x15, 0xe5, 0xc1}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0x55}, 5,
     {0x79, 0x85, 0x62, 0xe0, 0x49, 0x85, 0x2f, 0x57, 0xdc, 0x8c, 0x34, 0x3b, 0xa1, 0x7f, 0x2c, 0xa1, 0xd9, 0x73, 0x94, 0xef, 0xc8, 0xad, 0xc4, 0x43},
     {0xc8, 0x13, 0xf8, 0x8a, 0x3b, 0xe3, 0xb3, 0x34, 0xf7, 0x54, 0x25, 0xce, 0x91, 0x75, 0xfb, 0xe3, 0xc8, 0x49, 0x3b, 0x89, 0xc8, 0x70, 0x3b, 0x49}},
    {ETYPE_DES3_CBC_SHA1, {0x00, 0x00, 0x00, 0x01, 0xaa}, 5,
     {0x26, 0xdc, 0xe3, 0x34, 0xb5, 0x45, 0x29, 0x2f, 0x2f, 0xea, 0xb9, 0xa8, 0x70, 0x1a, 0x89, 0xa4, 0xb9, 0x9e, 0xb9, 0x94, 0x2c, 0xec, 0xd0, 0x16},
     {0xf4, 0x8f, 0xfd, 0x6e, 0x83, 0xf8, 0x3e, 0x73, 0x54, 0xe6, 0x94, 0xfd, 0x25, 0x2c, 0xf8, 0x3b, 0xfe, 0x58, 0xf7, 0xd5, 0xba, 0x37, 0xec, 0x5d}},
    {0}
};

int
main(int argc, char **argv)
{
    struct testcase *t;
    krb5_context context;
    krb5_error_code ret;
    int val = 0;

    ret = krb5_init_context (&context);
    if (ret)
	errx (1, "krb5_init_context failed: %d", ret);

    for (t = tests; t->enctype != 0; ++t) {
	krb5_keyblock key;
	krb5_keyblock *dkey;

	key.keytype = KEYTYPE_DES3;
	key.keyvalue.length = MAXSIZE;
	key.keyvalue.data   = t->key;

	ret = krb5_derive_key(context, &key, t->enctype, t->constant,
			      t->constant_len, &dkey);
	if (ret)
	    krb5_err (context, 1, ret, "krb5_derive_key");
	if (memcmp (dkey->keyvalue.data, t->res, dkey->keyvalue.length) != 0) {
	    const unsigned char *p = dkey->keyvalue.data;
	    int i;

	    printf ("derive_key failed\n");
	    printf ("should be: ");
	    for (i = 0; i < dkey->keyvalue.length; ++i)
		printf ("%02x", t->res[i]);
	    printf ("\nresult was: ");
	    for (i = 0; i < dkey->keyvalue.length; ++i)
		printf ("%02x", p[i]);
	    printf ("\n");
	    val = 1;
	}
    }
    return val;
}
