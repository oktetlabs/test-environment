..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: page; ASN.1 library
.. _doxid-asn:

ASN.1 library
=============

ITU-T Recommendation X.680 "Information technology - Abstract Syntax Notation One (ASN.1):
Specification of basic notation"

ITU-T Recommendation X.690 "Information technology - ASN.1 encoding rules:
Specification of BER, CER and DER"

This library provides full-functional API for work with ASN.1 values. Representation of data, storing ASN.1 value, is opaque for library users. One of the main features of this library is that "ASN.1 value" object instance contains all information, necessary for BER encoding, whereas ASN.1 textual representation of value is insufficient for it encoding requires information about ASN.1 type definition.

According with ASN.1 specification, ASN value may be considered as tree hierarchy. Compound ASN values are consist of some another values, which may be either compound or simple. Some types of compound value contain only one subvalue, but nevetheless they are considered as containers.

Some of compound types contains named subvalues, they are: 'SEQUENCE', 'CHOICE', 'SET'. Therefore, to access subvalue in instance of such type, one have to specify its symbolic label, which is set in definition of ASN.1 type this value belongs to.

User API: ``asn_usr.h``

Internal API: ``asn_impl.h``

