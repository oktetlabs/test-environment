/* Generated automatically by the program `genpeep'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "regs.h"
#include "output.h"
#include "real.h"

extern rtx peep_operand[];

#define operands peep_operand

rtx
peephole (ins1)
     rtx ins1;
{
  rtx insn, x, pat;
  int i;

  if (NEXT_INSN (ins1)
      && GET_CODE (NEXT_INSN (ins1)) == BARRIER)
    return 0;

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L207;
  x = XEXP (pat, 0);
  operands[0] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MULT) goto L207;
  x = XEXP (XEXP (pat, 1), 0);
  operands[1] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  x = XEXP (XEXP (pat, 1), 1);
  operands[2] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L207; }
  while (GET_CODE (insn) == NOTE
	 || (GET_CODE (insn) == INSN
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (GET_CODE (insn) == CODE_LABEL
      || GET_CODE (insn) == BARRIER)
    goto L207;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L207;
  x = XEXP (pat, 0);
  operands[3] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != PLUS) goto L207;
  x = XEXP (XEXP (pat, 1), 0);
  operands[4] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  x = XEXP (XEXP (pat, 1), 1);
  operands[5] = x;
  if (! register_operand (x, VOIDmode)) goto L207;
  if (! (TARGET_SNAKE && fmpyaddoperands (operands))) goto L207;
  PATTERN (ins1) = gen_rtx (PARALLEL, VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 207;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L207:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L208;
  x = XEXP (pat, 0);
  operands[3] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != PLUS) goto L208;
  x = XEXP (XEXP (pat, 1), 0);
  operands[4] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  x = XEXP (XEXP (pat, 1), 1);
  operands[5] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L208; }
  while (GET_CODE (insn) == NOTE
	 || (GET_CODE (insn) == INSN
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (GET_CODE (insn) == CODE_LABEL
      || GET_CODE (insn) == BARRIER)
    goto L208;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L208;
  x = XEXP (pat, 0);
  operands[0] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MULT) goto L208;
  x = XEXP (XEXP (pat, 1), 0);
  operands[1] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  x = XEXP (XEXP (pat, 1), 1);
  operands[2] = x;
  if (! register_operand (x, VOIDmode)) goto L208;
  if (! (TARGET_SNAKE && fmpyaddoperands (operands))) goto L208;
  PATTERN (ins1) = gen_rtx (PARALLEL, VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 208;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L208:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L209;
  x = XEXP (pat, 0);
  operands[0] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MULT) goto L209;
  x = XEXP (XEXP (pat, 1), 0);
  operands[1] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  x = XEXP (XEXP (pat, 1), 1);
  operands[2] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L209; }
  while (GET_CODE (insn) == NOTE
	 || (GET_CODE (insn) == INSN
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (GET_CODE (insn) == CODE_LABEL
      || GET_CODE (insn) == BARRIER)
    goto L209;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L209;
  x = XEXP (pat, 0);
  operands[3] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MINUS) goto L209;
  x = XEXP (XEXP (pat, 1), 0);
  operands[4] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  x = XEXP (XEXP (pat, 1), 1);
  operands[5] = x;
  if (! register_operand (x, VOIDmode)) goto L209;
  if (! (TARGET_SNAKE && fmpysuboperands (operands))) goto L209;
  PATTERN (ins1) = gen_rtx (PARALLEL, VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 209;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L209:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L210;
  x = XEXP (pat, 0);
  operands[3] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MINUS) goto L210;
  x = XEXP (XEXP (pat, 1), 0);
  operands[4] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  x = XEXP (XEXP (pat, 1), 1);
  operands[5] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L210; }
  while (GET_CODE (insn) == NOTE
	 || (GET_CODE (insn) == INSN
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (GET_CODE (insn) == CODE_LABEL
      || GET_CODE (insn) == BARRIER)
    goto L210;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L210;
  x = XEXP (pat, 0);
  operands[0] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != MULT) goto L210;
  x = XEXP (XEXP (pat, 1), 0);
  operands[1] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  x = XEXP (XEXP (pat, 1), 1);
  operands[2] = x;
  if (! register_operand (x, VOIDmode)) goto L210;
  if (! (TARGET_SNAKE && fmpysuboperands (operands))) goto L210;
  PATTERN (ins1) = gen_rtx (PARALLEL, VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 210;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L210:

  return 0;
}

rtx peep_operand[6];
