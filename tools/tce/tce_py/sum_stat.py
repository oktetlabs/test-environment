#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.

from html.parser import HTMLParser

class sum_stat(object):
    """
    TCE summary statistics:
        fn - number of files
        ln - number of lines
        le - number of lines executed
        bn - number of branches
        be - number of branches executed
    """

    def __init__(self):
        self.fn = 0
        self.ln = 0
        self.le = 0
        self.bn = 0
        self.be = 0

class _reader(HTMLParser):
    """
    Format of TCE summary:
        TAB :: <table ...> ... REC </table>
        REC :: <tr> FIL FRE LIN BRA REC | ''
        FIL :: <td> FRE </td>
        FRE :: <a ...> ... </a>
        LIN :: <td> CNT </td>
        BRA :: <td> CNT </td>
        CNT :: REAL '%' ' of ' INT
    """

    class _sustate(object):
        def __init__(self, idx, name):
            self._idx = idx
            self._name = name

        def __eq__(self, st):
            return self._idx == st._idx

        def __ne__(self, st):
            return not self.__eq__(st)

        def __str__(self):
            return self._name

        @property
        def nam(self):
            return self._name[:3]

    _S_REC_T = _sustate(1, "REC_T")
    _S_FIL_T = _sustate(2, "FIL_T")
    _S_FIL_E = _sustate(3, "FIL_E")
    _S_FRE_T = _sustate(4, "FRE_T")
    _S_FRE_D = _sustate(5, "FRE_D")
    _S_FRE_E = _sustate(6, "FRE_E")
    _S_LIN_T = _sustate(7, "LIN_T")
    _S_LIN_D = _sustate(8, "LIN_D")
    _S_LIN_E = _sustate(9, "LIN_E")
    _S_BRA_T = _sustate(10, "BRA_T")
    _S_BRA_D = _sustate(11, "BRA_D")
    _S_BRA_E = _sustate(12, "BRA_E")

    _S_TAG = _S_REC_T, _S_FIL_T, _S_FRE_T, _S_LIN_T, _S_BRA_T
    _S_ETA = _S_FIL_E, _S_FRE_E, _S_LIN_E, _S_BRA_E
    _S_DAT = _S_FRE_D, _S_LIN_D, _S_BRA_D

    def __init__(self):
        super(_reader, self).__init__()

        self._tab = False
        self._st = None
        self._stat = sum_stat()

    def handle_starttag(self, tag, attrs):
        if not self._tab:
            if tag == "table":
                self._tab = True
            return

        if not self._st:
            if tag == "tr":
                self._st = self._S_FIL_T
            return

        if self._st == self._S_REC_T:
            if tag != "tr":
                raise ValueError("REC expected")
            self._st = self._S_FIL_T

        elif self._st == self._S_FIL_T:
            if tag != "td":
                raise ValueError("FIL expected")
            self._st = self._S_FRE_T

        elif self._st == self._S_FRE_T:
            if tag != "a":
                raise ValueError("FRE expected")
            self._st = self._S_FRE_D

        elif self._st == self._S_LIN_T:
            if tag != "td":
                raise ValueError("LIN expected")
            self._st = self._S_LIN_D

        elif self._st == self._S_BRA_T:
            if tag != "td":
                raise ValueError("BRA expected")
            self._st = self._S_BRA_D

        else:
            if self._st in self._S_ETA:
                raise ValueError("end of %s expected" % self._st.nam)
            else:
                raise ValueError("data for %s expected" % self._st.nam)

    def handle_endtag(self, tag):
        if not self._tab:
            return

        if not self._st:
            return

        if self._st == self._S_REC_T:
            if tag != "table":
                raise ValueError("end of TAB expected")
            self._tab = False
            self._st = None

        elif self._st == self._S_FIL_E:
            if tag != "td":
                raise ValueError("end of FIL expected")
            self._st = self._S_LIN_T

        elif self._st == self._S_FRE_E:
            if tag != "a":
                raise ValueError("end of FRE expected")
            self._st = self._S_FIL_E

        elif self._st == self._S_LIN_E:
            if tag != "td":
                raise ValueError("end of LIN expected")
            self._st = self._S_BRA_T

        elif self._st == self._S_BRA_E:
            if tag != "td":
                raise ValueError("end of BRA expected")
            self._st = self._S_REC_T

        else:
            if self._st in self._S_TAG:
                raise ValueError("%s expected" % self._st.nam)
            else:
                raise ValueError("data for %s unexpected" % self._st.nam)

    def handle_data(self, data):
        if not self._tab:
            return

        if not self._st:
            return

        if self._st == self._S_FRE_D:
            self._st = self._S_FRE_E
            self._add_fn(data)

        elif self._st == self._S_LIN_D:
            self._st = self._S_LIN_E
            self._add_ln(data)

        elif self._st == self._S_BRA_D:
            self._st = self._S_BRA_E
            self._add_bn(data)

        else:
            if self._st in self._S_TAG:
                raise ValueError("%s expected" % self._st.nam)
            else:
                raise ValueError("end of %s expected" % self._st.nam)

    def _add_fn(self, data):
        self._stat.fn += 1

    def _add_ln(self, data):
        ce, cn = self._parse_cnt(data)
        self._stat.le += ce
        self._stat.ln += cn

    def _add_bn(self, data):
        ce, cn = self._parse_cnt(data)
        self._stat.be += ce
        self._stat.bn += cn

    def _parse_cnt(self, data):
        try:
            s = data.split(" ")
            if len(s) != 3 or s[1] != "of":
                raise ValueError()

            c1 = s[0]
            c2 = s[2]
            if len(c1) < 1 or c1[-1] != "%":
                raise ValueError()

            cn = int(c2)
            cx = float(c1[:-1]) / 100
            cx = int(round(cx * cn))

        except Exception as e:
            raise ValueError("invalid %s CNT" % self._st.nam)

        return cx, cn

    @classmethod
    def read(cls, text):
        reader = cls()
        reader.feed(text)
        s = reader._stat
        reader.close()
        return s

def read(fname):
    try:
        with open(fname, "r") as fp:
            text = fp.read()
    except Exception as e:
        raise ValueError("fail to read")

    try:
        stat = _reader.read(text)
    except Exception as e:
        raise ValueError("format error: %s" % e)

    return stat
