/* Generated automatically by the program `genemit'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "expr.h"
#include "real.h"
#include "output.h"
#include "insn-config.h"

#include "insn-flags.h"

#include "insn-codes.h"

extern char *insn_operand_constraint[][MAX_RECOG_OPERANDS];

extern rtx recog_operand[];
#define operands emit_operand

#define FAIL goto _fail

#define DONE goto _done

rtx
gen_cmpsi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
 hppa_compare_op0 = operands[0];
 hppa_compare_op1 = operands[1];
 hppa_branch_type = CMP_SI;
 DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, CCmode, 0), gen_rtx (COMPARE, CCmode, operand0, operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_cmpsf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  hppa_compare_op0 = operands[0];
  hppa_compare_op1 = operands[1];
  hppa_branch_type = CMP_SF;
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, CCFPmode, 0), gen_rtx (COMPARE, CCFPmode, operand0, operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_cmpdf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  hppa_compare_op0 = operands[0];
  hppa_compare_op1 = operands[1];
  hppa_branch_type = CMP_DF;
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, CCFPmode, 0), gen_rtx (COMPARE, CCFPmode, operand0, operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_seq (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  /* set up operands from compare.  */
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
  /* fall through and generate default code */
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (EQ, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sne (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (NE, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_slt (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LT, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sgt (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (GT, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sle (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LE, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sge (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  /* fp scc patterns rarely match, and are not a win on the PA.  */
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (GE, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sltu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LTU, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sgtu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (GTU, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sleu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LEU, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_sgeu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (GEU, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_scc (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (GET_CODE (operand3), SImode,
		operand1,
		operand2));
}

rtx
gen_negscc (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, VOIDmode, gen_rtx (GET_CODE (operand3), SImode,
		operand1,
		operand2)));
}

rtx
gen_incscc (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, gen_rtx (GET_CODE (operand4), SImode,
		operand2,
		operand3), operand1));
}

rtx
gen_decscc (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, SImode, operand1, gen_rtx (GET_CODE (operand4), SImode,
		operand2,
		operand3)));
}

rtx
gen_sminsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SMIN, SImode, operand1, operand2));
}

rtx
gen_uminsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (UMIN, SImode, operand1, operand2));
}

rtx
gen_smaxsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SMAX, SImode, operand1, operand2));
}

rtx
gen_umaxsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (UMAX, SImode, operand1, operand2));
}

rtx
gen_beq (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (EQ, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  /* set up operands from compare.  */
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
  /* fall through and generate default code */
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (EQ, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bne (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (NE, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (NE, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bgt (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (GT, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GT, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_blt (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (LT, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LT, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bge (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (GE, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GE, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_ble (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    {
      emit_insn (gen_cmp_fp (LE, hppa_compare_op0, hppa_compare_op1));
      emit_bcond_fp (NE, operands[0]);
      DONE;
    }
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LE, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bgtu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GTU, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bltu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LTU, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bgeu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GEU, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_bleu (operand0)
     rtx operand0;
{
  rtx operand1;
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;

{
  if (hppa_branch_type != CMP_SI)
    FAIL;
  operands[1] = hppa_compare_op0;
  operands[2] = hppa_compare_op1;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_jump_insn (gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LEU, VOIDmode, operand1, operand2), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movsi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, SImode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_reload_insi (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (emit_move_sequence (operands, SImode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves.  */
  emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_insn (gen_rtx (CLOBBER, VOIDmode, operand2));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_reload_outsi (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (emit_move_sequence (operands, SImode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves.  */
  emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_insn (gen_rtx (CLOBBER, VOIDmode, operand2));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_pre_ldwm (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, operand3, gen_rtx (MEM, SImode, gen_rtx (PLUS, SImode, operand1, operand2))),
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2))));
}

rtx
gen_pre_stwm (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, gen_rtx (MEM, SImode, gen_rtx (PLUS, SImode, operand1, operand2)), operand3),
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2))));
}

rtx
gen_post_ldwm (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, operand3, gen_rtx (MEM, SImode, operand1)),
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2))));
}

rtx
gen_post_stwm (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, gen_rtx (MEM, SImode, operand1), operand3),
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2))));
}

rtx
gen_add_high_const (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, VOIDmode, operand1, gen_rtx (HIGH, SImode, operand2)));
}

rtx
gen_split_65 (operands)
     rtx *operands;
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx _val = 0;
  start_sequence ();

  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (HIGH, SImode, operand1)));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LO_SUM, SImode, operand2, operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movhi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, HImode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movqi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, QImode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movstrsi (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operand4;
  rtx operand5;
  rtx operands[6];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  /* If the blocks are not at least word-aligned and rather big (>16 items),
     or the size is indeterminate, don't inline the copy code.  A
     procedure call is better since it can check the alignment at
     runtime and make the optimal decisions.  */
     if (INTVAL (operands[3]) < 4
	 && (GET_CODE (operands[2]) != CONST_INT
	     || (INTVAL (operands[2]) / INTVAL (operands[3]) > 16)))
       FAIL;

  operands[0] = copy_to_mode_reg (SImode, XEXP (operands[0], 0));
  operands[1] = copy_to_mode_reg (SImode, XEXP (operands[1], 0));
  operands[4] = gen_reg_rtx (SImode);
  operands[5] = gen_reg_rtx (SImode);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  operand5 = operands[5];
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (7,
		gen_rtx (SET, VOIDmode, gen_rtx (MEM, BLKmode, operand0), gen_rtx (MEM, BLKmode, operand1)),
		gen_rtx (CLOBBER, VOIDmode, operand0),
		gen_rtx (CLOBBER, VOIDmode, operand1),
		gen_rtx (CLOBBER, VOIDmode, operand4),
		gen_rtx (CLOBBER, VOIDmode, operand5),
		gen_rtx (USE, VOIDmode, operand2),
		gen_rtx (USE, VOIDmode, operand3))));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movdf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, DFmode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movdi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, DImode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_reload_indi (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (emit_move_sequence (operands, DImode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves.  */
  emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_insn (gen_rtx (CLOBBER, VOIDmode, operand2));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_reload_outdi (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (emit_move_sequence (operands, DImode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves.  */
  emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_insn (gen_rtx (CLOBBER, VOIDmode, operand2));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_movsf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (emit_move_sequence (operands, SFmode, 0))
    DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_zero_extendhisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTEND, SImode, operand1));
}

rtx
gen_zero_extendqihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTEND, HImode, operand1));
}

rtx
gen_zero_extendqisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTEND, SImode, operand1));
}

rtx
gen_extendhisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SIGN_EXTEND, SImode, operand1));
}

rtx
gen_extendqihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SIGN_EXTEND, HImode, operand1));
}

rtx
gen_extendqisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SIGN_EXTEND, SImode, operand1));
}

rtx
gen_extendsfdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT_EXTEND, DFmode, operand1));
}

rtx
gen_truncdfsf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT_TRUNCATE, SFmode, operand1));
}

rtx
gen_floatsisf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, SFmode, operand1));
}

rtx
gen_floatsidf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, DFmode, operand1));
}

rtx
gen_floatunssisf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
operands[2] = gen_reg_rtx (DImode);
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand2, 1), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand2, 0), const0_rtx));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, SFmode, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_floatunssidf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
operands[2] = gen_reg_rtx (DImode);
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand2, 1), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand2, 0), const0_rtx));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, DFmode, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_floatdisf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, SFmode, operand1));
}

rtx
gen_floatdidf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FLOAT, DFmode, operand1));
}

rtx
gen_fix_truncsfsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FIX, SImode, gen_rtx (FIX, SFmode, operand1)));
}

rtx
gen_fix_truncdfsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FIX, SImode, gen_rtx (FIX, DFmode, operand1)));
}

rtx
gen_fix_truncsfdi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FIX, DImode, gen_rtx (FIX, SFmode, operand1)));
}

rtx
gen_fix_truncdfdi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (FIX, DImode, gen_rtx (FIX, DFmode, operand1)));
}

rtx
gen_adddi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, DImode, operand1, operand2));
}

rtx
gen_split_117 (operands)
     rtx *operands;
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx _val = 0;
  start_sequence ();

{
  int val = INTVAL (operands[2]);
  int low = (val < 0) ? -0x2000 : 0x1fff;
  int rest = val - low;

  operands[2] = GEN_INT (rest);
  operands[3] = GEN_INT (low);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  emit_insn (gen_rtx (SET, VOIDmode, operand4, gen_rtx (PLUS, SImode, operand1, operand2)));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand4, operand3)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_split_118 (operands)
     rtx *operands;
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx _val = 0;
  start_sequence ();

{
  int intval = INTVAL (operands[2]);

  /* Try diving the constant by 2, then 4, and finally 8 to see
     if we can get a constant which can be loaded into a register
     in a single instruction (cint_ok_for_move).  */
  if (intval % 2 == 0 && cint_ok_for_move (intval / 2))
    {
      operands[2] = GEN_INT (INTVAL (operands[2]) / 2);
      operands[3] = GEN_INT (2);
    }
  else if (intval % 4 == 0 && cint_ok_for_move (intval / 4))
    {
      operands[2] = GEN_INT (INTVAL (operands[2]) / 4);
      operands[3] = GEN_INT (4);
    }
  else if (intval % 8 == 0 && cint_ok_for_move (intval / 8))
    {
      operands[2] = GEN_INT (INTVAL (operands[2]) / 8);
      operands[3] = GEN_INT (8);
    }
  else 
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  emit_insn (gen_rtx (SET, VOIDmode, operand4, operand2));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, gen_rtx (MULT, SImode, operand4, operand3), operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_addsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2));
}

rtx
gen_subdi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, DImode, operand1, operand2));
}

rtx
gen_subsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, SImode, operand1, operand2));
}

rtx
gen_mulsi3 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operands[4];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  if (TARGET_SNAKE && ! TARGET_DISABLE_FPREGS)
    {
      rtx scratch = gen_reg_rtx (DImode);
      operands[1] = force_reg (SImode, operands[1]);
      operands[2] = force_reg (SImode, operands[2]);
      emit_insn (gen_umulsidi3 (scratch, operands[1], operands[2]));
      emit_insn (gen_rtx (SET, VOIDmode,
			  operands[0],
			  gen_rtx (SUBREG, SImode, scratch, 1)));
      DONE;
    }
  operands[3] = gen_reg_rtx(SImode);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 26), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 25), operand2));
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (5,
		gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29), gen_rtx (MULT, SImode, gen_rtx (REG, SImode, 26), gen_rtx (REG, SImode, 25))),
		gen_rtx (CLOBBER, VOIDmode, operand3),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (REG, SImode, 29)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_umulsidi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MULT, DImode, gen_rtx (ZERO_EXTEND, DImode, operand1), gen_rtx (ZERO_EXTEND, DImode, operand2)));
}

rtx
gen_divsi3 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operands[4];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  operands[3] = gen_reg_rtx(SImode);
  if (!(GET_CODE (operands[2]) == CONST_INT && emit_hpdiv_const(operands, 0)))
    {
      emit_move_insn (gen_rtx (REG, SImode, 26), operands[1]);
      emit_move_insn (gen_rtx (REG, SImode, 25), operands[2]);
      emit
	(gen_rtx
	 (PARALLEL, VOIDmode,
	  gen_rtvec (5, gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29),
				 gen_rtx (DIV, SImode,
					  gen_rtx (REG, SImode, 26),
					  gen_rtx (REG, SImode, 25))),
		     gen_rtx (CLOBBER, VOIDmode, operands[3]),
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
      emit_move_insn (operands[0], gen_rtx (REG, SImode, 29));
    }
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 26), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 25), operand2));
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (5,
		gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29), gen_rtx (DIV, SImode, gen_rtx (REG, SImode, 26), gen_rtx (REG, SImode, 25))),
		gen_rtx (CLOBBER, VOIDmode, operand3),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (REG, SImode, 29)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_udivsi3 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operands[4];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  operands[3] = gen_reg_rtx(SImode);
  if (!(GET_CODE (operands[2]) == CONST_INT && emit_hpdiv_const(operands, 1)))
    {
      emit_move_insn (gen_rtx (REG, SImode, 26), operands[1]);
      emit_move_insn (gen_rtx (REG, SImode, 25), operands[2]);
      emit
	(gen_rtx
	 (PARALLEL, VOIDmode,
	  gen_rtvec (5, gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29),
				 gen_rtx (UDIV, SImode,
					  gen_rtx (REG, SImode, 26),
					  gen_rtx (REG, SImode, 25))),
		     gen_rtx (CLOBBER, VOIDmode, operands[3]), 
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		     gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
      emit_move_insn (operands[0], gen_rtx (REG, SImode, 29));
    }
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 26), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 25), operand2));
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (5,
		gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29), gen_rtx (UDIV, SImode, gen_rtx (REG, SImode, 26), gen_rtx (REG, SImode, 25))),
		gen_rtx (CLOBBER, VOIDmode, operand3),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (REG, SImode, 29)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_modsi3 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operands[4];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  operands[3] = gen_reg_rtx(SImode);
  emit_move_insn (gen_rtx (REG, SImode, 26), operands[1]);
  emit_move_insn (gen_rtx (REG, SImode, 25), operands[2]);
  emit
    (gen_rtx
     (PARALLEL, VOIDmode,
      gen_rtvec (5, gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29),
			     gen_rtx (MOD, SImode,
				      gen_rtx (REG, SImode, 26),
				      gen_rtx (REG, SImode, 25))),
		 gen_rtx (CLOBBER, VOIDmode, operands[3]), 
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_move_insn (operands[0], gen_rtx (REG, SImode, 29));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 26), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 25), operand2));
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (5,
		gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29), gen_rtx (MOD, SImode, gen_rtx (REG, SImode, 26), gen_rtx (REG, SImode, 25))),
		gen_rtx (CLOBBER, VOIDmode, operand3),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (REG, SImode, 29)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_umodsi3 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operands[4];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;

{
  operands[3] = gen_reg_rtx(SImode);
  emit_move_insn (gen_rtx (REG, SImode, 26), operands[1]);
  emit_move_insn (gen_rtx (REG, SImode, 25), operands[2]);
  emit
    (gen_rtx
     (PARALLEL, VOIDmode,
      gen_rtvec (5, gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29),
			     gen_rtx (UMOD, SImode,
				      gen_rtx (REG, SImode, 26),
				      gen_rtx (REG, SImode, 25))),
		 gen_rtx (CLOBBER, VOIDmode, operands[3]), 
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		 gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_move_insn (operands[0], gen_rtx (REG, SImode, 29));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 26), operand1));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 25), operand2));
  emit (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (5,
		gen_rtx (SET, VOIDmode, gen_rtx (REG, SImode, 29), gen_rtx (UMOD, SImode, gen_rtx (REG, SImode, 26), gen_rtx (REG, SImode, 25))),
		gen_rtx (CLOBBER, VOIDmode, operand3),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31)))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (REG, SImode, 29)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_anddi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (! register_operand (operands[1], DImode)
      || ! register_operand (operands[2], DImode))
    /* Let GCC break this into word-at-a-time operations.  */
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (AND, DImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_andsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (AND, SImode, operand1, operand2));
}

rtx
gen_iordi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (! register_operand (operands[1], DImode)
      || ! register_operand (operands[2], DImode))
    /* Let GCC break this into word-at-a-time operations.  */
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (IOR, DImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_iorsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (! (ior_operand (operands[2]) || register_operand (operands[2])))
    operands[2] = force_reg (SImode, operands[2]);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (IOR, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_xordi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (! register_operand (operands[1], DImode)
      || ! register_operand (operands[2], DImode))
    /* Let GCC break this into word-at-a-time operations.  */
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, DImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_xorsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, SImode, operand1, operand2));
}

rtx
gen_negdi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, DImode, operand1));
}

rtx
gen_negsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, SImode, operand1));
}

rtx
gen_one_cmpldi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (! register_operand (operands[1], DImode))
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (NOT, DImode, operand1)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_one_cmplsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NOT, SImode, operand1));
}

rtx
gen_adddf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, DFmode, operand1, operand2));
}

rtx
gen_addsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SFmode, operand1, operand2));
}

rtx
gen_subdf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, DFmode, operand1, operand2));
}

rtx
gen_subsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, SFmode, operand1, operand2));
}

rtx
gen_muldf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MULT, DFmode, operand1, operand2));
}

rtx
gen_mulsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MULT, SFmode, operand1, operand2));
}

rtx
gen_divdf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (DIV, DFmode, operand1, operand2));
}

rtx
gen_divsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (DIV, SFmode, operand1, operand2));
}

rtx
gen_negdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, DFmode, operand1));
}

rtx
gen_negsf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, SFmode, operand1));
}

rtx
gen_absdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ABS, DFmode, operand1));
}

rtx
gen_abssf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ABS, SFmode, operand1));
}

rtx
gen_sqrtdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SQRT, DFmode, operand1));
}

rtx
gen_sqrtsf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SQRT, SFmode, operand1));
}

rtx
gen_ashlsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT)
    {
      rtx temp = gen_reg_rtx (SImode);
      emit_insn (gen_subsi3 (temp, GEN_INT (31), operands[2]));
	if (GET_CODE (operands[1]) == CONST_INT)
	  emit_insn (gen_zvdep_imm (operands[0], operands[1], temp));
	else
	  emit_insn (gen_zvdep32 (operands[0], operands[1], temp));
      DONE;
    }
  /* Make sure both inputs are not constants, 
     the recognizer can't handle that.  */
  operands[1] = force_reg (SImode, operands[1]);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFT, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_zvdep32 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFT, SImode, operand1, gen_rtx (MINUS, SImode, GEN_INT (31), operand2)));
}

rtx
gen_zvdep_imm (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFT, SImode, operand1, gen_rtx (MINUS, SImode, GEN_INT (31), operand2)));
}

rtx
gen_ashrsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT)
    {
      rtx temp = gen_reg_rtx (SImode);
      emit_insn (gen_subsi3 (temp, GEN_INT (31), operands[2]));
      emit_insn (gen_vextrs32 (operands[0], operands[1], temp));
      DONE;
    }
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFTRT, SImode, operand1, operand2)));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_vextrs32 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFTRT, SImode, operand1, gen_rtx (MINUS, SImode, GEN_INT (31), operand2)));
}

rtx
gen_lshrsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (LSHIFTRT, SImode, operand1, operand2));
}

rtx
gen_rotrsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ROTATERT, SImode, operand1, operand2));
}

rtx
gen_rotlsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ROTATE, SImode, operand1, operand2));
}

rtx
gen_return ()
{
  return gen_rtx (RETURN, VOIDmode);
}

rtx
gen_return_internal ()
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (USE, VOIDmode, gen_rtx (REG, SImode, 2)),
		gen_rtx (RETURN, VOIDmode)));
}

rtx
gen_prologue ()
{
  rtx _val = 0;
  start_sequence ();
hppa_expand_prologue ();DONE;
  emit_insn (const0_rtx);
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_epilogue ()
{
  rtx _val = 0;
  start_sequence ();

{
  /* Try to use the trivial return first.  Else use the full 
     epilogue.  */
  if (hppa_can_use_return_insn_p ())
   emit_jump_insn (gen_return ());
  else
    {
      hppa_expand_epilogue ();
      emit_jump_insn (gen_return_internal ());
    }
  DONE;
}
  emit_jump_insn (gen_rtx (RETURN, VOIDmode));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_call_profiler (operand0)
     rtx operand0;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (UNSPEC_VOLATILE, VOIDmode, gen_rtvec (1,
		const0_rtx), 0),
		gen_rtx (USE, VOIDmode, operand0)));
}

rtx
gen_blockage ()
{
  return gen_rtx (UNSPEC_VOLATILE, VOIDmode, gen_rtvec (1,
		GEN_INT (2)), 0);
}

rtx
gen_jump (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (LABEL_REF, VOIDmode, operand0));
}

rtx
gen_casesi (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  rtx operands[5];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;
  operands[4] = operand4;

{
  if (GET_CODE (operands[0]) != REG)
    operands[0] = force_reg (SImode, operands[0]);

  if (operands[1] != const0_rtx)
    {
      rtx reg = gen_reg_rtx (SImode);

      operands[1] = GEN_INT (-INTVAL (operands[1]));
      if (!INT_14_BITS (operands[1]))
	operands[1] = force_reg (SImode, operands[1]);
      emit_insn (gen_addsi3 (reg, operands[0], operands[1]));

      operands[0] = reg;
    }

  if (!INT_11_BITS (operands[2]))
    operands[2] = force_reg (SImode, operands[2]);

  emit_jump_insn (gen_casesi0 (operands[0], operands[2],
			       operands[3], operands[4]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  emit (operand0);
  emit (operand1);
  emit (operand2);
  emit (operand3);
  emit (operand4);
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_casesi0 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LEU, VOIDmode, operand0, operand1), gen_rtx (PLUS, SImode, gen_rtx (MEM, SImode, gen_rtx (PLUS, SImode, pc_rtx, operand0)), gen_rtx (LABEL_REF, VOIDmode, operand2)), pc_rtx)),
		gen_rtx (USE, VOIDmode, gen_rtx (LABEL_REF, VOIDmode, operand3))));
}

rtx
gen_call (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;

{
  rtx op;
  
  if (TARGET_LONG_CALLS) 
    op = force_reg (SImode, XEXP (operands[0], 0));
  else
    op = XEXP (operands[0], 0);

  /* Use two different patterns for calls to explicitly named functions
     and calls through function pointers.  This is necessary as these two
     types of calls use different calling conventions, and CSE might try
     to change the named call into an indirect call in some cases (using
     two patterns keeps CSE from performing this optimization).  */
  if (GET_CODE (op) == SYMBOL_REF)
    emit_call_insn (gen_call_internal_symref (op, operands[1]));
  else
    emit_call_insn (gen_call_internal_reg (op, operands[1]));

  if (flag_pic)
    {
      if (!hppa_save_pic_table_rtx)
	hppa_save_pic_table_rtx = gen_reg_rtx (Pmode);
      emit_insn (gen_rtx (SET, VOIDmode,
			  gen_rtx (REG, Pmode, 19), hppa_save_pic_table_rtx));
    }
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_call_insn (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (CALL, VOIDmode, operand0, operand1),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)))));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_call_internal_symref (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (CALL, VOIDmode, gen_rtx (MEM, SImode, operand0), operand1),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)),
		gen_rtx (USE, VOIDmode, const0_rtx)));
}

rtx
gen_call_internal_reg (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (CALL, VOIDmode, gen_rtx (MEM, SImode, operand0), operand1),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)),
		gen_rtx (USE, VOIDmode, const1_rtx)));
}

rtx
gen_call_value (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];
  rtx _val = 0;
  start_sequence ();
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  rtx op;
  
  if (TARGET_LONG_CALLS) 
    op = force_reg (SImode, XEXP (operands[1], 0));
  else
    op = XEXP (operands[1], 0);

  /* Use two different patterns for calls to explicitly named functions
     and calls through function pointers.  This is necessary as these two
     types of calls use different calling conventions, and CSE might try
     to change the named call into an indirect call in some cases (using
     two patterns keeps CSE from performing this optimization).  */
  if (GET_CODE (op) == SYMBOL_REF)
    emit_call_insn (gen_call_value_internal_symref (operands[0], op,
						    operands[2]));
  else
    emit_call_insn (gen_call_value_internal_reg (operands[0], op, operands[2]));

  if (flag_pic)
    {
      if (!hppa_save_pic_table_rtx)
	hppa_save_pic_table_rtx = gen_reg_rtx (Pmode);
      emit_insn (gen_rtx (SET, VOIDmode,
			  gen_rtx (REG, Pmode, 19), hppa_save_pic_table_rtx));
    }
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_call_insn (gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (CALL, VOIDmode, operand1, operand2)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)))));
 _done:
  _val = gen_sequence ();
 _fail:
  end_sequence ();
  return _val;
}

rtx
gen_call_value_internal_symref (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (CALL, VOIDmode, gen_rtx (MEM, SImode, operand1), operand2)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)),
		gen_rtx (USE, VOIDmode, const0_rtx)));
}

rtx
gen_call_value_internal_reg (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (CALL, VOIDmode, gen_rtx (MEM, SImode, operand1), operand2)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 2)),
		gen_rtx (USE, VOIDmode, const1_rtx)));
}

rtx
gen_nop ()
{
  return const0_rtx;
}

rtx
gen_indirect_jump (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, operand0);
}

rtx
gen_extzv (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTRACT, SImode, operand1, operand2, operand3));
}

rtx
gen_extv (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (SIGN_EXTRACT, SImode, operand1, operand2, operand3));
}

rtx
gen_insv (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand0, operand1, operand2), operand3);
}

rtx
gen_decrement_and_branch_until_zero (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GET_CODE (operand2), VOIDmode,
		gen_rtx (PLUS, SImode, operand0, operand1),
		const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand3), pc_rtx)),
		gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand0, operand1)),
		gen_rtx (CLOBBER, VOIDmode, gen_rtx (SCRATCH, SImode, 0))));
}

rtx
gen_cacheflush (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (3,
		gen_rtx (UNSPEC_VOLATILE, VOIDmode, gen_rtvec (1,
		const1_rtx), 0),
		gen_rtx (USE, VOIDmode, gen_rtx (MEM, SImode, operand0)),
		gen_rtx (USE, VOIDmode, gen_rtx (MEM, SImode, operand1))));
}



void
add_clobbers (pattern, insn_code_number)
     rtx pattern;
     int insn_code_number;
{
  int i;

  switch (insn_code_number)
    {
    case 204:
    case 203:
      XVECEXP (pattern, 0, 2) = gen_rtx (CLOBBER, VOIDmode, gen_rtx (SCRATCH, SImode, 0));
      break;

    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
      XVECEXP (pattern, 0, 2) = gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 26));
      XVECEXP (pattern, 0, 3) = gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 25));
      XVECEXP (pattern, 0, 4) = gen_rtx (CLOBBER, VOIDmode, gen_rtx (REG, SImode, 31));
      break;

    case 57:
      XVECEXP (pattern, 0, 1) = gen_rtx (CLOBBER, VOIDmode, gen_rtx (SCRATCH, SImode, 0));
      break;

    default:
      abort ();
    }
}

void
init_mov_optab ()
{
#ifdef HAVE_movccfp
  if (HAVE_movccfp)
    mov_optab->handlers[(int) CCFPmode].insn_code = CODE_FOR_movccfp;
#endif
}
