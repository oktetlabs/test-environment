/** @file
 * TCE-related internal stuff
 *
 * Copyright (C) 2003-2018 OKTET Labs.
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
