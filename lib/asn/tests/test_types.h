/** @file
 * @brief ASN.1 library
 *
 * Definitions of ASN types used for tests.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE__ASN_LIB__TEST_TYPES__H__
#define __TE__ASN_LIB__TEST_TYPES__H__

#include "te_errno.h"
#include "asn_impl.h"

/* 
PlainSeq1 ::= [APPLICATION 1] SEQUENCE {
    number INTEGER,
    string UniversalString
}
*/

extern asn_type asn_base_integer_s;
extern asn_type asn_base_charstring_s;

asn_named_entry_t _plain_seq1_ne_array [] = {
    { "number", &asn_base_integer_s },
    { "string", &asn_base_charstring_s }
};

asn_type at_plain_seq1 = {
    "PlainSeq1",
    {APPLICATION, 1},
    SEQUENCE,
    2,
    {_plain_seq1_ne_array}
};


/* 
PlainChoice1 ::= [APPLICATION 2] CHOICE {
    number INTEGER,
    string UniversalString
}
*/ 
asn_type at_plain_choice1 = {
    "PlainChoice1",
    {APPLICATION, 2},
    CHOICE,
    2,
    {_plain_seq1_ne_array}
};


/*
PlainIntArray ::= [APPLICATION 3] SEQUENCE OF INTEGER
*/
asn_type at_plain_int_array = {
    "PlainIntArray",
    {APPLICATION, 3},
    SEQUENCE_OF,
    0,
    {&asn_base_integer_s}
};


/*
NamedIntArray ::= SEQUENCE {
    name UniversalString,
    array PlainIntArray
}
*/

asn_named_entry_t _at_named_int_array_ne [] = {
    { "name", &asn_base_charstring_s },
    { "array", &at_plain_int_array }
};



asn_type at_named_int_array = {
    "NamedArray",
    {CONTEXT_SPECIFIC, 1},
    SEQUENCE,
    sizeof(_at_named_int_array_ne)/sizeof(asn_named_entry_t),
    {_at_named_int_array_ne}
};


/*
OurNames ::= INTEGER  {
    galba (9),
    thor (16)
}
*/

asn_enum_entry_t _at_our_names_enum_entries[] = 
{
    {"galba", 9},
    {"thor", 16}
};

asn_type at_our_names = {
    "OurNames",
    {UNIVERSAL, 10},
    ENUMERATED,
    sizeof(_at_our_names_enum_entries)/sizeof(asn_enum_entry_t), 
    {_at_our_names_enum_entries}
};

#endif /* __TE__ASN_LIB__TEST_TYPES__H__ */
