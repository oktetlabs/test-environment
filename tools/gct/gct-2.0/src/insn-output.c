/* Generated automatically by the program `genoutput'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"

#include "conditions.h"
#include "insn-flags.h"
#include "insn-attr.h"

#include "insn-codes.h"

#include "recog.h"

#include <stdio.h>
#include "output.h"

static char *
output_20 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_20[] = {
    "com%I3clr,%B4 %3,%2,0\n\taddi 1,%0,%0",
    "com%I3clr,%B4 %3,%2,0\n\taddi,tr 1,%1,%0\n\tcopy %1,%0",
  };
  return strings_20[which_alternative];
}

static char *
output_24 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_24[] = {
    "com%I3clr,%B4 %3,%2,0\n\taddi -1,%0,%0",
    "com%I3clr,%B4 %3,%2,0\n\taddi,tr -1,%1,%0\n\tcopy %1,%0",
  };
  return strings_24[which_alternative];
}

static char *
output_25 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_25[] = {
    "comclr,> %2,%0,0\n\tcopy %2,%0",
    "comiclr,> %2,%0,0\n\tldi %2,%0",
    "comclr,> %1,%2,%0\n\tcopy %1,%0",
  };
  return strings_25[which_alternative];
}

static char *
output_26 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_26[] = {
    "comclr,>> %2,%0,0\n\tcopy %2,%0",
    "comiclr,>> %2,%0,0\n\tldi %2,%0",
  };
  return strings_26[which_alternative];
}

static char *
output_27 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_27[] = {
    "comclr,< %2,%0,0\n\tcopy %2,%0",
    "comiclr,< %2,%0,0\n\tldi %2,%0",
    "comclr,< %1,%2,%0\n\tcopy %1,%0",
  };
  return strings_27[which_alternative];
}

static char *
output_28 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_28[] = {
    "comclr,<< %2,%0,0\n\tcopy %2,%0",
    "comiclr,<< %2,%0,0\n\tldi %2,%0",
  };
  return strings_28[which_alternative];
}

static char *
output_29 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_29[] = {
    "com%I4clr,%S5 %4,%3,0\n\tldi 0,%0",
    "com%I4clr,%B5 %4,%3,%0\n\tcopy %1,%0",
    "com%I4clr,%B5 %4,%3,%0\n\tldi %1,%0",
    "com%I4clr,%B5 %4,%3,%0\n\tldil L'%1,%0",
    "com%I4clr,%B5 %4,%3,%0\n\tzdepi %Z1,%0",
  };
  return strings_29[which_alternative];
}

static char *
output_30 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_30[] = {
    "com%I4clr,%S5 %4,%3,0\n\tcopy %2,%0",
    "com%I4clr,%S5 %4,%3,0\n\tldi %2,%0",
    "com%I4clr,%S5 %4,%3,0\n\tldil L'%2,%0",
    "com%I4clr,%S5 %4,%3,0\n\tzdepi %Z2,%0",
    "com%I4clr,%B5 %4,%3,0\n\tcopy %1,%0",
    "com%I4clr,%B5 %4,%3,0\n\tldi %1,%0",
    "com%I4clr,%B5 %4,%3,0\n\tldil L'%1,%0",
    "com%I4clr,%B5 %4,%3,0\n\tzdepi %Z1,%0",
  };
  return strings_30[which_alternative];
}

static char *
output_41 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return output_cbranch (operands, INSN_ANNULLED_BRANCH_P (insn), 
			 get_attr_length (insn), 0, insn);
}
}

static char *
output_42 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return output_cbranch (operands, INSN_ANNULLED_BRANCH_P (insn), 
			 get_attr_length (insn), 1, insn);
}
}

static char *
output_43 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return output_bb (operands, INSN_ANNULLED_BRANCH_P (insn), 
			 get_attr_length (insn), 
			 (operands[3] != pc_rtx),
			 insn, 0);
}
}

static char *
output_44 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return output_bb (operands, INSN_ANNULLED_BRANCH_P (insn), 
			 get_attr_length (insn), 
			 (operands[3] != pc_rtx),
			 insn, 1);
}
}

static char *
output_45 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INSN_ANNULLED_BRANCH_P (insn))
    return "ftest\n\tbl,n %0,0";
  else
    return "ftest\n\tbl%* %0,0";
}
}

static char *
output_46 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INSN_ANNULLED_BRANCH_P (insn))
    return "ftest\n\tadd,tr 0,0,0\n\tbl,n %0,0";
  else
    return "ftest\n\tadd,tr 0,0,0\n\tbl%* %0,0";
}
}

static char *
output_51 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_51[] = {
    "copy %r1,%0",
    "ldi %1,%0",
    "ldil L'%1,%0",
    "zdepi %Z1,%0",
    "ldw%M1 %1,%0",
    "stw%M0 %r1,%0",
    "mtsar %r1",
    "fcpy,sgl %r1,%0",
    "fldws%F1 %1,%0",
    "fstws%F0 %1,%0",
  };
  return strings_51[which_alternative];
}

static char *
output_53 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INTVAL (operands[2]) < 0)
    return "ldwm %2(0,%0),%3";
  return "ldws,mb %2(0,%0),%3";
}
}

static char *
output_54 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INTVAL (operands[2]) < 0)
    return "stwm %r3,%2(0,%0)";
  return "stws,mb %r3,%2(0,%0)";
}
}

static char *
output_55 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INTVAL (operands[2]) > 0)
    return "ldwm %2(0,%0),%3";
  return "ldws,ma %2(0,%0),%3";
}
}

static char *
output_56 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (INTVAL (operands[2]) > 0)
    return "stwm %r3,%2(0,%0)";
  return "stws,ma %r3,%2(0,%0)";
}
}

static char *
output_57 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  rtx label_rtx = gen_label_rtx ();
  rtx xoperands[3];
  extern FILE *asm_out_file;

  xoperands[0] = operands[0];
  xoperands[1] = operands[1];
  xoperands[2] = label_rtx;
  output_asm_insn ("bl .+8,%0\n\taddil L'%1-%2,%0", xoperands);
  ASM_OUTPUT_INTERNAL_LABEL (asm_out_file, "L", CODE_LABEL_NUMBER (label_rtx));
  output_asm_insn ("ldo R'%1-%2(1),%0", xoperands);
  return "";
  }

}

static char *
output_58 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_58[] = {
    "addil L'%G1,%%r27",
  };
  return strings_58[which_alternative];
}

static char *
output_59 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_59[] = {
    "addil L'%G1,%%r27",
    "ldil L'%G1,%0\n\tadd %0,%%r27,%0",
  };
  return strings_59[which_alternative];
}

static char *
output_60 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_60[] = {
    "addil L'%G2,%1",
    "ldil L'%G2,%0\n\tadd %0,%1,%0",
  };
  return strings_60[which_alternative];
}

static char *
output_67 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_67[] = {
    "copy %r1,%0",
    "ldi %1,%0",
    "ldil L'%1,%0",
    "zdepi %Z1,%0",
    "ldh%M1 %1,%0",
    "sth%M0 %r1,%0",
    "mtsar %r1",
    "fcpy,sgl %r1,%0",
  };
  return strings_67[which_alternative];
}

static char *
output_74 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_74[] = {
    "copy %r1,%0",
    "ldi %1,%0",
    "ldil L'%1,%0",
    "zdepi %Z1,%0",
    "ldb%M1 %1,%0",
    "stb%M0 %r1,%0",
    "mtsar %r1",
    "fcpy,sgl %r1,%0",
  };
  return strings_74[which_alternative];
}

static char *
output_78 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_block_move (operands, !which_alternative);
}

static char *
output_79 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return (which_alternative == 0 ? output_move_double (operands)
				    : " fldds%F1 %1,%0");
}

static char *
output_81 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]) || FP_REG_P (operands[1]) 
      || operands[1] == CONST0_RTX (DFmode))
    return output_fp_move_double (operands);
  return output_move_double (operands);
}
}

static char *
output_87 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  rtx op0 = operands[0];
  rtx op1 = operands[1];

  if (GET_CODE (op1) == CONST_INT)
    {
      operands[0] = operand_subword (op0, 1, 0, DImode);
      output_asm_insn ("ldil L'%1,%0", operands);

      operands[0] = operand_subword (op0, 0, 0, DImode);
      if (INTVAL (op1) < 0)
	output_asm_insn ("ldi -1,%0", operands);
      else
	output_asm_insn ("ldi 0,%0", operands);
      return "";
    }
  else if (GET_CODE (op1) == CONST_DOUBLE)
    {
      operands[0] = operand_subword (op0, 1, 0, DImode);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, CONST_DOUBLE_LOW (op1));
      output_asm_insn ("ldil L'%1,%0", operands);

      operands[0] = operand_subword (op0, 0, 0, DImode);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, CONST_DOUBLE_HIGH (op1));
      output_asm_insn (singlemove_string (operands), operands);
      return "";
    }
  else
    abort ();
}
}

static char *
output_88 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]) || FP_REG_P (operands[1])
      || (operands[1] == CONST0_RTX (DImode)))
    return output_fp_move_double (operands);
  return output_move_double (operands);
}
}

static char *
output_89 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  /* Don't output a 64 bit constant, since we can't trust the assembler to
     handle it correctly.  */
  if (GET_CODE (operands[2]) == CONST_DOUBLE)
    operands[2] = gen_rtx (CONST_INT, VOIDmode, CONST_DOUBLE_LOW (operands[2]));
  if (which_alternative == 1)
    output_asm_insn ("copy %1,%0", operands);
  return "ldo R'%G2(%R1),%R0";
}
}

static char *
output_90 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return (which_alternative == 0 ? singlemove_string (operands)
				    : " fldws%F1 %1,%0");
}

static char *
output_92 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_92[] = {
    "fcpy,sgl %r1,%0",
    "copy %r1,%0",
    "fldws%F1 %1,%0",
    "ldw%M1 %1,%0",
    "fstws%F0 %r1,%0",
    "stw%M0 %r1,%0",
  };
  return strings_92[which_alternative];
}

static char *
output_95 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_95[] = {
    "extru %1,31,16,%0",
    "ldh%M1 %1,%0",
  };
  return strings_95[which_alternative];
}

static char *
output_96 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_96[] = {
    "extru %1,31,8,%0",
    "ldb%M1 %1,%0",
  };
  return strings_96[which_alternative];
}

static char *
output_97 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_97[] = {
    "extru %1,31,8,%0",
    "ldb%M1 %1,%0",
  };
  return strings_97[which_alternative];
}

static char *
output_115 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) >= 0)
	return "addi %2,%R1,%R0\n\taddc %1,0,%0";
      else
	return "addi %2,%R1,%R0\n\tsubb %1,0,%0";
    }
  else
    return "add %R2,%R1,%R0\n\taddc %2,%1,%0";
}
}

static char *
output_119 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_119[] = {
    "add %1,%2,%0",
    "ldo %2(%1),%0",
  };
  return strings_119[which_alternative];
}

static char *
output_121 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_121[] = {
    "sub %1,%2,%0",
    "subi %1,%2,%0",
  };
  return strings_121[which_alternative];
}

static char *
output_124 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_mul_insn (0, insn);
}

static char *
output_126 (operands, insn)
     rtx *operands;
     rtx insn;
{

 return output_div_insn (operands, 0, insn);
}

static char *
output_128 (operands, insn)
     rtx *operands;
     rtx insn;
{

 return output_div_insn (operands, 1, insn);
}

static char *
output_130 (operands, insn)
     rtx *operands;
     rtx insn;
{

  return output_mod_insn (0, insn);
}

static char *
output_132 (operands, insn)
     rtx *operands;
     rtx insn;
{

  return output_mod_insn (1, insn);
}

static char *
output_135 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_and (operands); 
}

static char *
output_141 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_ior (operands); 
}

static char *
output_171 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_171[] = {
    "zvdep %1,32,%0",
    "zvdepi %1,32,%0",
  };
  return strings_171[which_alternative];
}

static char *
output_172 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  int x = INTVAL (operands[1]);
  operands[2] = GEN_INT (4 + exact_log2 ((x >> 4) + 1));
  operands[1] = GEN_INT ((x & 0xf) - 0x10);
  return "zvdepi %1,%2,%0";
}
}

static char *
output_176 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_176[] = {
    "vshd 0,%1,%0",
    "extru %1,%P2,%L2,%0",
  };
  return strings_176[which_alternative];
}

static char *
output_177 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      operands[2] = GEN_INT (INTVAL (operands[2]) & 31);
      return "shd %1,%1,%2,%0";
    }
  else
    return "vshd %1,%1,%0";
}
}

static char *
output_178 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  operands[2] = GEN_INT ((32 - INTVAL (operands[2])) & 31);
  return "shd %1,%1,%2,%0";
}
}

static char *
output_181 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  int cnt = INTVAL (operands[2]) & 31;
  operands[3] = GEN_INT (exact_log2 (1 + (INTVAL (operands[3]) >> cnt)));
  operands[2] = GEN_INT (31 - cnt);
  return "zdep %1,%2,%3,%0";
}
}

static char *
output_190 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      operands[1] = GEN_INT (~INTVAL (operands[1]));
      return "addi,uv %1,%0,0\n\tblr,n %0,0\n\tb,n %l3";
    }
  else
    {
      return "sub,>> %0,%1,0\n\tblr,n %0,0\n\tb,n %l3";
    }
}
}

static char *
output_192 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  output_arg_descriptor (insn);
  return output_call (insn, operands[0], gen_rtx (REG, SImode, 2));
}
}

static char *
output_195 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  output_arg_descriptor (insn);
  return output_call (insn, operands[1], gen_rtx (REG, SImode, 2));
}
}

static char *
output_201 (operands, insn)
     rtx *operands;
     rtx insn;
{
  static /*const*/ char *const strings_201[] = {
    "dep %3,%2+%1-1,%1,%0",
    "depi %3,%2+%1-1,%1,%0",
  };
  return strings_201[which_alternative];
}

static char *
output_202 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  operands[3] = GEN_INT ((INTVAL (operands[3]) & 0xf) - 0x10);
  return "depi %3,%2+%1-1,%1,%0";
}
}

static char *
output_203 (operands, insn)
     rtx *operands;
     rtx insn;
{
 output_dbra (operands, insn, which_alternative); 
}

static char *
output_204 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_dbra (operands, insn, which_alternative);
}

static char *
output_205 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_movb (operands, insn, which_alternative, 0); 
}

static char *
output_206 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_movb (operands, insn, which_alternative, 1); 
}

static char *
output_207 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_MODE (operands[0]) == DFmode)
    {
      if (rtx_equal_p (operands[5], operands[3]))
	return "fmpyadd,dbl %1,%2,%0,%4,%3";
      else
	return "fmpyadd,dbl %1,%2,%0,%5,%3";
    }
  else
    {
      if (rtx_equal_p (operands[5], operands[3]))
	return "fmpyadd,sgl %1,%2,%0,%4,%3";
      else
	return "fmpyadd,sgl %1,%2,%0,%5,%3";
    }
}
}

static char *
output_208 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_MODE (operands[0]) == DFmode)
    {
      if (rtx_equal_p (operands[3], operands[5]))
	return "fmpyadd,dbl %1,%2,%0,%4,%3";
      else
	return "fmpyadd,dbl %1,%2,%0,%5,%3";
    }
  else
    {
      if (rtx_equal_p (operands[3], operands[5]))
	return "fmpyadd,sgl %1,%2,%0,%4,%3";
      else
	return "fmpyadd,sgl %1,%2,%0,%5,%3";
    }
}
}

static char *
output_209 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_MODE (operands[0]) == DFmode)
    return "fmpysub,dbl %1,%2,%0,%5,%3";
  else
    return "fmpysub,sgl %1,%2,%0,%5,%3";
}
}

static char *
output_210 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_MODE (operands[0]) == DFmode)
    return "fmpysub,dbl %1,%2,%0,%5,%3";
  else
    return "fmpysub,sgl %1,%2,%0,%5,%3";
}
}

char * const insn_template[] =
  {
    0,
    0,
    0,
    "fcmp,sgl,%Y2 %r0,%r1",
    "fcmp,dbl,%Y2 %r0,%r1",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "com%I2clr,%B3 %2,%1,%0\n\tldi 1,%0",
    "com%I2clr,%B3 %2,%1,%0\n\tldi -1,%0",
    "sub%I3 %3,%2,0\n\taddc 0,%1,%0",
    "sub %2,%3,0\n\taddc 0,%1,%0",
    "addi %k3,%2,0\n\taddc 0,%1,%0",
    0,
    "sub%I3 %3,%2,0\n\tsubb %1,0,%0",
    "sub %2,%3,0\n\tsubb %1,0,%0",
    "addi %k3,%2,0\n\tsubb %1,0,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "ldw T'%2(%1),%0",
    0,
    "ldwx,s %1(0,%2),%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "ldil LP'%G1,%0",
    "ldil L'%G1,%0",
    "ldo RP'%G2(%1),%0\n\textru,= %0,31,1,%3\n\tldw -4(0,%%r27),%3\n\tadd %0,%3,%0",
    "ldo R'%G2(%1),%0",
    0,
    0,
    0,
    "ldhx,s %2(0,%1),%0",
    "ldhs,mb %2(0,%0),%3",
    "sths,mb %r3,%2(0,%0)",
    "ldil L'%G1,%0",
    "ldo R'%G2(%1),%0",
    0,
    0,
    "ldbs,mb %2(0,%0),%3",
    "stbs,mb %r3,%2(0,%0)",
    0,
    0,
    0,
    0,
    0,
    "flddx,s %1(0,%2),%0",
    "fstdx,s %0,%1(0,%2)",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "fldwx,s %1(0,%2),%0",
    "fstwx,s %0,%1(0,%2)",
    0,
    0,
    0,
    "extrs %1,31,16,%0",
    "extrs %1,31,8,%0",
    "extrs %1,31,8,%0",
    "fcnvff,sgl,dbl %1,%0",
    "fcnvff,dbl,sgl %1,%0",
    "fldws %1,%0\n\tfcnvxf,sgl,sgl %0,%0",
    "fcnvxf,sgl,sgl %1,%0",
    "fldws %1,%0\n\tfcnvxf,sgl,dbl %0,%0",
    "fcnvxf,sgl,dbl %1,%0",
    0,
    0,
    "fcnvxf,dbl,sgl %1,%0",
    "fcnvxf,dbl,dbl %1,%0",
    "fcnvfxt,sgl,sgl %1,%0",
    "fcnvfxt,dbl,sgl %1,%0",
    "fcnvfxt,sgl,dbl %1,%0",
    "fcnvfxt,dbl,dbl %1,%0",
    0,
    "uaddcm %2,%1,%0",
    0,
    0,
    0,
    "sub %R1,%R2,%R0\n\tsubb %1,%2,%0",
    0,
    0,
    "xmpyu %1,%2,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "and %1,%2,%0\n\tand %R1,%R2,%R0",
    0,
    "andcm %2,%1,%0\n\tandcm %R2,%R1,%R0",
    "andcm %2,%1,%0",
    0,
    "or %1,%2,%0\n\tor %R1,%R2,%R0",
    0,
    0,
    "or %1,%2,%0",
    0,
    "xor %1,%2,%0\n\txor %R1,%R2,%R0",
    "xor %1,%2,%0",
    "sub 0,%R1,%R0\n\tsubb 0,%1,%0",
    "sub 0,%1,%0",
    0,
    "uaddcm 0,%1,%0\n\tuaddcm 0,%R1,%R0",
    "uaddcm 0,%1,%0",
    "fadd,dbl %1,%2,%0",
    "fadd,sgl %1,%2,%0",
    "fsub,dbl %1,%2,%0",
    "fsub,sgl %1,%2,%0",
    "fmpy,dbl %1,%2,%0",
    "fmpy,sgl %1,%2,%0",
    "fdiv,dbl %1,%2,%0",
    "fdiv,sgl %1,%2,%0",
    "fsub,dbl 0,%1,%0",
    "fsub,sgl 0,%1,%0",
    "fabs,dbl %1,%0",
    "fabs,sgl %1,%0",
    "fsqrt,dbl %1,%0",
    "fsqrt,sgl %1,%0",
    "ldb%M1 %1,%0",
    "ldh%M1 %1,%0",
    "sh%O3add %2,%1,%0",
    "sh%O4add %2,%1,%0\n\tadd%I3 %3,%0,%0",
    0,
    "zdep %1,%P2,%L2,%0",
    0,
    0,
    0,
    "extrs %1,%P2,%L2,%0",
    "vextrs %1,32,%0",
    0,
    0,
    0,
    "shd %1,%2,%4,%0",
    "shd %1,%2,%4,%0",
    0,
    "bv%* 0(%%r2)",
    "bv%* 0(%%r2)",
    0,
    0,
    "bl _mcount,%%r2\n\tldo %0(%%r2),%%r25",
    "",
    "bl%* %l0,0",
    0,
    0,
    0,
    0,
    "copy %r0,%%r22\n\t.CALL\tARGW0=GR\n\tbl $$dyncall,%%r31\n\tcopy %%r31,%%r2",
    0,
    0,
    "copy %r1,%%r22\n\t.CALL\tARGW0=GR\n\tbl $$dyncall,%%r31\n\tcopy %%r31,%%r2",
    "nop",
    "bv%* 0(%0)",
    "extru %1,%3+%2-1,%2,%0",
    "extrs %1,%3+%2-1,%2,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "fdc 0(0,%0)\n\tsync\n\tfic 0(0,%0)\n\tsync\n\tfdc 0(0,%1)\n\tsync\n\tfic 0(0,%1)\n\tsync\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop",
  };

char *(*const insn_outfun[])() =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_20,
    0,
    0,
    0,
    output_24,
    output_25,
    output_26,
    output_27,
    output_28,
    output_29,
    output_30,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_41,
    output_42,
    output_43,
    output_44,
    output_45,
    output_46,
    0,
    0,
    0,
    0,
    output_51,
    0,
    output_53,
    output_54,
    output_55,
    output_56,
    output_57,
    output_58,
    output_59,
    output_60,
    0,
    0,
    0,
    0,
    0,
    0,
    output_67,
    0,
    0,
    0,
    0,
    0,
    0,
    output_74,
    0,
    0,
    0,
    output_78,
    output_79,
    0,
    output_81,
    0,
    0,
    0,
    0,
    0,
    output_87,
    output_88,
    output_89,
    output_90,
    0,
    output_92,
    0,
    0,
    output_95,
    output_96,
    output_97,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_115,
    0,
    0,
    0,
    output_119,
    0,
    output_121,
    0,
    0,
    output_124,
    0,
    output_126,
    0,
    output_128,
    0,
    output_130,
    0,
    output_132,
    0,
    0,
    output_135,
    0,
    0,
    0,
    0,
    0,
    output_141,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_171,
    output_172,
    0,
    0,
    0,
    output_176,
    output_177,
    output_178,
    0,
    0,
    output_181,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_190,
    0,
    output_192,
    0,
    0,
    output_195,
    0,
    0,
    0,
    0,
    0,
    output_201,
    output_202,
    output_203,
    output_204,
    output_205,
    output_206,
    output_207,
    output_208,
    output_209,
    output_210,
    0,
  };

rtx (*const insn_gen_function[]) () =
  {
    gen_cmpsi,
    gen_cmpsf,
    gen_cmpdf,
    0,
    0,
    gen_seq,
    gen_sne,
    gen_slt,
    gen_sgt,
    gen_sle,
    gen_sge,
    gen_sltu,
    gen_sgtu,
    gen_sleu,
    gen_sgeu,
    gen_scc,
    gen_negscc,
    0,
    0,
    0,
    gen_incscc,
    0,
    0,
    0,
    gen_decscc,
    gen_sminsi3,
    gen_uminsi3,
    gen_smaxsi3,
    gen_umaxsi3,
    0,
    0,
    gen_beq,
    gen_bne,
    gen_bgt,
    gen_blt,
    gen_bge,
    gen_ble,
    gen_bgtu,
    gen_bltu,
    gen_bgeu,
    gen_bleu,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_movsi,
    gen_reload_insi,
    gen_reload_outsi,
    0,
    0,
    0,
    gen_pre_ldwm,
    gen_pre_stwm,
    gen_post_ldwm,
    gen_post_stwm,
    0,
    0,
    0,
    gen_add_high_const,
    0,
    0,
    0,
    0,
    0,
    gen_movhi,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_movqi,
    0,
    0,
    0,
    gen_movstrsi,
    0,
    0,
    gen_movdf,
    0,
    0,
    0,
    gen_movdi,
    gen_reload_indi,
    gen_reload_outdi,
    0,
    0,
    0,
    0,
    gen_movsf,
    0,
    0,
    0,
    gen_zero_extendhisi2,
    gen_zero_extendqihi2,
    gen_zero_extendqisi2,
    gen_extendhisi2,
    gen_extendqihi2,
    gen_extendqisi2,
    gen_extendsfdf2,
    gen_truncdfsf2,
    0,
    gen_floatsisf2,
    0,
    gen_floatsidf2,
    gen_floatunssisf2,
    gen_floatunssidf2,
    gen_floatdisf2,
    gen_floatdidf2,
    gen_fix_truncsfsi2,
    gen_fix_truncdfsi2,
    gen_fix_truncsfdi2,
    gen_fix_truncdfdi2,
    gen_adddi3,
    0,
    0,
    0,
    gen_addsi3,
    gen_subdi3,
    gen_subsi3,
    gen_mulsi3,
    gen_umulsidi3,
    0,
    gen_divsi3,
    0,
    gen_udivsi3,
    0,
    gen_modsi3,
    0,
    gen_umodsi3,
    0,
    gen_anddi3,
    0,
    gen_andsi3,
    0,
    0,
    gen_iordi3,
    0,
    gen_iorsi3,
    0,
    0,
    gen_xordi3,
    0,
    gen_xorsi3,
    gen_negdi2,
    gen_negsi2,
    gen_one_cmpldi2,
    0,
    gen_one_cmplsi2,
    gen_adddf3,
    gen_addsf3,
    gen_subdf3,
    gen_subsf3,
    gen_muldf3,
    gen_mulsf3,
    gen_divdf3,
    gen_divsf3,
    gen_negdf2,
    gen_negsf2,
    gen_absdf2,
    gen_abssf2,
    gen_sqrtdf2,
    gen_sqrtsf2,
    0,
    0,
    0,
    0,
    gen_ashlsi3,
    0,
    gen_zvdep32,
    gen_zvdep_imm,
    gen_ashrsi3,
    0,
    gen_vextrs32,
    gen_lshrsi3,
    gen_rotrsi3,
    gen_rotlsi3,
    0,
    0,
    0,
    gen_return,
    gen_return_internal,
    gen_prologue,
    gen_epilogue,
    gen_call_profiler,
    gen_blockage,
    gen_jump,
    gen_casesi,
    gen_casesi0,
    gen_call,
    gen_call_internal_symref,
    gen_call_internal_reg,
    gen_call_value,
    gen_call_value_internal_symref,
    gen_call_value_internal_reg,
    gen_nop,
    gen_indirect_jump,
    gen_extzv,
    gen_extv,
    gen_insv,
    0,
    gen_decrement_and_branch_until_zero,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_cacheflush,
  };

char *insn_name[] =
  {
    "cmpsi",
    "cmpsf",
    "cmpdf",
    "cmpdf+1",
    "seq-1",
    "seq",
    "sne",
    "slt",
    "sgt",
    "sle",
    "sge",
    "sltu",
    "sgtu",
    "sleu",
    "sgeu",
    "scc",
    "negscc",
    "negscc+1",
    "negscc+2",
    "incscc-1",
    "incscc",
    "incscc+1",
    "incscc+2",
    "decscc-1",
    "decscc",
    "sminsi3",
    "uminsi3",
    "smaxsi3",
    "umaxsi3",
    "umaxsi3+1",
    "beq-1",
    "beq",
    "bne",
    "bgt",
    "blt",
    "bge",
    "ble",
    "bgtu",
    "bltu",
    "bgeu",
    "bleu",
    "bleu+1",
    "bleu+2",
    "bleu+3",
    "movsi-3",
    "movsi-2",
    "movsi-1",
    "movsi",
    "reload_insi",
    "reload_outsi",
    "reload_outsi+1",
    "reload_outsi+2",
    "pre_ldwm-1",
    "pre_ldwm",
    "pre_stwm",
    "post_ldwm",
    "post_stwm",
    "post_stwm+1",
    "post_stwm+2",
    "add_high_const-1",
    "add_high_const",
    "add_high_const+1",
    "add_high_const+2",
    "add_high_const+3",
    "movhi-2",
    "movhi-1",
    "movhi",
    "movhi+1",
    "movhi+2",
    "movhi+3",
    "movqi-3",
    "movqi-2",
    "movqi-1",
    "movqi",
    "movqi+1",
    "movqi+2",
    "movstrsi-1",
    "movstrsi",
    "movstrsi+1",
    "movdf-1",
    "movdf",
    "movdf+1",
    "movdf+2",
    "movdi-1",
    "movdi",
    "reload_indi",
    "reload_outdi",
    "reload_outdi+1",
    "reload_outdi+2",
    "movsf-2",
    "movsf-1",
    "movsf",
    "movsf+1",
    "movsf+2",
    "zero_extendhisi2-1",
    "zero_extendhisi2",
    "zero_extendqihi2",
    "zero_extendqisi2",
    "extendhisi2",
    "extendqihi2",
    "extendqisi2",
    "extendsfdf2",
    "truncdfsf2",
    "truncdfsf2+1",
    "floatsisf2",
    "floatsisf2+1",
    "floatsidf2",
    "floatunssisf2",
    "floatunssidf2",
    "floatdisf2",
    "floatdidf2",
    "fix_truncsfsi2",
    "fix_truncdfsi2",
    "fix_truncsfdi2",
    "fix_truncdfdi2",
    "adddi3",
    "adddi3+1",
    "adddi3+2",
    "addsi3-1",
    "addsi3",
    "subdi3",
    "subsi3",
    "mulsi3",
    "umulsidi3",
    "umulsidi3+1",
    "divsi3",
    "divsi3+1",
    "udivsi3",
    "udivsi3+1",
    "modsi3",
    "modsi3+1",
    "umodsi3",
    "umodsi3+1",
    "anddi3",
    "anddi3+1",
    "andsi3",
    "andsi3+1",
    "iordi3-1",
    "iordi3",
    "iordi3+1",
    "iorsi3",
    "iorsi3+1",
    "xordi3-1",
    "xordi3",
    "xordi3+1",
    "xorsi3",
    "negdi2",
    "negsi2",
    "one_cmpldi2",
    "one_cmpldi2+1",
    "one_cmplsi2",
    "adddf3",
    "addsf3",
    "subdf3",
    "subsf3",
    "muldf3",
    "mulsf3",
    "divdf3",
    "divsf3",
    "negdf2",
    "negsf2",
    "absdf2",
    "abssf2",
    "sqrtdf2",
    "sqrtsf2",
    "sqrtsf2+1",
    "sqrtsf2+2",
    "ashlsi3-2",
    "ashlsi3-1",
    "ashlsi3",
    "ashlsi3+1",
    "zvdep32",
    "zvdep_imm",
    "ashrsi3",
    "ashrsi3+1",
    "vextrs32",
    "lshrsi3",
    "rotrsi3",
    "rotlsi3",
    "rotlsi3+1",
    "rotlsi3+2",
    "return-1",
    "return",
    "return_internal",
    "prologue",
    "epilogue",
    "call_profiler",
    "blockage",
    "jump",
    "casesi",
    "casesi0",
    "call",
    "call_internal_symref",
    "call_internal_reg",
    "call_value",
    "call_value_internal_symref",
    "call_value_internal_reg",
    "nop",
    "indirect_jump",
    "extzv",
    "extv",
    "insv",
    "insv+1",
    "decrement_and_branch_until_zero",
    "decrement_and_branch_until_zero+1",
    "decrement_and_branch_until_zero+2",
    "decrement_and_branch_until_zero+3",
    "decrement_and_branch_until_zero+4",
    "cacheflush-3",
    "cacheflush-2",
    "cacheflush-1",
    "cacheflush",
  };
char **insn_name_ptr = insn_name;

const int insn_n_operands[] =
  {
    2,
    2,
    2,
    3,
    3,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    4,
    4,
    4,
    4,
    4,
    5,
    4,
    4,
    4,
    5,
    3,
    3,
    3,
    3,
    6,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    4,
    4,
    4,
    4,
    0,
    0,
    2,
    3,
    3,
    3,
    2,
    3,
    4,
    4,
    4,
    4,
    3,
    2,
    2,
    3,
    2,
    2,
    4,
    3,
    3,
    2,
    2,
    3,
    4,
    4,
    2,
    3,
    2,
    2,
    4,
    4,
    4,
    6,
    2,
    2,
    2,
    3,
    3,
    2,
    3,
    3,
    2,
    2,
    3,
    2,
    2,
    2,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    5,
    5,
    3,
    3,
    3,
    4,
    3,
    1,
    4,
    2,
    4,
    2,
    4,
    1,
    4,
    1,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    4,
    5,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    6,
    6,
    4,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    5,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    0,
    1,
    4,
    4,
    4,
    4,
    5,
    6,
    3,
    3,
    6,
    6,
    6,
    6,
    2,
  };

const int insn_n_dups[] =
  {
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    0,
    0,
    0,
    0,
    2,
    2,
    4,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    3,
    3,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    3,
    2,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
  };

char *const insn_operand_constraint[][MAX_RECOG_OPERANDS] =
  {
    { "", "", },
    { "", "", },
    { "", "", },
    { "fxG", "fxG", "", },
    { "fxG", "fxG", "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "", },
    { "=r", "r", "rI", "", },
    { "=r", "r", "rI", "", },
    { "=r", "r", "r", "rI", },
    { "=r", "r", "r", "r", },
    { "=r", "r", "r", "I", },
    { "=r,r", "0,?r", "r,r", "rI,rI", "", },
    { "=r", "r", "r", "rI", },
    { "=r", "r", "r", "r", },
    { "=r", "r", "r", "I", },
    { "=r,r", "0,?r", "r,r", "rI,rI", "", },
    { "=r,r,r", "%0,0,r", "r,I,M", },
    { "=r,r", "%0,0", "r,I", },
    { "=r,r,r", "%0,0,r", "r,I,M", },
    { "=r,r", "%0,0", "r,I", },
    { "=r,r,r,r,r", "0,r,J,N,K", "", "r,r,r,r,r", "rI,rI,rI,rI,rI", "", },
    { "=r,r,r,r,r,r,r,r", "0,0,0,0,r,J,N,K", "r,J,N,K,0,0,0,0", "r,r,r,r,r,r,r,r", "rI,rI,rI,rI,rI,rI,rI,rI", "", },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { "", "r", "rL", "", },
    { "", "r", "rL", "", },
    { "r", "", "", "", },
    { "r", "", "", "", },
    { 0 },
    { 0 },
    { "", "", },
    { "=Z", "", "=&r", },
    { "", "Z", "=&r", },
    { "=r", "r", "", },
    { "=r,r,r,r,r,Q,*q,!fx,fx,*T", "rM,J,N,K,Q,rM,rM,!fxM,*T,fx", },
    { "=r", "r", "r", },
    { "=r", "0", "", "=r", },
    { "=r", "0", "", "rM", },
    { "=r", "0", "", "r", },
    { "=r", "0", "", "rM", },
    { "=r", "i", "=a", },
    { "=a", "", },
    { "=!a,*r", "", },
    { "=!a,*r", "r,r", "", },
    { "=r", "", },
    { "=r", "", },
    { "=r", "r", "", "=r", },
    { "=r", "r", "i", },
    { "", "", "", },
    { "", "", },
    { "=r,r,r,r,r,Q,*q,!fx", "rM,J,N,K,Q,rM,rM,!fxM", },
    { "=r", "r", "r", },
    { "=r", "0", "L", "=r", },
    { "=r", "0", "L", "rM", },
    { "=r", "", },
    { "=r", "r", "i", },
    { "", "", },
    { "=r,r,r,r,r,Q,*q,!fx", "rM,J,N,K,Q,rM,rM,!fxM", },
    { "=r", "0", "L", "=r", },
    { "=r", "0", "L", "rM", },
    { "", "", "", "", },
    { "+r,r", "+r,r", "=r,r", "=&r,&r", "J,2", "n,n", },
    { "=?r,fx", "?E,m", },
    { "", "", },
    { "=fx,*r,Q,?o,?Q,fx,*&r,*&r", "fxG,*rG,fx,*r,*r,Q,o,Q", },
    { "=fx", "r", "r", },
    { "fx", "r", "r", },
    { "", "", },
    { "=z", "", "=&r", },
    { "", "z", "=&r", },
    { "=r", "", },
    { "=r,o,Q,&r,&r,&r,x,x,*T", "rM,r,r,o,Q,i,xM,*T,x", },
    { "=r,r", "0,r", "i,i", },
    { "=?r,fx", "?E,m", },
    { "", "", },
    { "=fx,r,fx,r,Q,Q", "fxG,rG,Q,Q,fx,rG", },
    { "=fx", "r", "r", },
    { "fx", "r", "r", },
    { "=r,r", "r,Q", },
    { "=r,r", "r,Q", },
    { "=r,r", "r,Q", },
    { "=r", "r", },
    { "=r", "r", },
    { "=r", "r", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "m", },
    { "=fx", "fx", },
    { "=fx", "m", },
    { "=fx", "fx", },
    { "", "", },
    { "", "", },
    { "=x", "x", },
    { "=x", "x", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=x", "x", },
    { "=x", "x", },
    { "=r", "%r", "rI", },
    { "=r", "r", "r", },
    { "", "", "", "", "", },
    { "", "", "", "", "", },
    { "=r,r", "%r,r", "r,J", },
    { "=r", "r", "r", },
    { "=r,r", "r,I", "r,r", },
    { "", "", "", "", },
    { "=x", "x", "x", },
    { "=a", },
    { "", "", "", "", },
    { "", "=a", },
    { "", "", "", "", },
    { "", "=a", },
    { "", "", "", "", },
    { "=a", },
    { "", "", "", "", },
    { "=a", },
    { "", "", "", },
    { "=r", "%r", "r", },
    { "=r,r", "%r,0", "rO,P", },
    { "=r", "r", "r", },
    { "=r", "r", "r", },
    { "", "", "", },
    { "=r", "%r", "r", },
    { "", "", "", },
    { "=r", "0", "", },
    { "=r", "%r", "r", },
    { "", "", "", },
    { "=r", "%r", "r", },
    { "=r", "%r", "r", },
    { "=r", "r", },
    { "=r", "r", },
    { "", "", },
    { "=r", "r", },
    { "=r", "r", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=fx", "fx", },
    { "=r", "m", },
    { "=r", "m", },
    { "=r", "r", "r", "", },
    { "=&r", "r", "r", "rI", "", },
    { "", "", "", },
    { "=r", "r", "n", },
    { "=r,r", "r,L", "q,q", },
    { "=r", "", "q", },
    { "", "", "", },
    { "=r", "r", "n", },
    { "=r", "r", "q", },
    { "=r,r", "r,r", "q,n", },
    { "=r,r", "r,r", "q,n", },
    { "=r", "r", "n", },
    { "=r", "r", "r", "n", "n", "", },
    { "=r", "r", "r", "n", "n", "", },
    { "=r", "r", "", "", },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { "", },
    { 0 },
    { 0 },
    { "", "", "", "", "", },
    { "r", "rI", },
    { "", "", },
    { "", "i", },
    { "r", "i", },
    { "", "", "", },
    { "=rfx", "", "i", },
    { "=rfx", "r", "i", },
    { 0 },
    { "r", },
    { "=r", "r", "", "", },
    { "=r", "r", "", "", },
    { "+r,r", "", "", "r,L", },
    { "+r", "", "", "", },
    { "+!r,!*f*x,!*m", "L,L,L", "", "", "=X,r,r", },
    { "+!r,!*f*x,!*m", "L,L,L", "", "", "=X,r,r", "", },
    { "=!r,!*f*x,!*m", "r,r,r", "", },
    { "=!r,!*f*x,!*m", "r,r,r", "", },
    { "=fx", "fx", "fx", "+fx", "fx", "fx", },
    { "=fx", "fx", "fx", "+fx", "fx", "fx", },
    { "=fx", "fx", "fx", "+fx", "fx", "fx", },
    { "=fx", "fx", "fx", "+fx", "fx", "fx", },
    { "r", "r", },
  };

const enum machine_mode insn_operand_mode[][MAX_RECOG_OPERANDS] =
  {
    { SImode, SImode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, CCFPmode, },
    { DFmode, DFmode, CCFPmode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, VOIDmode, SImode, SImode, VOIDmode, },
    { SImode, SImode, SImode, SImode, SImode, VOIDmode, },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode, SImode, SImode, VOIDmode, },
    { VOIDmode, SImode, SImode, VOIDmode, },
    { SImode, SImode, VOIDmode, VOIDmode, },
    { SImode, SImode, VOIDmode, VOIDmode, },
    { VOIDmode },
    { VOIDmode },
    { SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, VOIDmode, },
    { SImode, VOIDmode, },
    { SImode, SImode, VOIDmode, },
    { SImode, SImode, },
    { SImode, VOIDmode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { VOIDmode, VOIDmode, VOIDmode, },
    { HImode, HImode, },
    { HImode, HImode, },
    { HImode, SImode, SImode, },
    { SImode, SImode, SImode, HImode, },
    { SImode, SImode, SImode, HImode, },
    { HImode, VOIDmode, },
    { HImode, HImode, VOIDmode, },
    { QImode, QImode, },
    { QImode, QImode, },
    { SImode, SImode, SImode, QImode, },
    { SImode, SImode, SImode, QImode, },
    { BLKmode, BLKmode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, SImode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { DFmode, SImode, SImode, },
    { DFmode, SImode, SImode, },
    { DImode, DImode, },
    { DImode, DImode, SImode, },
    { DImode, DImode, SImode, },
    { DImode, VOIDmode, },
    { DImode, DImode, },
    { DImode, DImode, DImode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { SFmode, SImode, SImode, },
    { SFmode, SImode, SImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { DFmode, SFmode, },
    { SFmode, DFmode, },
    { SFmode, SImode, },
    { SFmode, SImode, },
    { DFmode, SImode, },
    { DFmode, SImode, },
    { SFmode, SImode, },
    { DFmode, SImode, },
    { SFmode, DImode, },
    { DFmode, DImode, },
    { SImode, SFmode, },
    { SImode, DFmode, },
    { DImode, SFmode, },
    { DImode, DFmode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { SImode, SImode, SImode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { DImode, SImode, SImode, },
    { SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, },
    { DImode, DImode, DImode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { DImode, DImode, DImode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { DImode, DImode, DImode, },
    { DImode, DImode, DImode, },
    { SImode, SImode, SImode, },
    { DImode, DImode, },
    { SImode, SImode, },
    { DImode, DImode, },
    { DImode, DImode, },
    { SImode, SImode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { SImode, },
    { VOIDmode },
    { VOIDmode },
    { SImode, SImode, SImode, VOIDmode, VOIDmode, },
    { SImode, SImode, },
    { SImode, VOIDmode, },
    { SImode, VOIDmode, },
    { SImode, VOIDmode, },
    { VOIDmode, SImode, VOIDmode, },
    { VOIDmode, SImode, VOIDmode, },
    { VOIDmode, SImode, VOIDmode, },
    { VOIDmode },
    { SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, VOIDmode, VOIDmode, SImode, },
    { SImode, SImode, VOIDmode, VOIDmode, SImode, SImode, },
    { SImode, SImode, VOIDmode, },
    { SImode, SImode, VOIDmode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, VOIDmode, },
    { SImode, SImode, },
  };

const char insn_operand_strict_low[][MAX_RECOG_OPERANDS] =
  {
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0 },
    { 0 },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, },
    { 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, },
    { 0, 0, 0, 0, },
    { 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, },
    { 0 },
    { 0 },
    { 0, 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0 },
    { 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, },
  };

extern int reg_or_0_operand ();
extern int arith5_operand ();
extern int comparison_operator ();
extern int register_operand ();
extern int arith11_operand ();
extern int int11_operand ();
extern int reg_or_cint_move_operand ();
extern int uint5_operand ();
extern int pc_or_label_operand ();
extern int general_operand ();
extern int symbolic_operand ();
extern int reg_or_nonsymb_mem_operand ();
extern int move_operand ();
extern int pre_cint_operand ();
extern int post_cint_operand ();
extern int pic_operand ();
extern int scratch_operand ();
extern int const_int_operand ();
extern int function_label_operand ();
extern int immediate_operand ();
extern int int5_operand ();
extern int arith_operand ();
extern int reg_or_0_or_nonsymb_mem_operand ();
extern int div_operand ();
extern int arith_double_operand ();
extern int and_operand ();
extern int arith32_operand ();
extern int ior_operand ();
extern int memory_operand ();
extern int shadd_operand ();
extern int lhs_lshift_operand ();
extern int lhs_lshift_cint_operand ();
extern int plus_xor_ior_operator ();
extern int call_operand_address ();
extern int eq_neq_comparison_operator ();
extern int movb_comparison_operator ();

int (*const insn_operand_predicate[][MAX_RECOG_OPERANDS])() =
  {
    { reg_or_0_operand, arith5_operand, },
    { reg_or_0_operand, reg_or_0_operand, },
    { reg_or_0_operand, reg_or_0_operand, },
    { reg_or_0_operand, reg_or_0_operand, comparison_operator, },
    { reg_or_0_operand, reg_or_0_operand, comparison_operator, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, },
    { register_operand, register_operand, arith11_operand, comparison_operator, },
    { register_operand, register_operand, arith11_operand, comparison_operator, },
    { register_operand, register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, int11_operand, },
    { register_operand, register_operand, register_operand, arith11_operand, comparison_operator, },
    { register_operand, register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, int11_operand, },
    { register_operand, register_operand, register_operand, arith11_operand, comparison_operator, },
    { register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, arith11_operand, },
    { register_operand, reg_or_cint_move_operand, 0, register_operand, arith11_operand, comparison_operator, },
    { register_operand, reg_or_cint_move_operand, reg_or_cint_move_operand, register_operand, arith11_operand, comparison_operator, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, register_operand, arith5_operand, comparison_operator, },
    { 0, register_operand, arith5_operand, comparison_operator, },
    { register_operand, uint5_operand, pc_or_label_operand, pc_or_label_operand, },
    { register_operand, uint5_operand, pc_or_label_operand, pc_or_label_operand, },
    { 0 },
    { 0 },
    { general_operand, general_operand, },
    { register_operand, general_operand, register_operand, },
    { general_operand, register_operand, register_operand, },
    { register_operand, register_operand, symbolic_operand, },
    { reg_or_nonsymb_mem_operand, move_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, pre_cint_operand, register_operand, },
    { register_operand, register_operand, pre_cint_operand, reg_or_0_operand, },
    { register_operand, register_operand, post_cint_operand, register_operand, },
    { register_operand, register_operand, post_cint_operand, reg_or_0_operand, },
    { register_operand, pic_operand, scratch_operand, },
    { register_operand, 0, },
    { register_operand, 0, },
    { register_operand, register_operand, const_int_operand, },
    { register_operand, function_label_operand, },
    { register_operand, 0, },
    { register_operand, register_operand, function_label_operand, register_operand, },
    { register_operand, register_operand, immediate_operand, },
    { 0, 0, 0, },
    { general_operand, general_operand, },
    { reg_or_nonsymb_mem_operand, move_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, int5_operand, register_operand, },
    { register_operand, register_operand, int5_operand, reg_or_0_operand, },
    { register_operand, 0, },
    { register_operand, register_operand, immediate_operand, },
    { general_operand, general_operand, },
    { reg_or_nonsymb_mem_operand, move_operand, },
    { register_operand, register_operand, int5_operand, register_operand, },
    { register_operand, register_operand, int5_operand, reg_or_0_operand, },
    { general_operand, general_operand, arith_operand, const_int_operand, },
    { register_operand, register_operand, register_operand, register_operand, arith_operand, const_int_operand, },
    { general_operand, 0, },
    { general_operand, general_operand, },
    { reg_or_nonsymb_mem_operand, reg_or_0_or_nonsymb_mem_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { reg_or_nonsymb_mem_operand, general_operand, },
    { register_operand, general_operand, register_operand, },
    { general_operand, register_operand, register_operand, },
    { register_operand, 0, },
    { reg_or_nonsymb_mem_operand, general_operand, },
    { register_operand, register_operand, immediate_operand, },
    { general_operand, 0, },
    { general_operand, general_operand, },
    { reg_or_nonsymb_mem_operand, reg_or_0_or_nonsymb_mem_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, reg_or_nonsymb_mem_operand, },
    { register_operand, reg_or_nonsymb_mem_operand, },
    { register_operand, reg_or_nonsymb_mem_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { general_operand, const_int_operand, },
    { general_operand, register_operand, },
    { general_operand, const_int_operand, },
    { general_operand, register_operand, },
    { general_operand, register_operand, },
    { general_operand, register_operand, },
    { general_operand, register_operand, },
    { general_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, arith11_operand, },
    { register_operand, register_operand, register_operand, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { register_operand, register_operand, arith_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, arith11_operand, register_operand, },
    { general_operand, move_operand, move_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, },
    { general_operand, move_operand, move_operand, register_operand, },
    { div_operand, register_operand, },
    { general_operand, move_operand, move_operand, register_operand, },
    { div_operand, register_operand, },
    { general_operand, move_operand, move_operand, register_operand, },
    { register_operand, },
    { general_operand, move_operand, move_operand, register_operand, },
    { register_operand, },
    { register_operand, arith_double_operand, arith_double_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, and_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, arith_double_operand, arith_double_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, arith32_operand, },
    { register_operand, register_operand, ior_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, arith_double_operand, arith_double_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, arith_double_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, memory_operand, },
    { register_operand, memory_operand, },
    { register_operand, register_operand, register_operand, shadd_operand, },
    { register_operand, register_operand, register_operand, const_int_operand, shadd_operand, },
    { register_operand, lhs_lshift_operand, arith32_operand, },
    { register_operand, register_operand, const_int_operand, },
    { register_operand, arith5_operand, register_operand, },
    { register_operand, lhs_lshift_cint_operand, register_operand, },
    { register_operand, register_operand, arith32_operand, },
    { register_operand, register_operand, const_int_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, arith32_operand, },
    { register_operand, register_operand, arith32_operand, },
    { register_operand, register_operand, const_int_operand, },
    { register_operand, register_operand, register_operand, const_int_operand, const_int_operand, plus_xor_ior_operator, },
    { register_operand, register_operand, register_operand, const_int_operand, const_int_operand, plus_xor_ior_operator, },
    { register_operand, register_operand, const_int_operand, const_int_operand, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { const_int_operand, },
    { 0 },
    { 0 },
    { general_operand, const_int_operand, const_int_operand, 0, 0, },
    { register_operand, arith11_operand, },
    { 0, 0, },
    { call_operand_address, 0, },
    { register_operand, 0, },
    { 0, 0, 0, },
    { 0, call_operand_address, 0, },
    { 0, register_operand, 0, },
    { 0 },
    { register_operand, },
    { register_operand, register_operand, uint5_operand, uint5_operand, },
    { register_operand, register_operand, uint5_operand, uint5_operand, },
    { register_operand, uint5_operand, uint5_operand, arith5_operand, },
    { register_operand, uint5_operand, uint5_operand, const_int_operand, },
    { register_operand, int5_operand, comparison_operator, 0, scratch_operand, },
    { register_operand, int5_operand, eq_neq_comparison_operator, 0, scratch_operand, const_int_operand, },
    { register_operand, register_operand, movb_comparison_operator, },
    { register_operand, register_operand, movb_comparison_operator, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, },
    { register_operand, register_operand, },
  };

const int insn_n_alternatives[] =
  {
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    2,
    1,
    1,
    1,
    2,
    3,
    2,
    3,
    2,
    5,
    8,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    1,
    1,
    1,
    10,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    1,
    1,
    1,
    1,
    0,
    0,
    8,
    1,
    1,
    1,
    1,
    1,
    0,
    8,
    1,
    1,
    0,
    2,
    2,
    0,
    8,
    1,
    1,
    0,
    1,
    1,
    1,
    9,
    2,
    2,
    0,
    6,
    1,
    1,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    2,
    1,
    2,
    0,
    1,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    2,
    1,
    1,
    0,
    1,
    0,
    1,
    1,
    0,
    1,
    1,
    1,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    2,
    1,
    0,
    1,
    1,
    2,
    2,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    0,
    1,
    1,
    0,
    1,
    1,
    1,
    2,
    1,
    3,
    3,
    3,
    3,
    1,
    1,
    1,
    1,
    1,
  };
