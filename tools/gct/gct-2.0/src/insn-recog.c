/* Generated automatically by the program `genrecog'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"
#include "output.h"
#include "flags.h"

extern rtx gen_split_65 ();
extern rtx gen_split_117 ();
extern rtx gen_split_118 ();

/* `recog' contains a decision tree
   that recognizes whether the rtx X0 is a valid instruction.

   recog returns -1 if the rtx is not valid.
   If the rtx is valid, recog returns a nonnegative number
   which is the insn code number for the pattern that matched.
   This is the same as the order in the machine description of
   the entry that matched.  This number can be used as an index into
   entry that matched.  This number can be used as an index into various
   insn_* tables, such as insn_templates, insn_outfun, and insn_n_operands
   (found in insn-output.c).

   The third argument to recog is an optional pointer to an int.
   If present, recog will accept a pattern if it matches except for
   missing CLOBBER expressions at the end.  In that case, the value
   pointed to by the optional pointer will be set to the number of
   CLOBBERs that need to be added (it should be initialized to zero by
   the caller).  If it is set nonzero, the caller should allocate a
   PARALLEL of the appropriate size, copy the initial entries, and call
   add_clobbers (found in insn-emit.c) to fill in the CLOBBERs.

   The function split_insns returns 0 if the rtl could not
   be split or the split rtl in a SEQUENCE if it can be.*/

rtx recog_operand[MAX_RECOG_OPERANDS];

rtx *recog_operand_loc[MAX_RECOG_OPERANDS];

rtx *recog_dup_loc[MAX_DUP_OPERANDS];

char recog_dup_num[MAX_DUP_OPERANDS];

#define operands recog_operand

int
recog_1 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == SImode && GET_CODE (x1) == PLUS && 1)
    goto L24;
  L12:
  if (comparison_operator (x1, SImode))
    {
      ro[3] = x1;
      goto L13;
    }
  L17:
  switch (GET_CODE (x1))
    {
    case NEG:
      goto L18;
    }
  if (GET_MODE (x1) != SImode)
    goto ret0;
  switch (GET_CODE (x1))
    {
    case PLUS:
      goto L45;
    case MINUS:
      goto L52;
    case SMIN:
      goto L80;
    case UMIN:
      goto L85;
    case SMAX:
      goto L90;
    case UMAX:
      goto L95;
    case IF_THEN_ELSE:
      goto L100;
    case MEM:
      goto L174;
    }
  goto ret0;

  L24:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != SImode)
    {
      goto L12;
    }
  switch (GET_CODE (x2))
    {
    case LEU:
      goto L25;
    case GEU:
      goto L32;
    case GTU:
      goto L39;
    }
  goto L12;

  L25:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L26;
    }
  goto L12;

  L26:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L27;
    }
  goto L12;

  L27:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 17;
    }
  goto L12;

  L32:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L33;
    }
  goto L12;

  L33:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L34;
    }
  goto L12;

  L34:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 18;
    }
  goto L12;

  L39:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L40;
    }
  goto L12;

  L40:
  x3 = XEXP (x2, 1);
  if (int11_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L41;
    }
  goto L12;

  L41:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 19;
    }
  goto L12;

  L13:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L14;
    }
  goto L17;

  L14:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, SImode))
    {
      ro[2] = x2;
      return 15;
    }
  goto L17;

  L18:
  x2 = XEXP (x1, 0);
  if (comparison_operator (x2, SImode))
    {
      ro[3] = x2;
      goto L19;
    }
  goto ret0;

  L19:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L20;
    }
  goto ret0;

  L20:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[2] = x3;
      return 16;
    }
  goto ret0;

  L45:
  x2 = XEXP (x1, 0);
  if (comparison_operator (x2, SImode))
    {
      ro[4] = x2;
      goto L46;
    }
  goto ret0;

  L46:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L47;
    }
  goto ret0;

  L47:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L48;
    }
  goto ret0;

  L48:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 20;
    }
  goto ret0;

  L52:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L53;
    }
  goto ret0;

  L53:
  x2 = XEXP (x1, 1);
  switch (GET_MODE (x2))
    {
    case SImode:
      switch (GET_CODE (x2))
	{
	case GTU:
	  goto L54;
	case LTU:
	  goto L61;
	case LEU:
	  goto L68;
	}
    }
  L74:
  if (comparison_operator (x2, SImode))
    {
      ro[4] = x2;
      goto L75;
    }
  goto ret0;

  L54:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L55;
    }
  goto L74;

  L55:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[3] = x3;
      return 21;
    }
  goto L74;

  L61:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L62;
    }
  goto L74;

  L62:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[3] = x3;
      return 22;
    }
  goto L74;

  L68:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L69;
    }
  goto L74;

  L69:
  x3 = XEXP (x2, 1);
  if (int11_operand (x3, SImode))
    {
      ro[3] = x3;
      return 23;
    }
  goto L74;

  L75:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L76;
    }
  goto ret0;

  L76:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[3] = x3;
      return 24;
    }
  goto ret0;

  L80:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L81;
    }
  goto ret0;

  L81:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, SImode))
    {
      ro[2] = x2;
      return 25;
    }
  goto ret0;

  L85:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L86;
    }
  goto ret0;

  L86:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, SImode))
    {
      ro[2] = x2;
      return 26;
    }
  goto ret0;

  L90:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L91;
    }
  goto ret0;

  L91:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, SImode))
    {
      ro[2] = x2;
      return 27;
    }
  goto ret0;

  L95:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L96;
    }
  goto ret0;

  L96:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, SImode))
    {
      ro[2] = x2;
      return 28;
    }
  goto ret0;

  L100:
  x2 = XEXP (x1, 0);
  if (comparison_operator (x2, VOIDmode))
    {
      ro[5] = x2;
      goto L101;
    }
  goto ret0;

  L101:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L102;
    }
  goto ret0;

  L102:
  x3 = XEXP (x2, 1);
  if (arith11_operand (x3, SImode))
    {
      ro[4] = x3;
      goto L103;
    }
  goto ret0;

  L103:
  x2 = XEXP (x1, 1);
  if (reg_or_cint_move_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L104;
    }
  goto ret0;

  L104:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 0 && 1)
    return 29;
  L112:
  if (reg_or_cint_move_operand (x2, SImode))
    {
      ro[2] = x2;
      return 30;
    }
  goto ret0;

  L174:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L175;
  goto ret0;

  L175:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L176;
    }
  goto ret0;

  L176:
  x3 = XEXP (x2, 1);
  if (symbolic_operand (x3, SImode))
    {
      ro[2] = x3;
      if (flag_pic && operands[1] == pic_offset_table_rtx)
	return 50;
      }
  goto ret0;
 ret0: return -1;
}

int
recog_2 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == SImode && GET_CODE (x1) == MEM && 1)
    goto L183;
  L240:
  if (pnum_clobbers != 0 && pic_operand (x1, SImode))
    {
      ro[1] = x1;
      *pnum_clobbers = 1;
      return 57;
    }
  L243:
  switch (GET_MODE (x1))
    {
    case SImode:
      switch (GET_CODE (x1))
	{
	case HIGH:
	  goto L244;
	case LO_SUM:
	  goto L274;
	case ZERO_EXTEND:
	  goto L430;
	case SIGN_EXTEND:
	  goto L442;
	case FIX:
	  goto L486;
	case PLUS:
	  goto L511;
	}
    }
  switch (GET_CODE (x1))
    {
    case PLUS:
    L251:
      goto L252;
    }
  L534:
  switch (GET_MODE (x1))
    {
    case SImode:
      switch (GET_CODE (x1))
	{
	case PLUS:
	  goto L804;
	case MINUS:
	  goto L545;
	case AND:
	  goto L683;
	case IOR:
	  goto L694;
	case XOR:
	  goto L709;
	case NEG:
	  goto L718;
	case NOT:
	  goto L726;
	case LSHIFTRT:
	  goto L794;
	case ASHIFT:
	  goto L820;
	case ASHIFTRT:
	  goto L839;
	case ROTATERT:
	  goto L856;
	case ROTATE:
	  goto L861;
	case ZERO_EXTRACT:
	  goto L969;
	case SIGN_EXTRACT:
	  goto L975;
	}
    }
  L865:
  if (plus_xor_ior_operator (x1, SImode))
    {
      ro[5] = x1;
      goto L866;
    }
  goto ret0;

  L183:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L184;
  goto L240;

  L184:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L185;
  goto L240;

  L185:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L186;
    }
  goto L240;

  L186:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 4 && 1)
    goto L187;
  goto L240;

  L187:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      if (! TARGET_DISABLE_INDEXING)
	return 52;
      }
  goto L240;

  L244:
  x2 = XEXP (x1, 0);
  ro[1] = x2;
  if (TARGET_KERNEL && symbolic_operand(operands[1], Pmode)
   && ! function_label_operand (operands[1])
   && ! read_only_operand (operands[1]))
    return 58;
  L248:
  ro[1] = x2;
  if (! TARGET_KERNEL && symbolic_operand(operands[1], Pmode)
   && ! function_label_operand (operands[1])
   && ! read_only_operand (operands[1]))
    return 59;
  L258:
  if (function_label_operand (x2, SImode))
    {
      ro[1] = x2;
      return 61;
    }
  L262:
  ro[1] = x2;
  if (check_pic (1))
    return 62;
  goto L865;

  L274:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L275;
    }
  goto L865;

  L275:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    {
      ro[2] = x2;
      return 64;
    }
  goto L865;

  L430:
  x2 = XEXP (x1, 0);
  switch (GET_MODE (x2))
    {
    case HImode:
      if (reg_or_nonsymb_mem_operand (x2, HImode))
	{
	  ro[1] = x2;
	  return 95;
	}
      break;
    case QImode:
      if (reg_or_nonsymb_mem_operand (x2, QImode))
	{
	  ro[1] = x2;
	  return 97;
	}
    }
  goto L865;

  L442:
  x2 = XEXP (x1, 0);
  switch (GET_MODE (x2))
    {
    case HImode:
      if (register_operand (x2, HImode))
	{
	  ro[1] = x2;
	  return 98;
	}
      break;
    case QImode:
      if (register_operand (x2, QImode))
	{
	  ro[1] = x2;
	  return 100;
	}
    }
  goto L865;

  L486:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) != FIX)
    {
    goto L865;
    }
  switch (GET_MODE (x2))
    {
    case SFmode:
      goto L487;
    case DFmode:
      goto L492;
    }
  goto L865;

  L487:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    {
      ro[1] = x3;
      return 111;
    }
  goto L865;

  L492:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    {
      ro[1] = x3;
      return 112;
    }
  goto L865;

  L511:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == NOT && 1)
    goto L512;
  goto L251;

  L512:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L513;
    }
  goto L251;

  L513:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return 116;
    }
  goto L251;

  L252:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L253;
    }
  goto L534;

  L253:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == HIGH && 1)
    goto L254;
  goto L534;

  L254:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[2] = x3;
      if (reload_completed)
	return 60;
      }
  goto L534;

  L804:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != SImode)
    {
      goto L865;
    }
  switch (GET_CODE (x2))
    {
    case MULT:
      goto L805;
    case PLUS:
      goto L812;
    case SUBREG:
    case REG:
      if (register_operand (x2, SImode))
	{
	  ro[1] = x2;
	  goto L536;
	}
    }
  goto L865;

  L805:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L806;
    }
  goto L865;

  L806:
  x3 = XEXP (x2, 1);
  if (shadd_operand (x3, SImode))
    {
      ro[3] = x3;
      goto L807;
    }
  goto L865;

  L807:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 167;
    }
  goto L865;

  L812:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L813;
  goto L865;

  L813:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L814;
    }
  goto L865;

  L814:
  x4 = XEXP (x3, 1);
  if (shadd_operand (x4, SImode))
    {
      ro[4] = x4;
      goto L815;
    }
  goto L865;

  L815:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L816;
    }
  goto L865;

  L816:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[3] = x2;
      if (reload_in_progress)
	return 168;
      }
  goto L865;

  L536:
  x2 = XEXP (x1, 1);
  if (arith_operand (x2, SImode))
    {
      ro[2] = x2;
      return 119;
    }
  goto L865;

  L545:
  x2 = XEXP (x1, 0);
  if (arith11_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L546;
    }
  goto L865;

  L546:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return 121;
    }
  goto L865;

  L683:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != SImode)
    {
      goto L865;
    }
  switch (GET_CODE (x2))
    {
    case NOT:
      goto L684;
    case ASHIFT:
      goto L885;
    case SUBREG:
    case REG:
      if (register_operand (x2, SImode))
	{
	  ro[1] = x2;
	  goto L673;
	}
    }
  goto L865;

  L684:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L685;
    }
  goto L865;

  L685:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return 137;
    }
  goto L865;

  L885:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L886;
    }
  goto L865;

  L886:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[2] = x3;
      goto L887;
    }
  goto L865;

  L887:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[3] = x2;
      if (exact_log2 (1 + (INTVAL (operands[3]) >> (INTVAL (operands[2]) & 31))) >= 0)
	return 181;
      }
  goto L865;

  L673:
  x2 = XEXP (x1, 1);
  if (and_operand (x2, SImode))
    {
      ro[2] = x2;
      return 135;
    }
  goto L865;

  L694:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L695;
    }
  goto L865;

  L695:
  x2 = XEXP (x1, 1);
  if (ior_operand (x2, SImode))
    {
      ro[2] = x2;
      return 141;
    }
  L700:
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return 142;
    }
  goto L865;

  L709:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L710;
    }
  goto L865;

  L710:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return 145;
    }
  goto L865;

  L718:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 147;
    }
  goto L865;

  L726:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      return 150;
    }
  goto L865;

  L794:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != SImode)
    {
      goto L865;
    }
  if (memory_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L795;
    }
  L851:
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L852;
    }
  goto L865;

  L795:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) != CONST_INT)
    {
      x2 = XEXP (x1, 0);
    goto L851;
    }
  if (XWINT (x2, 0) == 24 && 1)
    return 165;
  if (XWINT (x2, 0) == 16 && 1)
    return 166;
  x2 = XEXP (x1, 0);
  goto L851;

  L852:
  x2 = XEXP (x1, 1);
  if (arith32_operand (x2, SImode))
    {
      ro[2] = x2;
      return 176;
    }
  goto L865;

  L820:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L821;
    }
  L825:
  if (arith5_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L826;
    }
  L832:
  if (lhs_lshift_cint_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L833;
    }
  goto L865;

  L821:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[2] = x2;
      return 170;
    }
  x2 = XEXP (x1, 0);
  goto L825;

  L826:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MINUS && 1)
    goto L827;
  x2 = XEXP (x1, 0);
  goto L832;

  L827:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 31 && 1)
    goto L828;
  x2 = XEXP (x1, 0);
  goto L832;

  L828:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      return 171;
    }
  x2 = XEXP (x1, 0);
  goto L832;

  L833:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MINUS && 1)
    goto L834;
  goto L865;

  L834:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 31 && 1)
    goto L835;
  goto L865;

  L835:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      return 172;
    }
  goto L865;

  L839:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L845;
    }
  goto L865;

  L845:
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case MINUS:
      if (GET_MODE (x2) == SImode && 1)
	goto L846;
      break;
    case CONST_INT:
      ro[2] = x2;
      return 174;
    }
  goto L865;

  L846:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 31 && 1)
    goto L847;
  goto L865;

  L847:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      return 175;
    }
  goto L865;

  L856:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L857;
    }
  goto L865;

  L857:
  x2 = XEXP (x1, 1);
  if (arith32_operand (x2, SImode))
    {
      ro[2] = x2;
      return 177;
    }
  goto L865;

  L861:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L862;
    }
  goto L865;

  L862:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[2] = x2;
      return 178;
    }
  goto L865;

  L969:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L970;
    }
  goto L865;

  L970:
  x2 = XEXP (x1, 1);
  if (uint5_operand (x2, SImode))
    {
      ro[2] = x2;
      goto L971;
    }
  goto L865;

  L971:
  x2 = XEXP (x1, 2);
  if (uint5_operand (x2, SImode))
    {
      ro[3] = x2;
      return 199;
    }
  goto L865;

  L975:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L976;
    }
  goto L865;

  L976:
  x2 = XEXP (x1, 1);
  if (uint5_operand (x2, SImode))
    {
      ro[2] = x2;
      goto L977;
    }
  goto L865;

  L977:
  x2 = XEXP (x1, 2);
  if (uint5_operand (x2, SImode))
    {
      ro[3] = x2;
      return 200;
    }
  goto L865;

  L866:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != SImode)
    goto ret0;
  switch (GET_CODE (x2))
    {
    case ASHIFT:
      goto L867;
    case LSHIFTRT:
      goto L876;
    }
  goto ret0;

  L867:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L868;
    }
  goto ret0;

  L868:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[3] = x3;
      goto L869;
    }
  goto ret0;

  L869:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == LSHIFTRT && 1)
    goto L870;
  goto ret0;

  L870:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L871;
    }
  goto ret0;

  L871:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[4] = x3;
      if (INTVAL (operands[3]) + INTVAL (operands[4]) == 32)
	return 179;
      }
  goto ret0;

  L876:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L877;
    }
  goto ret0;

  L877:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[4] = x3;
      goto L878;
    }
  goto ret0;

  L878:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == ASHIFT && 1)
    goto L879;
  goto ret0;

  L879:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L880;
    }
  goto ret0;

  L880:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[3] = x3;
      if (INTVAL (operands[3]) + INTVAL (operands[4]) == 32)
	return 180;
      }
  goto ret0;
 ret0: return -1;
}

int
recog_3 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XEXP (x0, 0);
  switch (GET_MODE (x1))
    {
    case CCFPmode:
      if (GET_CODE (x1) == REG && XINT (x1, 0) == 0 && 1)
	goto L2;
      break;
    case SImode:
      if (register_operand (x1, SImode))
	{
	  ro[0] = x1;
	  goto L23;
	}
    }
  if (GET_CODE (x1) == PC && 1)
    goto L115;
  L178:
  switch (GET_MODE (x1))
    {
    case SImode:
      if (reg_or_nonsymb_mem_operand (x1, SImode))
	{
	  ro[0] = x1;
	  goto L179;
	}
    L181:
      if (register_operand (x1, SImode))
	{
	  ro[0] = x1;
	  goto L182;
	}
      break;
    case HImode:
      if (reg_or_nonsymb_mem_operand (x1, HImode))
	{
	  ro[0] = x1;
	  goto L284;
	}
    L286:
      if (register_operand (x1, HImode))
	{
	  ro[0] = x1;
	  goto L287;
	}
      break;
    case QImode:
      if (reg_or_nonsymb_mem_operand (x1, QImode))
	{
	  ro[0] = x1;
	  goto L328;
	}
      break;
    case DFmode:
      if (general_operand (x1, DFmode))
	{
	  ro[0] = x1;
	  goto L373;
	}
    L375:
      if (reg_or_nonsymb_mem_operand (x1, DFmode))
	{
	  ro[0] = x1;
	  goto L376;
	}
    L378:
      if (register_operand (x1, DFmode))
	{
	  ro[0] = x1;
	  goto L379;
	}
    L468:
      if (general_operand (x1, DFmode))
	{
	  ro[0] = x1;
	  goto L469;
	}
    L386:
      if (GET_CODE (x1) == MEM && 1)
	goto L387;
      break;
    case DImode:
      if (register_operand (x1, DImode))
	{
	  ro[0] = x1;
	  goto L395;
	}
    L398:
      if (reg_or_nonsymb_mem_operand (x1, DImode))
	{
	  ro[0] = x1;
	  goto L399;
	}
      break;
    case SFmode:
      if (general_operand (x1, SFmode))
	{
	  ro[0] = x1;
	  goto L407;
	}
    L409:
      if (reg_or_nonsymb_mem_operand (x1, SFmode))
	{
	  ro[0] = x1;
	  goto L410;
	}
    L412:
      if (register_operand (x1, SFmode))
	{
	  ro[0] = x1;
	  goto L413;
	}
    L460:
      if (general_operand (x1, SFmode))
	{
	  ro[0] = x1;
	  goto L461;
	}
    L420:
      switch (GET_CODE (x1))
	{
	case MEM:
	  goto L421;
	}
    }
  switch (GET_CODE (x1))
    {
    case ZERO_EXTRACT:
      if (GET_MODE (x1) == SImode && 1)
	goto L980;
    }
  L901:
  if (GET_CODE (x1) == PC && 1)
    goto L965;
  goto ret0;

  L2:
  x1 = XEXP (x0, 1);
  if (comparison_operator (x1, CCFPmode))
    {
      ro[2] = x1;
      goto L3;
    }
  goto ret0;

  L3:
  x2 = XEXP (x1, 0);
  if (reg_or_0_operand (x2, SFmode))
    {
      ro[0] = x2;
      goto L4;
    }
  L8:
  if (reg_or_0_operand (x2, DFmode))
    {
      ro[0] = x2;
      goto L9;
    }
  goto ret0;

  L4:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, SFmode))
    {
      ro[1] = x2;
      return 3;
    }
  x2 = XEXP (x1, 0);
  goto L8;

  L9:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, DFmode))
    {
      ro[1] = x2;
      return 4;
    }
  goto ret0;
 L23:
  tem = recog_1 (x0, insn, pnum_clobbers);
  if (tem >= 0) return tem;
  x1 = XEXP (x0, 0);
  goto L178;

  L115:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == IF_THEN_ELSE && 1)
    goto L116;
  x1 = XEXP (x0, 0);
  goto L178;

  L116:
  x2 = XEXP (x1, 0);
  if (comparison_operator (x2, VOIDmode))
    {
      ro[3] = x2;
      goto L117;
    }
  L134:
  switch (GET_CODE (x2))
    {
    case NE:
      goto L135;
    case EQ:
      goto L146;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L117:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L118;
    }
  goto L134;

  L118:
  x3 = XEXP (x2, 1);
  if (arith5_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L119;
    }
  goto L134;

  L119:
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case LABEL_REF:
      goto L120;
    case PC:
      goto L129;
    }
  x2 = XEXP (x1, 0);
  goto L134;

  L120:
  x3 = XEXP (x2, 0);
  ro[0] = x3;
  goto L121;

  L121:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == PC && 1)
    return 41;
  x2 = XEXP (x1, 0);
  goto L134;

  L129:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L130;
  x2 = XEXP (x1, 0);
  goto L134;

  L130:
  x3 = XEXP (x2, 0);
  ro[0] = x3;
  return 42;

  L135:
  x3 = XEXP (x2, 0);
  switch (GET_MODE (x3))
    {
    case SImode:
      switch (GET_CODE (x3))
	{
	case ZERO_EXTRACT:
	  goto L136;
	}
      break;
    case CCFPmode:
      if (GET_CODE (x3) == REG && XINT (x3, 0) == 0 && 1)
	goto L158;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L136:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[0] = x4;
      goto L137;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L137:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 1 && 1)
    goto L138;
  x1 = XEXP (x0, 0);
  goto L178;

  L138:
  x4 = XEXP (x3, 2);
  if (uint5_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L139;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L139:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 0 && 1)
    goto L140;
  x1 = XEXP (x0, 0);
  goto L178;

  L140:
  x2 = XEXP (x1, 1);
  if (pc_or_label_operand (x2, VOIDmode))
    {
      ro[2] = x2;
      goto L141;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L141:
  x2 = XEXP (x1, 2);
  if (pc_or_label_operand (x2, VOIDmode))
    {
      ro[3] = x2;
      return 43;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L158:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 0 && 1)
    goto L159;
  x1 = XEXP (x0, 0);
  goto L178;

  L159:
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case LABEL_REF:
      goto L160;
    case PC:
      goto L169;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L160:
  x3 = XEXP (x2, 0);
  ro[0] = x3;
  goto L161;

  L161:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == PC && 1)
    return 45;
  x1 = XEXP (x0, 0);
  goto L178;

  L169:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L170;
  x1 = XEXP (x0, 0);
  goto L178;

  L170:
  x3 = XEXP (x2, 0);
  ro[0] = x3;
  return 46;

  L146:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == ZERO_EXTRACT && 1)
    goto L147;
  x1 = XEXP (x0, 0);
  goto L178;

  L147:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[0] = x4;
      goto L148;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L148:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 1 && 1)
    goto L149;
  x1 = XEXP (x0, 0);
  goto L178;

  L149:
  x4 = XEXP (x3, 2);
  if (uint5_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L150;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L150:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XWINT (x3, 0) == 0 && 1)
    goto L151;
  x1 = XEXP (x0, 0);
  goto L178;

  L151:
  x2 = XEXP (x1, 1);
  if (pc_or_label_operand (x2, VOIDmode))
    {
      ro[2] = x2;
      goto L152;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L152:
  x2 = XEXP (x1, 2);
  if (pc_or_label_operand (x2, VOIDmode))
    {
      ro[3] = x2;
      return 44;
    }
  x1 = XEXP (x0, 0);
  goto L178;

  L179:
  x1 = XEXP (x0, 1);
  if (move_operand (x1, SImode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], SImode)
   || reg_or_0_operand (operands[1], SImode))
	return 51;
      }
  x1 = XEXP (x0, 0);
  goto L181;
 L182:
  return recog_2 (x0, insn, pnum_clobbers);

  L284:
  x1 = XEXP (x0, 1);
  if (move_operand (x1, HImode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], HImode)
   || reg_or_0_operand (operands[1], HImode))
	return 67;
      }
  x1 = XEXP (x0, 0);
  goto L286;

  L287:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != HImode)
    goto ret0;
  switch (GET_CODE (x1))
    {
    case MEM:
      goto L288;
    case HIGH:
      goto L320;
    case LO_SUM:
      goto L324;
    case ZERO_EXTEND:
      goto L434;
    case SIGN_EXTEND:
      goto L446;
    }
  goto ret0;

  L288:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L289;
  goto ret0;

  L289:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L290;
  goto ret0;

  L290:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L291;
    }
  goto ret0;

  L291:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 2 && 1)
    goto L292;
  goto ret0;

  L292:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      if (! TARGET_DISABLE_INDEXING)
	return 68;
      }
  goto ret0;

  L320:
  x2 = XEXP (x1, 0);
  ro[1] = x2;
  if (check_pic (1))
    return 71;
  goto ret0;

  L324:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, HImode))
    {
      ro[1] = x2;
      goto L325;
    }
  goto ret0;

  L325:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, VOIDmode))
    {
      ro[2] = x2;
      return 72;
    }
  goto ret0;

  L434:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == QImode && reg_or_nonsymb_mem_operand (x2, QImode))
    {
      ro[1] = x2;
      return 96;
    }
  goto ret0;

  L446:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, QImode))
    {
      ro[1] = x2;
      return 99;
    }
  goto ret0;

  L328:
  x1 = XEXP (x0, 1);
  if (move_operand (x1, QImode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], QImode)
   || reg_or_0_operand (operands[1], QImode))
	return 74;
      }
  x1 = XEXP (x0, 0);
  goto L901;

  L373:
  x1 = XEXP (x0, 1);
  ro[1] = x1;
  if (GET_CODE (operands[1]) == CONST_DOUBLE
   && operands[1] != CONST0_RTX (DFmode))
    return 79;
  x1 = XEXP (x0, 0);
  goto L375;

  L376:
  x1 = XEXP (x0, 1);
  if (reg_or_0_or_nonsymb_mem_operand (x1, DFmode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], DFmode)
   || reg_or_0_operand (operands[1], DFmode))
	return 81;
      }
  x1 = XEXP (x0, 0);
  goto L378;

  L379:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != DFmode)
    {
      x1 = XEXP (x0, 0);
      goto L468;
    }
  switch (GET_CODE (x1))
    {
    case MEM:
      goto L380;
    case FLOAT_EXTEND:
      goto L454;
    case PLUS:
      goto L730;
    case MINUS:
      goto L740;
    case MULT:
      goto L750;
    case DIV:
      goto L760;
    case NEG:
      goto L770;
    case ABS:
      goto L778;
    case SQRT:
      goto L786;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L380:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L381;
  x1 = XEXP (x0, 0);
  goto L468;

  L381:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L382;
  x1 = XEXP (x0, 0);
  goto L468;

  L382:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L383;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L383:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 8 && 1)
    goto L384;
  x1 = XEXP (x0, 0);
  goto L468;

  L384:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      if (! TARGET_DISABLE_INDEXING)
	return 82;
      }
  x1 = XEXP (x0, 0);
  goto L468;

  L454:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      return 101;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L730:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      goto L731;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L731:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    {
      ro[2] = x2;
      return 151;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L740:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      goto L741;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L741:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    {
      ro[2] = x2;
      return 153;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L750:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      goto L751;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L751:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    {
      ro[2] = x2;
      return 155;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L760:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      goto L761;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L761:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    {
      ro[2] = x2;
      return 157;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L770:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      return 159;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L778:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      return 161;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L786:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      return 163;
    }
  x1 = XEXP (x0, 0);
  goto L468;

  L469:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == DFmode && GET_CODE (x1) == FLOAT && 1)
    goto L474;
  x1 = XEXP (x0, 0);
  goto L386;

  L474:
  x2 = XEXP (x1, 0);
  switch (GET_MODE (x2))
    {
    case SImode:
      if (register_operand (x2, SImode))
	{
	  ro[1] = x2;
	  return 106;
	}
      break;
    case DImode:
      if (register_operand (x2, DImode))
	{
	  ro[1] = x2;
	  if (TARGET_SNAKE)
	    return 110;
	  }
    }
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[1] = x2;
      return 105;
    }
  x1 = XEXP (x0, 0);
  goto L386;

  L387:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L388;
  goto ret0;

  L388:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L389;
  goto ret0;

  L389:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L390;
    }
  goto ret0;

  L390:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 8 && 1)
    goto L391;
  goto ret0;

  L391:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L392;
    }
  goto ret0;

  L392:
  x1 = XEXP (x0, 1);
  if (register_operand (x1, DFmode))
    {
      ro[0] = x1;
      if (! TARGET_DISABLE_INDEXING)
	return 83;
      }
  goto ret0;

  L395:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != DImode)
    {
      x1 = XEXP (x0, 0);
      goto L398;
    }
  switch (GET_CODE (x1))
    {
    case HIGH:
      goto L396;
    case LO_SUM:
      goto L403;
    case FIX:
      goto L496;
    case PLUS:
      goto L506;
    case MINUS:
      goto L540;
    case MULT:
      goto L550;
    case AND:
      goto L677;
    case IOR:
      goto L689;
    case XOR:
      goto L704;
    case NEG:
      goto L714;
    case NOT:
      goto L722;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L396:
  x2 = XEXP (x1, 0);
  ro[1] = x2;
  if (check_pic (1))
    return 87;
  x1 = XEXP (x0, 0);
  goto L398;

  L403:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L404;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L404:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, DImode))
    {
      ro[2] = x2;
      return 89;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L496:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) != FIX)
    {
      x1 = XEXP (x0, 0);
    goto L398;
    }
  switch (GET_MODE (x2))
    {
    case SFmode:
      goto L497;
    case DFmode:
      goto L502;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L497:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    {
      ro[1] = x3;
      if (TARGET_SNAKE)
	return 113;
      }
  x1 = XEXP (x0, 0);
  goto L398;

  L502:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    {
      ro[1] = x3;
      if (TARGET_SNAKE)
	return 114;
      }
  x1 = XEXP (x0, 0);
  goto L398;

  L506:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L507;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L507:
  x2 = XEXP (x1, 1);
  if (arith11_operand (x2, DImode))
    {
      ro[2] = x2;
      return 115;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L540:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L541;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L541:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DImode))
    {
      ro[2] = x2;
      return 120;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L550:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == DImode && GET_CODE (x2) == ZERO_EXTEND && 1)
    goto L551;
  x1 = XEXP (x0, 0);
  goto L398;

  L551:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L552;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L552:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == DImode && GET_CODE (x2) == ZERO_EXTEND && 1)
    goto L553;
  x1 = XEXP (x0, 0);
  goto L398;

  L553:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      if (TARGET_SNAKE && ! TARGET_DISABLE_FPREGS)
	return 123;
      }
  x1 = XEXP (x0, 0);
  goto L398;

  L677:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != DImode)
    {
      x1 = XEXP (x0, 0);
      goto L398;
    }
  if (GET_CODE (x2) == NOT && 1)
    goto L678;
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L668;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L678:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DImode))
    {
      ro[1] = x3;
      goto L679;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L679:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DImode))
    {
      ro[2] = x2;
      return 136;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L668:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DImode))
    {
      ro[2] = x2;
      return 134;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L689:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L690;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L690:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DImode))
    {
      ro[2] = x2;
      return 139;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L704:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      goto L705;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L705:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DImode))
    {
      ro[2] = x2;
      return 144;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L714:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      return 146;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L722:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DImode))
    {
      ro[1] = x2;
      return 149;
    }
  x1 = XEXP (x0, 0);
  goto L398;

  L399:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DImode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], DImode)
   || reg_or_0_operand (operands[1], DImode))
	return 88;
      }
  x1 = XEXP (x0, 0);
  goto L901;

  L407:
  x1 = XEXP (x0, 1);
  ro[1] = x1;
  if (GET_CODE (operands[1]) == CONST_DOUBLE
   && operands[1] != CONST0_RTX (SFmode))
    return 90;
  x1 = XEXP (x0, 0);
  goto L409;

  L410:
  x1 = XEXP (x0, 1);
  if (reg_or_0_or_nonsymb_mem_operand (x1, SFmode))
    {
      ro[1] = x1;
      if (register_operand (operands[0], SFmode)
   || reg_or_0_operand (operands[1], SFmode))
	return 92;
      }
  x1 = XEXP (x0, 0);
  goto L412;

  L413:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SFmode)
    {
      x1 = XEXP (x0, 0);
      goto L460;
    }
  switch (GET_CODE (x1))
    {
    case MEM:
      goto L414;
    case FLOAT_TRUNCATE:
      goto L458;
    case PLUS:
      goto L735;
    case MINUS:
      goto L745;
    case MULT:
      goto L755;
    case DIV:
      goto L765;
    case NEG:
      goto L774;
    case ABS:
      goto L782;
    case SQRT:
      goto L790;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L414:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L415;
  x1 = XEXP (x0, 0);
  goto L460;

  L415:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L416;
  x1 = XEXP (x0, 0);
  goto L460;

  L416:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L417;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L417:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 4 && 1)
    goto L418;
  x1 = XEXP (x0, 0);
  goto L460;

  L418:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      if (! TARGET_DISABLE_INDEXING)
	return 93;
      }
  x1 = XEXP (x0, 0);
  goto L460;

  L458:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    {
      ro[1] = x2;
      return 102;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L735:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      goto L736;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L736:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    {
      ro[2] = x2;
      return 152;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L745:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      goto L746;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L746:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    {
      ro[2] = x2;
      return 154;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L755:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      goto L756;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L756:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    {
      ro[2] = x2;
      return 156;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L765:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      goto L766;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L766:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    {
      ro[2] = x2;
      return 158;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L774:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      return 160;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L782:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      return 162;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L790:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    {
      ro[1] = x2;
      return 164;
    }
  x1 = XEXP (x0, 0);
  goto L460;

  L461:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == SFmode && GET_CODE (x1) == FLOAT && 1)
    goto L466;
  x1 = XEXP (x0, 0);
  goto L420;

  L466:
  x2 = XEXP (x1, 0);
  switch (GET_MODE (x2))
    {
    case SImode:
      if (register_operand (x2, SImode))
	{
	  ro[1] = x2;
	  return 104;
	}
      break;
    case DImode:
      if (register_operand (x2, DImode))
	{
	  ro[1] = x2;
	  if (TARGET_SNAKE)
	    return 109;
	  }
    }
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[1] = x2;
      return 103;
    }
  x1 = XEXP (x0, 0);
  goto L420;

  L421:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L422;
  goto ret0;

  L422:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MULT && 1)
    goto L423;
  goto ret0;

  L423:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L424;
    }
  goto ret0;

  L424:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 4 && 1)
    goto L425;
  goto ret0;

  L425:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L426;
    }
  goto ret0;

  L426:
  x1 = XEXP (x0, 1);
  if (register_operand (x1, SFmode))
    {
      ro[0] = x1;
      if (! TARGET_DISABLE_INDEXING)
	return 94;
      }
  goto ret0;

  L980:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L981;
    }
  goto ret0;

  L981:
  x2 = XEXP (x1, 1);
  if (uint5_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L982;
    }
  goto ret0;

  L982:
  x2 = XEXP (x1, 2);
  if (uint5_operand (x2, SImode))
    {
      ro[2] = x2;
      goto L983;
    }
  goto ret0;

  L983:
  x1 = XEXP (x0, 1);
  if (arith5_operand (x1, SImode))
    {
      ro[3] = x1;
      return 201;
    }
  L989:
  if (GET_CODE (x1) == CONST_INT && 1)
    {
      ro[3] = x1;
      if ((INTVAL (operands[3]) & 0x10) != 0 &&
   (~INTVAL (operands[3]) & (1L << INTVAL (operands[1])) - 1 & ~0xf) == 0)
	return 202;
      }
  goto ret0;

  L965:
  x1 = XEXP (x0, 1);
  if (register_operand (x1, SImode))
    {
      ro[0] = x1;
      return 198;
    }
  if (GET_CODE (x1) == LABEL_REF && 1)
    goto L903;
  goto ret0;

  L903:
  x2 = XEXP (x1, 0);
  ro[0] = x2;
  return 188;
 ret0: return -1;
}

int
recog_4 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == LEU && 1)
    goto L909;
  L1013:
  if (comparison_operator (x3, VOIDmode))
    {
      ro[2] = x3;
      goto L1014;
    }
  L1047:
  if (eq_neq_comparison_operator (x3, VOIDmode))
    {
      ro[2] = x3;
      goto L1048;
    }
  L1062:
  if (movb_comparison_operator (x3, VOIDmode))
    {
      ro[2] = x3;
      goto L1063;
    }
  goto ret0;

  L909:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[0] = x4;
      goto L910;
    }
  goto L1013;

  L910:
  x4 = XEXP (x3, 1);
  if (arith11_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L911;
    }
  goto L1013;

  L911:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == PLUS && 1)
    goto L912;
  x3 = XEXP (x2, 0);
  goto L1013;

  L912:
  x4 = XEXP (x3, 0);
  if (GET_MODE (x4) == SImode && GET_CODE (x4) == MEM && 1)
    goto L913;
  x3 = XEXP (x2, 0);
  goto L1013;

  L913:
  x5 = XEXP (x4, 0);
  if (GET_MODE (x5) == SImode && GET_CODE (x5) == PLUS && 1)
    goto L914;
  x3 = XEXP (x2, 0);
  goto L1013;

  L914:
  x6 = XEXP (x5, 0);
  if (GET_CODE (x6) == PC && 1)
    goto L915;
  x3 = XEXP (x2, 0);
  goto L1013;

  L915:
  x6 = XEXP (x5, 1);
  if (rtx_equal_p (x6, ro[0]) && 1)
    goto L916;
  x3 = XEXP (x2, 0);
  goto L1013;

  L916:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == LABEL_REF && 1)
    goto L917;
  x3 = XEXP (x2, 0);
  goto L1013;

  L917:
  x5 = XEXP (x4, 0);
  ro[2] = x5;
  goto L918;

  L918:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L919;
  x3 = XEXP (x2, 0);
  goto L1013;

  L919:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L920;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1013;

  L920:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L921;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1013;

  L921:
  x3 = XEXP (x2, 0);
  ro[3] = x3;
  return 190;

  L1014:
  x4 = XEXP (x3, 0);
  if (GET_MODE (x4) == SImode && GET_CODE (x4) == PLUS && 1)
    goto L1015;
  goto L1047;

  L1015:
  x5 = XEXP (x4, 0);
  if (register_operand (x5, SImode))
    {
      ro[0] = x5;
      goto L1016;
    }
  goto L1047;

  L1016:
  x5 = XEXP (x4, 1);
  if (int5_operand (x5, SImode))
    {
      ro[1] = x5;
      goto L1017;
    }
  goto L1047;

  L1017:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 0 && 1)
    goto L1018;
  goto L1047;

  L1018:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1019;
  x3 = XEXP (x2, 0);
  goto L1047;

  L1019:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1020;

  L1020:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L1021;
  x3 = XEXP (x2, 0);
  goto L1047;

  L1021:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1022;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1047;

  L1022:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[0]) && 1)
    goto L1023;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1047;

  L1023:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L1024;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1047;

  L1024:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[0]) && 1)
    goto L1025;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1047;

  L1025:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[1]) && pnum_clobbers != 0 && 1)
    {
      *pnum_clobbers = 1;
      return 203;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1047;

  L1048:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[0] = x4;
      goto L1049;
    }
  goto L1062;

  L1049:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && 1)
    {
      ro[5] = x4;
      goto L1050;
    }
  goto L1062;

  L1050:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1051;
  x3 = XEXP (x2, 0);
  goto L1062;

  L1051:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1052;

  L1052:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L1053;
  x3 = XEXP (x2, 0);
  goto L1062;

  L1053:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1054;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1062;

  L1054:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[0]) && 1)
    goto L1055;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1062;

  L1055:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L1056;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1062;

  L1056:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[0]) && 1)
    goto L1057;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1062;

  L1057:
  x3 = XEXP (x2, 1);
  if (pnum_clobbers != 0 && int5_operand (x3, SImode))
    {
      ro[1] = x3;
      if (INTVAL (operands[5]) == - INTVAL (operands[1]))
	{
	  *pnum_clobbers = 1;
	  return 204;
	}
      }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1062;

  L1063:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L1064;
    }
  goto ret0;

  L1064:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 0 && 1)
    goto L1065;
  goto ret0;

  L1065:
  x3 = XEXP (x2, 1);
  switch (GET_CODE (x3))
    {
    case LABEL_REF:
      goto L1066;
    case PC:
      goto L1079;
    }
  goto ret0;

  L1066:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1067;

  L1067:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L1068;
  goto ret0;

  L1068:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1069;
  goto ret0;

  L1069:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L1070;
    }
  goto ret0;

  L1070:
  x2 = XEXP (x1, 1);
  if (rtx_equal_p (x2, ro[1]) && 1)
    return 205;
  goto ret0;

  L1079:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1080;
  goto ret0;

  L1080:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1081;

  L1081:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1082;
  goto ret0;

  L1082:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L1083;
    }
  goto ret0;

  L1083:
  x2 = XEXP (x1, 1);
  if (rtx_equal_p (x2, ro[1]) && 1)
    return 206;
  goto ret0;
 ret0: return -1;
}

int
recog_5 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  switch (GET_MODE (x2))
    {
    case SImode:
      switch (GET_CODE (x2))
	{
	case MEM:
	  goto L203;
	case REG:
	  if (XINT (x2, 0) == 29 && 1)
	    goto L571;
	}
    L190:
      if (register_operand (x2, SImode))
	{
	  ro[3] = x2;
	  goto L191;
	}
    L234:
      if (register_operand (x2, SImode))
	{
	  ro[0] = x2;
	  goto L266;
	}
      break;
    case HImode:
      if (GET_CODE (x2) == MEM && 1)
	goto L308;
      if (register_operand (x2, HImode))
	{
	  ro[3] = x2;
	  goto L296;
	}
      break;
    case QImode:
      if (GET_CODE (x2) == MEM && 1)
	goto L344;
      if (register_operand (x2, QImode))
	{
	  ro[3] = x2;
	  goto L332;
	}
    }
  if (GET_CODE (x2) == PC && 1)
    goto L907;
  goto ret0;

  L203:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) != SImode)
    goto ret0;
  if (GET_CODE (x3) == PLUS && 1)
    goto L204;
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L226;
    }
  goto ret0;

  L204:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L205;
    }
  goto ret0;

  L205:
  x4 = XEXP (x3, 1);
  if (pre_cint_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L206;
    }
  goto ret0;

  L206:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, SImode))
    {
      ro[3] = x2;
      goto L207;
    }
  goto ret0;

  L207:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L208;
  goto ret0;

  L208:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L209;
    }
  goto ret0;

  L209:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L210;
  goto ret0;

  L210:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L211;
  goto ret0;

  L211:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 54;
  goto ret0;

  L226:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, SImode))
    {
      ro[3] = x2;
      goto L227;
    }
  goto ret0;

  L227:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L228;
  goto ret0;

  L228:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L229;
    }
  goto ret0;

  L229:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L230;
  goto ret0;

  L230:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L231;
  goto ret0;

  L231:
  x3 = XEXP (x2, 1);
  if (post_cint_operand (x3, SImode))
    {
      ro[2] = x3;
      return 56;
    }
  goto ret0;

  L571:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != SImode)
    {
      x2 = XEXP (x1, 0);
      goto L190;
    }
  switch (GET_CODE (x2))
    {
    case MULT:
      goto L572;
    case DIV:
      goto L594;
    case UDIV:
      goto L616;
    case MOD:
      goto L638;
    case UMOD:
      goto L660;
    }
  x2 = XEXP (x1, 0);
  goto L190;

  L572:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L573;
  x2 = XEXP (x1, 0);
  goto L190;

  L573:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L574;
  x2 = XEXP (x1, 0);
  goto L190;

  L574:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L575;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L575:
  x2 = XEXP (x1, 0);
  if (pnum_clobbers != 0 && register_operand (x2, SImode))
    {
      ro[0] = x2;
      *pnum_clobbers = 3;
      return 124;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L594:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L595;
  x2 = XEXP (x1, 0);
  goto L190;

  L595:
  x3 = XEXP (x2, 1);
  if (div_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L596;
    }
  x2 = XEXP (x1, 0);
  goto L190;

  L596:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L597;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L597:
  x2 = XEXP (x1, 0);
  if (pnum_clobbers != 0 && register_operand (x2, SImode))
    {
      ro[1] = x2;
      *pnum_clobbers = 3;
      return 126;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L616:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L617;
  x2 = XEXP (x1, 0);
  goto L190;

  L617:
  x3 = XEXP (x2, 1);
  if (div_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L618;
    }
  x2 = XEXP (x1, 0);
  goto L190;

  L618:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L619;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L619:
  x2 = XEXP (x1, 0);
  if (pnum_clobbers != 0 && register_operand (x2, SImode))
    {
      ro[1] = x2;
      *pnum_clobbers = 3;
      return 128;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L638:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L639;
  x2 = XEXP (x1, 0);
  goto L190;

  L639:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L640;
  x2 = XEXP (x1, 0);
  goto L190;

  L640:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L641;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L641:
  x2 = XEXP (x1, 0);
  if (pnum_clobbers != 0 && register_operand (x2, SImode))
    {
      ro[0] = x2;
      *pnum_clobbers = 3;
      return 130;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L660:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L661;
  x2 = XEXP (x1, 0);
  goto L190;

  L661:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L662;
  x2 = XEXP (x1, 0);
  goto L190;

  L662:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L663;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L663:
  x2 = XEXP (x1, 0);
  if (pnum_clobbers != 0 && register_operand (x2, SImode))
    {
      ro[0] = x2;
      *pnum_clobbers = 3;
      return 132;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L190;

  L191:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MEM && 1)
    goto L192;
  x2 = XEXP (x1, 0);
  goto L234;

  L192:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) != SImode)
    {
      x2 = XEXP (x1, 0);
      goto L234;
    }
  if (GET_CODE (x3) == PLUS && 1)
    goto L193;
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L217;
    }
  x2 = XEXP (x1, 0);
  goto L234;

  L193:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L194;
    }
  x2 = XEXP (x1, 0);
  goto L234;

  L194:
  x4 = XEXP (x3, 1);
  if (pre_cint_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L195;
    }
  x2 = XEXP (x1, 0);
  goto L234;

  L195:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L196;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L196:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L197;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L197:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L198;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L198:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L199;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L199:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 53;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L217:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L218;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L218:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L219;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L219:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L220;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L220:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L221;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L221:
  x3 = XEXP (x2, 1);
  if (post_cint_operand (x3, SImode))
    {
      ro[2] = x3;
      return 55;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L234;

  L266:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == LO_SUM && 1)
    goto L267;
  L235:
  if (pic_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L236;
    }
  goto ret0;

  L267:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L268;
    }
  goto L235;

  L268:
  x3 = XEXP (x2, 1);
  if (function_label_operand (x3, SImode))
    {
      ro[2] = x3;
      goto L269;
    }
  goto L235;

  L269:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L270;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L235;

  L270:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[3] = x2;
      return 63;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L235;

  L236:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L237;
  goto ret0;

  L237:
  x2 = XEXP (x1, 0);
  if (scratch_operand (x2, SImode))
    {
      ro[2] = x2;
      return 57;
    }
  goto ret0;

  L308:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == PLUS && 1)
    goto L309;
  goto ret0;

  L309:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L310;
    }
  goto ret0;

  L310:
  x4 = XEXP (x3, 1);
  if (int5_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L311;
    }
  goto ret0;

  L311:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, HImode))
    {
      ro[3] = x2;
      goto L312;
    }
  goto ret0;

  L312:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L313;
  goto ret0;

  L313:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L314;
    }
  goto ret0;

  L314:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L315;
  goto ret0;

  L315:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L316;
  goto ret0;

  L316:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 70;
  goto ret0;

  L296:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == HImode && GET_CODE (x2) == MEM && 1)
    goto L297;
  goto ret0;

  L297:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == PLUS && 1)
    goto L298;
  goto ret0;

  L298:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L299;
    }
  goto ret0;

  L299:
  x4 = XEXP (x3, 1);
  if (int5_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L300;
    }
  goto ret0;

  L300:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L301;
  goto ret0;

  L301:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L302;
    }
  goto ret0;

  L302:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L303;
  goto ret0;

  L303:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L304;
  goto ret0;

  L304:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 69;
  goto ret0;

  L344:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == PLUS && 1)
    goto L345;
  goto ret0;

  L345:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L346;
    }
  goto ret0;

  L346:
  x4 = XEXP (x3, 1);
  if (int5_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L347;
    }
  goto ret0;

  L347:
  x2 = XEXP (x1, 1);
  if (reg_or_0_operand (x2, QImode))
    {
      ro[3] = x2;
      goto L348;
    }
  goto ret0;

  L348:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L349;
  goto ret0;

  L349:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L350;
    }
  goto ret0;

  L350:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L351;
  goto ret0;

  L351:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L352;
  goto ret0;

  L352:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 76;
  goto ret0;

  L332:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == QImode && GET_CODE (x2) == MEM && 1)
    goto L333;
  goto ret0;

  L333:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == PLUS && 1)
    goto L334;
  goto ret0;

  L334:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L335;
    }
  goto ret0;

  L335:
  x4 = XEXP (x3, 1);
  if (int5_operand (x4, SImode))
    {
      ro[2] = x4;
      goto L336;
    }
  goto ret0;

  L336:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L337;
  goto ret0;

  L337:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L338;
    }
  goto ret0;

  L338:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L339;
  goto ret0;

  L339:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L340;
  goto ret0;

  L340:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[2]) && 1)
    return 75;
  goto ret0;

  L907:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == IF_THEN_ELSE && 1)
    goto L908;
  goto ret0;
 L908:
  return recog_4 (x0, insn, pnum_clobbers);
 ret0: return -1;
}

int
recog_6 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != SImode)
    goto ret0;
  switch (GET_CODE (x2))
    {
    case MULT:
      goto L558;
    case DIV:
      goto L580;
    case UDIV:
      goto L602;
    case MOD:
      goto L624;
    case UMOD:
      goto L646;
    }
  goto ret0;

  L558:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L559;
  goto ret0;

  L559:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L560;
  goto ret0;

  L560:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L561;
  goto ret0;

  L561:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L562;
    }
  goto ret0;

  L562:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L563;
  goto ret0;

  L563:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 26 && 1)
    goto L564;
  goto ret0;

  L564:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L565;
  goto ret0;

  L565:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 25 && 1)
    goto L566;
  goto ret0;

  L566:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L567;
  goto ret0;

  L567:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 31 && 1)
    return 124;
  goto ret0;

  L580:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L581;
  goto ret0;

  L581:
  x3 = XEXP (x2, 1);
  if (div_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L582;
    }
  goto ret0;

  L582:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L583;
  goto ret0;

  L583:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L584;
    }
  goto ret0;

  L584:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L585;
  goto ret0;

  L585:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 26 && 1)
    goto L586;
  goto ret0;

  L586:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L587;
  goto ret0;

  L587:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 25 && 1)
    goto L588;
  goto ret0;

  L588:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L589;
  goto ret0;

  L589:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 31 && 1)
    return 126;
  goto ret0;

  L602:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L603;
  goto ret0;

  L603:
  x3 = XEXP (x2, 1);
  if (div_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L604;
    }
  goto ret0;

  L604:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L605;
  goto ret0;

  L605:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[1] = x2;
      goto L606;
    }
  goto ret0;

  L606:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L607;
  goto ret0;

  L607:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 26 && 1)
    goto L608;
  goto ret0;

  L608:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L609;
  goto ret0;

  L609:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 25 && 1)
    goto L610;
  goto ret0;

  L610:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L611;
  goto ret0;

  L611:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 31 && 1)
    return 128;
  goto ret0;

  L624:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L625;
  goto ret0;

  L625:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L626;
  goto ret0;

  L626:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L627;
  goto ret0;

  L627:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L628;
    }
  goto ret0;

  L628:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L629;
  goto ret0;

  L629:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 26 && 1)
    goto L630;
  goto ret0;

  L630:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L631;
  goto ret0;

  L631:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 25 && 1)
    goto L632;
  goto ret0;

  L632:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L633;
  goto ret0;

  L633:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 31 && 1)
    return 130;
  goto ret0;

  L646:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 26 && 1)
    goto L647;
  goto ret0;

  L647:
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == REG && XINT (x3, 0) == 25 && 1)
    goto L648;
  goto ret0;

  L648:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L649;
  goto ret0;

  L649:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L650;
    }
  goto ret0;

  L650:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L651;
  goto ret0;

  L651:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 26 && 1)
    goto L652;
  goto ret0;

  L652:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L653;
  goto ret0;

  L653:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 25 && 1)
    goto L654;
  goto ret0;

  L654:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L655;
  goto ret0;

  L655:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 31 && 1)
    return 132;
  goto ret0;
 ret0: return -1;
}

int
recog_7 (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  x1 = XVECEXP (x0, 0, 0);
  switch (GET_CODE (x1))
    {
    case CALL:
      goto L924;
    case SET:
      goto L942;
    case UNSPEC_VOLATILE:
      if (XINT (x1, 1) == 0 && XVECLEN (x1, 0) == 1 && 1)
	goto L1086;
    }
  goto ret0;

  L924:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MEM && 1)
    goto L925;
  goto ret0;

  L925:
  x3 = XEXP (x2, 0);
  if (call_operand_address (x3, SImode))
    {
      ro[0] = x3;
      goto L926;
    }
  L934:
  if (register_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L935;
    }
  goto ret0;

  L926:
  x2 = XEXP (x1, 1);
  ro[1] = x2;
  goto L927;

  L927:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L928;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  goto L934;

  L928:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 2 && 1)
    goto L929;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  goto L934;

  L929:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == USE && 1)
    goto L930;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  goto L934;

  L930:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 0 && 1)
    if (! TARGET_LONG_CALLS)
      return 192;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  goto L934;

  L935:
  x2 = XEXP (x1, 1);
  ro[1] = x2;
  goto L936;

  L936:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L937;
  goto ret0;

  L937:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 2 && 1)
    goto L938;
  goto ret0;

  L938:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == USE && 1)
    goto L939;
  goto ret0;

  L939:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 1 && 1)
    return 193;
  goto ret0;

  L942:
  x2 = XEXP (x1, 0);
  ro[0] = x2;
  goto L943;
  L992:
  if (GET_CODE (x2) == PC && 1)
    goto L993;
  goto ret0;

  L943:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CALL && 1)
    goto L944;
  x2 = XEXP (x1, 0);
  goto L992;

  L944:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && GET_CODE (x3) == MEM && 1)
    goto L945;
  x2 = XEXP (x1, 0);
  goto L992;

  L945:
  x4 = XEXP (x3, 0);
  if (call_operand_address (x4, SImode))
    {
      ro[1] = x4;
      goto L946;
    }
  L956:
  if (register_operand (x4, SImode))
    {
      ro[1] = x4;
      goto L957;
    }
  x2 = XEXP (x1, 0);
  goto L992;

  L946:
  x3 = XEXP (x2, 1);
  ro[2] = x3;
  goto L947;

  L947:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L948;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 0);
  goto L956;

  L948:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 2 && 1)
    goto L949;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 0);
  goto L956;

  L949:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == USE && 1)
    goto L950;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 0);
  goto L956;

  L950:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 0 && 1)
    if (! TARGET_LONG_CALLS)
      return 195;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 0);
  goto L956;

  L957:
  x3 = XEXP (x2, 1);
  ro[2] = x3;
  goto L958;

  L958:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L959;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L992;

  L959:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 2 && 1)
    goto L960;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L992;

  L960:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == USE && 1)
    goto L961;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L992;

  L961:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 1 && 1)
    return 196;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L992;

  L993:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == IF_THEN_ELSE && 1)
    goto L994;
  goto ret0;

  L994:
  x3 = XEXP (x2, 0);
  if (comparison_operator (x3, VOIDmode))
    {
      ro[2] = x3;
      goto L995;
    }
  L1030:
  if (eq_neq_comparison_operator (x3, VOIDmode))
    {
      ro[2] = x3;
      goto L1031;
    }
  goto ret0;

  L995:
  x4 = XEXP (x3, 0);
  if (GET_MODE (x4) == SImode && GET_CODE (x4) == PLUS && 1)
    goto L996;
  goto L1030;

  L996:
  x5 = XEXP (x4, 0);
  if (register_operand (x5, SImode))
    {
      ro[0] = x5;
      goto L997;
    }
  goto L1030;

  L997:
  x5 = XEXP (x4, 1);
  if (int5_operand (x5, SImode))
    {
      ro[1] = x5;
      goto L998;
    }
  goto L1030;

  L998:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XWINT (x4, 0) == 0 && 1)
    goto L999;
  goto L1030;

  L999:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1000;
  x3 = XEXP (x2, 0);
  goto L1030;

  L1000:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1001;

  L1001:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L1002;
  x3 = XEXP (x2, 0);
  goto L1030;

  L1002:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1003;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1003:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[0]) && 1)
    goto L1004;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1004:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L1005;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1005:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[0]) && 1)
    goto L1006;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1006:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, ro[1]) && 1)
    goto L1007;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1007:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L1008;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1008:
  x2 = XEXP (x1, 0);
  if (scratch_operand (x2, SImode))
    {
      ro[4] = x2;
      return 203;
    }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L1030;

  L1031:
  x4 = XEXP (x3, 0);
  if (register_operand (x4, SImode))
    {
      ro[0] = x4;
      goto L1032;
    }
  goto ret0;

  L1032:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && 1)
    {
      ro[5] = x4;
      goto L1033;
    }
  goto ret0;

  L1033:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1034;
  goto ret0;

  L1034:
  x4 = XEXP (x3, 0);
  ro[3] = x4;
  goto L1035;

  L1035:
  x3 = XEXP (x2, 2);
  if (GET_CODE (x3) == PC && 1)
    goto L1036;
  goto ret0;

  L1036:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1037;
  goto ret0;

  L1037:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[0]) && 1)
    goto L1038;
  goto ret0;

  L1038:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L1039;
  goto ret0;

  L1039:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, ro[0]) && 1)
    goto L1040;
  goto ret0;

  L1040:
  x3 = XEXP (x2, 1);
  if (int5_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L1041;
    }
  goto ret0;

  L1041:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L1042;
  goto ret0;

  L1042:
  x2 = XEXP (x1, 0);
  if (scratch_operand (x2, SImode))
    {
      ro[4] = x2;
      if (INTVAL (operands[5]) == - INTVAL (operands[1]))
	return 204;
      }
  goto ret0;

  L1086:
  x2 = XVECEXP (x1, 0, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 1 && 1)
    goto L1087;
  goto ret0;

  L1087:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L1088;
  goto ret0;

  L1088:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MEM && 1)
    goto L1089;
  goto ret0;

  L1089:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L1090;
    }
  goto ret0;

  L1090:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == USE && 1)
    goto L1091;
  goto ret0;

  L1091:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == MEM && 1)
    goto L1092;
  goto ret0;

  L1092:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      return 211;
    }
  goto ret0;
 ret0: return -1;
}

int
recog (x0, insn, pnum_clobbers)
     register rtx x0;
     rtx insn;
     int *pnum_clobbers;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  int tem;

  L0:
  switch (GET_CODE (x0))
    {
    case SET:
      goto L1;
    case PARALLEL:
      if (XVECLEN (x0, 0) == 2 && 1)
	goto L189;
      if (XVECLEN (x0, 0) == 7 && 1)
	goto L354;
      if (XVECLEN (x0, 0) == 5 && 1)
	goto L555;
      if (XVECLEN (x0, 0) == 3 && 1)
	goto L923;
      break;
    case RETURN:
      if (hppa_can_use_return_insn_p ())
	return 182;
      break;
    case UNSPEC_VOLATILE:
      if (XINT (x0, 1) == 0 && XVECLEN (x0, 0) == 1 && 1)
	goto L899;
      break;
    case CONST_INT:
      if (XWINT (x0, 0) == 0 && 1)
	return 197;
    }
  goto ret0;
 L1:
  return recog_3 (x0, insn, pnum_clobbers);

  L189:
  x1 = XVECEXP (x0, 0, 0);
  switch (GET_CODE (x1))
    {
    case SET:
      goto L202;
    case USE:
      goto L891;
    case UNSPEC_VOLATILE:
      if (XINT (x1, 1) == 0 && XVECLEN (x1, 0) == 1 && 1)
	goto L895;
    }
  goto ret0;
 L202:
  return recog_5 (x0, insn, pnum_clobbers);

  L891:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 2 && 1)
    goto L892;
  goto ret0;

  L892:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == RETURN && 1)
    return 183;
  goto ret0;

  L895:
  x2 = XVECEXP (x1, 0, 0);
  if (GET_CODE (x2) == CONST_INT && XWINT (x2, 0) == 0 && 1)
    goto L896;
  goto ret0;

  L896:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L897;
  goto ret0;

  L897:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[0] = x2;
      return 186;
    }
  goto ret0;

  L354:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == SET && 1)
    goto L355;
  goto ret0;

  L355:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == BLKmode && GET_CODE (x2) == MEM && 1)
    goto L356;
  goto ret0;

  L356:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[0] = x3;
      goto L357;
    }
  goto ret0;

  L357:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == BLKmode && GET_CODE (x2) == MEM && 1)
    goto L358;
  goto ret0;

  L358:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L359;
    }
  goto ret0;

  L359:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L360;
  goto ret0;

  L360:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[0]) && 1)
    goto L361;
  goto ret0;

  L361:
  x1 = XVECEXP (x0, 0, 2);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L362;
  goto ret0;

  L362:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, ro[1]) && 1)
    goto L363;
  goto ret0;

  L363:
  x1 = XVECEXP (x0, 0, 3);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L364;
  goto ret0;

  L364:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      goto L365;
    }
  goto ret0;

  L365:
  x1 = XVECEXP (x0, 0, 4);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L366;
  goto ret0;

  L366:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[3] = x2;
      goto L367;
    }
  goto ret0;

  L367:
  x1 = XVECEXP (x0, 0, 5);
  if (GET_CODE (x1) == USE && 1)
    goto L368;
  goto ret0;

  L368:
  x2 = XEXP (x1, 0);
  if (arith_operand (x2, SImode))
    {
      ro[4] = x2;
      goto L369;
    }
  goto ret0;

  L369:
  x1 = XVECEXP (x0, 0, 6);
  if (GET_CODE (x1) == USE && 1)
    goto L370;
  goto ret0;

  L370:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == CONST_INT && 1)
    {
      ro[5] = x2;
      return 78;
    }
  goto ret0;

  L555:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == SET && 1)
    goto L556;
  goto ret0;

  L556:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == REG && XINT (x2, 0) == 29 && 1)
    goto L557;
  goto ret0;
 L557:
  return recog_6 (x0, insn, pnum_clobbers);
 L923:
  return recog_7 (x0, insn, pnum_clobbers);

  L899:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == CONST_INT && XWINT (x1, 0) == 2 && 1)
    return 187;
  goto ret0;
 ret0: return -1;
}

rtx
split_insns (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx *ro = &recog_operand[0];
  register rtx x1, x2, x3, x4, x5, x6;
  rtx tem;

  L276:
  if (GET_CODE (x0) == PARALLEL && XVECLEN (x0, 0) == 2 && 1)
    goto L277;
  goto ret0;

  L277:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == SET && 1)
    goto L278;
  goto ret0;

  L278:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[0] = x2;
      goto L517;
    }
  goto ret0;

  L517:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) == SImode && GET_CODE (x2) == PLUS && 1)
    goto L518;
  L279:
  if (symbolic_operand (x2, VOIDmode))
    {
      ro[1] = x2;
      goto L280;
    }
  goto ret0;

  L518:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    {
      ro[1] = x3;
      goto L519;
    }
  goto L279;

  L519:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && 1)
    {
      ro[2] = x3;
      goto L520;
    }
  goto L279;

  L520:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L521;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L279;

  L521:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    goto L530;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L279;

  L530:
  ro[4] = x2;
  if (! cint_ok_for_move (INTVAL (operands[2])) 
   && VAL_14_BITS_P (INTVAL (operands[2]) >> 1))
    return gen_split_117 (operands);
  L531:
  ro[4] = x2;
  if (! cint_ok_for_move (INTVAL (operands[2])))
    return gen_split_118 (operands);
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L279;

  L280:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L281;
  goto ret0;

  L281:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    {
      ro[2] = x2;
      return gen_split_65 (operands);
    }
  goto ret0;
 ret0: return 0;
}

