/** @file
 * TCE-related internal stuff
 * 
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 *
 */

#ifndef __TE_TCE_INTERNAL_H__
#define __TE_TCE_INTERNAL_H__

typedef struct tce_counter
{
    unsigned num;
    long long *values;
} tce_counter;

extern int tce_read_value(rcf_rpc_server *rpcs, 
                          int progno, int objno, 
                          const char *functr, int functrno, int arcno, 
                          const char *attrname, const char *fmt, ...);

extern int tce_read_counters(rcf_rpc_server *rpcs, int progno, int objno, 
                             int ctrno, tce_counter *dest);

#define TCE_GLOBAL(_progno) _progno, -1, NULL, -1, -1
#define TCE_OBJ(_progno, _objno) _progno, _objno, NULL, -1, -1
#define TCE_FUN(_progno, _objno, _funno) _progno,  _objno, "fun", _funno, -1
#define TCE_CTR(_progno, _objno, _ctrno) _progno, _objno, "ctr", _ctrno, -1
#define TCE_ARC(_progno, _objno, _funno, _arcno) _progno, _objno, "fun", _funno, _arcno
#define TCE_VAL(_progno, _objno, _ctrno, _valno) _progno, _objno, "ctr", _ctrno, _valno

extern te_errno tce_save_data_gcc33(rcf_rpc_server *rpcs, int progno);
extern te_errno tce_save_data_gcc34(rcf_rpc_server *rpcs, int progno,
                                    unsigned version);

#endif /* __TE_TCE_INTERNAL_H__ */
