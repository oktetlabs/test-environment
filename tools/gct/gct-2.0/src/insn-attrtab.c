/* Generated automatically by the program `genattrtab'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "regs.h"
#include "real.h"
#include "output.h"
#include "insn-attr.h"

#define operands recog_operand

int
insn_current_length (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 188:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((jump_in_call_delay (insn)) == (0)) || ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188)))
        {
	  return 4;
        }
      else
        {
	  return 8;
        }

    case 206:
    case 205:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
	    {
	      return 4;
	    }
	  else
	    {
	      return 8;
	    }
        }
      else
        {
	  if (which_alternative == 1)
	    {
	      if ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) < (insn_current_address))
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (12)))) < (8188))
		    {
		      return 12 /* 0xc */;
		    }
		  else
		    {
		      return 16 /* 0x10 */;
		    }
	        }
	      else
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
		    {
		      return 12 /* 0xc */;
		    }
		  else
		    {
		      return 16 /* 0x10 */;
		    }
	        }
	    }
	  else
	    {
	      if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
	        {
		  return 8;
	        }
	      else
	        {
		  return 12 /* 0xc */;
	        }
	    }
        }

    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
	    {
	      return 4;
	    }
	  else
	    {
	      return 8;
	    }
        }
      else
        {
	  if (which_alternative == 1)
	    {
	      if ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) < (insn_current_address))
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (24)))) < (8188))
		    {
		      return 24 /* 0x18 */;
		    }
		  else
		    {
		      return 28 /* 0x1c */;
		    }
	        }
	      else
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
		    {
		      return 24 /* 0x18 */;
		    }
		  else
		    {
		      return 28 /* 0x1c */;
		    }
	        }
	    }
	  else
	    {
	      if ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) < (insn_current_address))
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (12)))) < (8188))
		    {
		      return 12 /* 0xc */;
		    }
		  else
		    {
		      return 16 /* 0x10 */;
		    }
	        }
	      else
	        {
		  if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
		    {
		      return 12 /* 0xc */;
		    }
		  else
		    {
		      return 16 /* 0x10 */;
		    }
	        }
	    }
        }

    case 44:
    case 43:
    case 42:
    case 41:
      if ((abs ((insn_addresses[INSN_UID (JUMP_LABEL (insn))]) - ((insn_current_address) + (8)))) < (8188))
        {
	  return 4;
        }
      else
        {
	  return 8;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 0;

    }
}

int
insn_variable_length_p (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 206:
    case 205:
    case 204:
    case 203:
    case 188:
    case 44:
    case 43:
    case 42:
    case 41:
      return 1;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 0;

    }
}

int
insn_default_length (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 210:
    case 209:
    case 208:
    case 207:
    case 202:
    case 201:
    case 200:
    case 199:
    case 197:
    case 175:
    case 172:
    case 171:
    case 167:
    case 145:
    case 142:
    case 137:
    case 121:
    case 119:
    case 116:
      insn_extract (insn);
      if (arith_operand (operands[2], VOIDmode))
        {
	  return 4;
        }
      else
        {
	  return 12 /* 0xc */;
        }

    case 150:
    case 147:
    case 100:
    case 99:
    case 98:
      insn_extract (insn);
      if (arith_operand (operands[1], VOIDmode))
        {
	  return 4;
        }
      else
        {
	  return 8;
        }

    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  if (symbolic_memory_operand (operands[1], VOIDmode))
	    {
	      return 8;
	    }
	  else
	    {
	      return 4;
	    }
        }
      else
        {
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 4;
	    }
	  else
	    {
	      return 8;
	    }
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || (which_alternative == 1))
        {
	  return 8;
        }
      else if (which_alternative == 2)
        {
	  return 16 /* 0x10 */;
        }
      else if (which_alternative == 3)
        {
	  return 8;
        }
      else if ((which_alternative == 4) || (which_alternative == 5))
        {
	  return 16 /* 0x10 */;
        }
      else if ((which_alternative == 6) || (which_alternative == 7))
        {
	  return 4;
        }
      else
        {
	  return 4;
        }

    case 211:
      return 60 /* 0x3c */;

    case 206:
    case 205:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 8;
        }
      else
        {
	  if (which_alternative == 1)
	    {
	      return 16 /* 0x10 */;
	    }
	  else
	    {
	      return 12 /* 0xc */;
	    }
        }

    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 8;
        }
      else
        {
	  if (which_alternative == 1)
	    {
	      return 28 /* 0x1c */;
	    }
	  else
	    {
	      return 16 /* 0x10 */;
	    }
        }

    case 196:
    case 193:
    case 190:
    case 57:
    case 46:
      return 12 /* 0xc */;

    case 187:
      return 0;

    case 90:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 8;
        }
      else
        {
	  return 4;
        }

    case 89:
    case 60:
    case 59:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 4;
        }
      else
        {
	  return 8;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 4;
        }
      else if (which_alternative == 1)
        {
	  return 8;
        }
      else if (which_alternative == 2)
        {
	  return 4;
        }
      else if (which_alternative == 3)
        {
	  return 8;
        }
      else if (which_alternative == 4)
        {
	  return 16 /* 0x10 */;
        }
      else if (which_alternative == 5)
        {
	  return 4;
        }
      else if (which_alternative == 6)
        {
	  return 8;
        }
      else
        {
	  return 16 /* 0x10 */;
        }

    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return 4;
        }

    case 63:
      return 16 /* 0x10 */;

    case 188:
    case 186:
    case 168:
    case 149:
    case 146:
    case 144:
    case 139:
    case 136:
    case 134:
    case 120:
    case 115:
    case 105:
    case 103:
    case 87:
    case 45:
    case 30:
    case 29:
    case 28:
    case 27:
    case 26:
    case 25:
    case 23:
    case 22:
    case 21:
    case 19:
    case 18:
    case 17:
    case 16:
    case 15:
    case 44:
    case 43:
    case 42:
    case 41:
      return 8;

    case 24:
    case 20:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 8;
        }
      else
        {
	  return 12 /* 0xc */;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 4;

    }
}

int
result_ready_cost (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 164:
      return 14 /* 0xe */;

    case 163:
      return 18 /* 0x12 */;

    case 158:
      return 10 /* 0xa */;

    case 157:
      return 12 /* 0xc */;

    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 1) && ((which_alternative != 2) && (which_alternative != 3)))
        {
	  return 3;
        }
      else if ((which_alternative == 3) || (which_alternative == 2))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 6) || (((which_alternative == 1) || (which_alternative == 2)) || (which_alternative == 8)))
        {
	  return 3;
        }
      else if (((which_alternative == 3) || (which_alternative == 4)) || (which_alternative == 7))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || (((which_alternative == 3) || (which_alternative == 4)) || (which_alternative == 2)))
        {
	  return 3;
        }
      else if (which_alternative != 1)
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 97:
    case 96:
    case 95:
    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && (which_alternative != 3)))) && ((which_alternative != 4) && (which_alternative != 6)))
        {
	  return 3;
        }
      else if (which_alternative == 4)
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 162:
    case 161:
    case 160:
    case 159:
    case 156:
    case 155:
    case 154:
    case 153:
    case 152:
    case 151:
    case 123:
    case 114:
    case 113:
    case 112:
    case 111:
    case 110:
    case 109:
    case 106:
    case 105:
    case 104:
    case 103:
    case 102:
    case 101:
    case 94:
    case 83:
    case 76:
    case 70:
    case 56:
    case 54:
      return 3;

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 7) || ((which_alternative == 5) || (which_alternative == 9)))
        {
	  return 3;
        }
      else if ((which_alternative == 4) || (which_alternative == 8))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 166:
    case 165:
    case 93:
    case 82:
    case 75:
    case 69:
    case 68:
    case 55:
    case 53:
    case 52:
    case 50:
      return 2;

    case 4:
    case 3:
      return 4;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 1;

    }
}

int
fp_mpy_unit_ready_cost (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 164:
      return 14 /* 0xe */;

    case 158:
      return 10 /* 0xa */;

    case 157:
      return 12 /* 0xc */;

    case 156:
    case 155:
    case 123:
      return 3;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 18 /* 0x12 */;

    }
}

unsigned int
fp_mpy_unit_blockage_range (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 158:
      return 524306 /* 0x80012 */;

    case 156:
    case 155:
    case 123:
      return 131090 /* 0x20012 */;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 655378 /* 0xa0012 */;

    }
}

int
fp_alu_unit_ready_cost (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 162:
    case 161:
    case 160:
    case 159:
    case 154:
    case 153:
    case 152:
    case 151:
    case 114:
    case 113:
    case 112:
    case 111:
    case 110:
    case 109:
    case 106:
    case 105:
    case 104:
    case 103:
    case 102:
    case 101:
      return 3;

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 6)
        {
	  return 3;
        }
      else
        {
	  return 4;
        }

    case 92:
    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 3;
        }
      else
        {
	  return 4;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && (which_alternative != 6))))))
        {
	  return 3;
        }
      else
        {
	  return 4;
        }

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 7)
        {
	  return 3;
        }
      else
        {
	  return 4;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 4;

    }
}

int
memory_unit_ready_cost (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 2) || (which_alternative == 3))
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 7) || ((which_alternative == 3) || (which_alternative == 4)))
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && (which_alternative != 4)))))
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 97:
    case 96:
    case 95:
    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 8) || (which_alternative == 4))
        {
	  return 2;
        }
      else
        {
	  return 3;
        }

    case 166:
    case 165:
    case 93:
    case 82:
    case 75:
    case 69:
    case 68:
    case 55:
    case 53:
    case 52:
    case 50:
      return 2;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 3;

    }
}

unsigned int
memory_unit_blockage_range (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 2) || (which_alternative == 3))
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 7) || ((which_alternative == 3) || (which_alternative == 4)))
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && (which_alternative != 4)))))
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 97:
    case 96:
    case 95:
    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 8) || (which_alternative == 4))
        {
	  return 65539 /* 0x10003 */;
        }
      else
        {
	  return 131075 /* 0x20003 */;
        }

    case 166:
    case 165:
    case 93:
    case 82:
    case 75:
    case 69:
    case 68:
    case 55:
    case 53:
    case 52:
    case 50:
      return 65539 /* 0x10003 */;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 131075 /* 0x20003 */;

    }
}

int
function_units_used (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 164:
    case 163:
    case 158:
    case 157:
    case 156:
    case 155:
    case 123:
      return 2;

    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && (which_alternative != 1))
        {
	  return 0;
        }
      else if (which_alternative == 0)
        {
	  return 1;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 7) || ((which_alternative == 8) || ((which_alternative == 1) || ((which_alternative == 2) || ((which_alternative == 3) || (which_alternative == 4))))))
        {
	  return 0;
        }
      else if (which_alternative == 6)
        {
	  return 1;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && (which_alternative != 1))
        {
	  return 0;
        }
      else if (which_alternative == 0)
        {
	  return 1;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 97:
    case 96:
    case 95:
    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return 0;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 5) || (which_alternative == 4))
        {
	  return 0;
        }
      else if (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && (which_alternative != 6))))
        {
	  return 1;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 8) || ((which_alternative == 9) || ((which_alternative == 5) || (which_alternative == 4))))
        {
	  return 0;
        }
      else if (which_alternative == 7)
        {
	  return 1;
        }
      else
        {
	  return -1 /* 0xffffffff */;
        }

    case 166:
    case 165:
    case 94:
    case 93:
    case 83:
    case 82:
    case 76:
    case 75:
    case 70:
    case 69:
    case 68:
    case 56:
    case 55:
    case 54:
    case 53:
    case 52:
    case 50:
      return 0;

    case 162:
    case 161:
    case 160:
    case 159:
    case 154:
    case 153:
    case 152:
    case 151:
    case 114:
    case 113:
    case 112:
    case 111:
    case 110:
    case 109:
    case 106:
    case 105:
    case 104:
    case 103:
    case 102:
    case 101:
    case 4:
    case 3:
      return 1;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return -1 /* 0xffffffff */;

    }
}

int
num_delay_slots (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 206:
    case 205:
    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case 198:
    case 195:
    case 192:
    case 188:
    case 183:
    case 182:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
    case 46:
    case 45:
    case 44:
    case 43:
    case 42:
    case 41:
      return 1;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 0;

    }
}

enum attr_in_call_delay
get_attr_in_call_delay (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 188:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((TARGET_JUMP_IN_DELAY) != (0))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 210:
    case 209:
    case 208:
    case 207:
    case 202:
    case 201:
    case 200:
    case 199:
    case 197:
    case 175:
    case 172:
    case 171:
    case 167:
    case 145:
    case 142:
    case 137:
    case 121:
    case 119:
    case 116:
      insn_extract (insn);
      if (arith_operand (operands[2], VOIDmode))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 150:
    case 147:
    case 100:
    case 99:
    case 98:
      insn_extract (insn);
      if (arith_operand (operands[1], VOIDmode))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case 89:
    case 60:
    case 59:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return IN_CALL_DELAY_TRUE;
        }
      else
        {
	  return IN_CALL_DELAY_FALSE;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    case 211:
    case 206:
    case 205:
    case 204:
    case 203:
    case 198:
    case 196:
    case 195:
    case 193:
    case 192:
    case 190:
    case 187:
    case 186:
    case 183:
    case 182:
    case 168:
    case 149:
    case 146:
    case 144:
    case 139:
    case 136:
    case 134:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
    case 120:
    case 115:
    case 105:
    case 103:
    case 87:
    case 78:
    case 63:
    case 57:
    case 46:
    case 45:
    case 44:
    case 43:
    case 42:
    case 41:
    case 30:
    case 29:
    case 28:
    case 27:
    case 26:
    case 25:
    case 24:
    case 23:
    case 22:
    case 21:
    case 20:
    case 19:
    case 18:
    case 17:
    case 16:
    case 15:
      return IN_CALL_DELAY_FALSE;

    default:
      return IN_CALL_DELAY_TRUE;

    }
}

enum attr_in_nullified_branch_delay
get_attr_in_nullified_branch_delay (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 210:
    case 209:
    case 208:
    case 207:
    case 202:
    case 201:
    case 200:
    case 199:
    case 197:
    case 175:
    case 172:
    case 171:
    case 167:
    case 145:
    case 142:
    case 137:
    case 121:
    case 119:
    case 116:
      insn_extract (insn);
      if (arith_operand (operands[2], VOIDmode))
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 150:
    case 147:
    case 100:
    case 99:
    case 98:
      insn_extract (insn);
      if (arith_operand (operands[1], VOIDmode))
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 6) && (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5))))))
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 2) || (which_alternative == 5))
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 92:
    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 89:
    case 60:
    case 59:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 74:
    case 67:
    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 7)
        {
	  return IN_NULLIFIED_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_NULLIFIED_BRANCH_DELAY_FALSE;
        }

    case 181:
    case 180:
    case 179:
    case 178:
    case 177:
    case 176:
    case 174:
    case 170:
    case 166:
    case 165:
    case 141:
    case 135:
    case 94:
    case 93:
    case 83:
    case 82:
    case 76:
    case 75:
    case 72:
    case 71:
    case 70:
    case 69:
    case 68:
    case 64:
    case 62:
    case 61:
    case 58:
    case 56:
    case 55:
    case 54:
    case 53:
    case 52:
    case 50:
      return IN_NULLIFIED_BRANCH_DELAY_TRUE;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return IN_NULLIFIED_BRANCH_DELAY_FALSE;

    }
}

enum attr_in_branch_delay
get_attr_in_branch_delay (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 210:
    case 209:
    case 208:
    case 207:
    case 202:
    case 201:
    case 200:
    case 199:
    case 197:
    case 175:
    case 172:
    case 171:
    case 167:
    case 145:
    case 142:
    case 137:
    case 121:
    case 119:
    case 116:
      insn_extract (insn);
      if (arith_operand (operands[2], VOIDmode))
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 150:
    case 147:
    case 100:
    case 99:
    case 98:
      insn_extract (insn);
      if (arith_operand (operands[1], VOIDmode))
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 90:
    case 79:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative != 0)
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case 89:
    case 60:
    case 59:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return IN_BRANCH_DELAY_TRUE;
        }
      else
        {
	  return IN_BRANCH_DELAY_FALSE;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    case 211:
    case 206:
    case 205:
    case 204:
    case 203:
    case 198:
    case 196:
    case 195:
    case 193:
    case 192:
    case 190:
    case 188:
    case 187:
    case 186:
    case 183:
    case 182:
    case 168:
    case 149:
    case 146:
    case 144:
    case 139:
    case 136:
    case 134:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
    case 120:
    case 115:
    case 105:
    case 103:
    case 87:
    case 78:
    case 63:
    case 57:
    case 46:
    case 45:
    case 44:
    case 43:
    case 42:
    case 41:
    case 30:
    case 29:
    case 28:
    case 27:
    case 26:
    case 25:
    case 24:
    case 23:
    case 22:
    case 21:
    case 20:
    case 19:
    case 18:
    case 17:
    case 16:
    case 15:
      return IN_BRANCH_DELAY_FALSE;

    default:
      return IN_BRANCH_DELAY_TRUE;

    }
}

enum attr_type
get_attr_type (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_MOVE;
        }
      else if ((which_alternative == 1) || (which_alternative == 2))
        {
	  return TYPE_STORE;
        }
      else if ((which_alternative == 3) || (which_alternative == 4))
        {
	  return TYPE_LOAD;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_MISC;
        }
      else if (which_alternative == 6)
        {
	  return TYPE_FPALU;
        }
      else if (which_alternative == 7)
        {
	  return TYPE_FPLOAD;
        }
      else
        {
	  return TYPE_FPSTORE;
        }

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_FPALU;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 2)
        {
	  return TYPE_FPSTORE;
        }
      else if ((which_alternative == 3) || (which_alternative == 4))
        {
	  return TYPE_STORE;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_FPLOAD;
        }
      else
        {
	  return TYPE_LOAD;
        }

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || ((which_alternative == 1) || ((which_alternative == 2) || (which_alternative == 3))))
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 4)
        {
	  return TYPE_LOAD;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_STORE;
        }
      else if (which_alternative == 6)
        {
	  return TYPE_MOVE;
        }
      else
        {
	  return TYPE_FPALU;
        }

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 0) || ((which_alternative == 1) || ((which_alternative == 2) || (which_alternative == 3))))
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 4)
        {
	  return TYPE_LOAD;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_STORE;
        }
      else if (which_alternative == 6)
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 7)
        {
	  return TYPE_FPALU;
        }
      else if (which_alternative == 8)
        {
	  return TYPE_FPLOAD;
        }
      else
        {
	  return TYPE_FPSTORE;
        }

    case 79:
    case 90:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_MOVE;
        }
      else
        {
	  return TYPE_FPLOAD;
        }

    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_FPALU;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 2)
        {
	  return TYPE_FPLOAD;
        }
      else if (which_alternative == 3)
        {
	  return TYPE_LOAD;
        }
      else if (which_alternative == 4)
        {
	  return TYPE_FPSTORE;
        }
      else
        {
	  return TYPE_STORE;
        }

    case 95:
    case 96:
    case 97:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_UNARY;
        }
      else
        {
	  return TYPE_LOAD;
        }

    case 203:
    case 204:
    case 205:
    case 206:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  return TYPE_CBRANCH;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_MULTI;
        }
      else
        {
	  return TYPE_MULTI;
        }

    case 124:
    case 126:
    case 128:
    case 130:
    case 132:
      return TYPE_MILLI;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 57:
    case 63:
    case 78:
    case 168:
      return TYPE_MULTI;

    case 163:
      return TYPE_FPSQRTDBL;

    case 164:
      return TYPE_FPSQRTSGL;

    case 157:
      return TYPE_FPDIVDBL;

    case 158:
      return TYPE_FPDIVSGL;

    case 123:
    case 155:
    case 156:
      return TYPE_FPMUL;

    case 3:
    case 4:
      return TYPE_FPCC;

    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 151:
    case 152:
    case 153:
    case 154:
    case 159:
    case 160:
    case 161:
    case 162:
      return TYPE_FPALU;

    case 83:
    case 94:
      return TYPE_FPSTORE;

    case 82:
    case 93:
      return TYPE_FPLOAD;

    case 193:
    case 196:
      return TYPE_DYNCALL;

    case 192:
    case 195:
      return TYPE_CALL;

    case 45:
    case 46:
      return TYPE_FBRANCH;

    case 41:
    case 42:
    case 43:
    case 44:
      return TYPE_CBRANCH;

    case 182:
    case 183:
    case 198:
      return TYPE_BRANCH;

    case 188:
      return TYPE_UNCOND_BRANCH;

    case 54:
    case 56:
    case 70:
    case 76:
      return TYPE_STORE;

    case 50:
    case 52:
    case 53:
    case 55:
    case 68:
    case 69:
    case 75:
    case 165:
    case 166:
      return TYPE_LOAD;

    case 98:
    case 99:
    case 100:
    case 146:
    case 147:
    case 149:
    case 150:
      return TYPE_UNARY;

    case 61:
    case 62:
    case 71:
    case 87:
      return TYPE_MOVE;

    default:
      return TYPE_BINARY;

    }
}

int
eligible_for_delay (delay_insn, slot, candidate_insn, flags)
     rtx delay_insn;
     int slot;
     rtx candidate_insn;
     int flags;
{
  rtx insn;

  if (slot >= 1)
    abort ();

  insn = delay_insn;
  switch (recog_memoized (insn))
    {
    case 206:
    case 205:
    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  slot += 4 * 1;
      break;
        }
      else
        {
	  slot += 0 * 1;
      break;
        }
      break;

    case 198:
    case 188:
    case 183:
    case 182:
      slot += 2 * 1;
      break;
      break;

    case 195:
    case 192:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
      slot += 1 * 1;
      break;
      break;

    case 46:
    case 45:
      slot += 3 * 1;
      break;
      break;

    case 44:
    case 43:
    case 42:
    case 41:
      slot += 4 * 1;
      break;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      slot += 0 * 1;
      break;
      break;

    }

  if (slot < 1)
    abort ();

  insn = candidate_insn;
  switch (slot)
    {
    case 4:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if (arith_operand (operands[2], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        case 211:
        case 206:
        case 205:
        case 204:
        case 203:
        case 198:
        case 196:
        case 195:
        case 193:
        case 192:
        case 190:
        case 188:
        case 187:
        case 186:
        case 183:
        case 182:
        case 168:
        case 149:
        case 146:
        case 144:
        case 139:
        case 136:
        case 134:
        case 132:
        case 130:
        case 128:
        case 126:
        case 124:
        case 120:
        case 115:
        case 105:
        case 103:
        case 87:
        case 78:
        case 63:
        case 57:
        case 46:
        case 45:
        case 44:
        case 43:
        case 42:
        case 41:
        case 30:
        case 29:
        case 28:
        case 27:
        case 26:
        case 25:
        case 24:
        case 23:
        case 22:
        case 21:
        case 20:
        case 19:
        case 18:
        case 17:
        case 16:
        case 15:
	  return 0;

        default:
	  return 1;

      }
    case 3:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if (arith_operand (operands[2], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        case 211:
        case 206:
        case 205:
        case 204:
        case 203:
        case 198:
        case 196:
        case 195:
        case 193:
        case 192:
        case 190:
        case 188:
        case 187:
        case 186:
        case 183:
        case 182:
        case 168:
        case 149:
        case 146:
        case 144:
        case 139:
        case 136:
        case 134:
        case 132:
        case 130:
        case 128:
        case 126:
        case 124:
        case 120:
        case 115:
        case 105:
        case 103:
        case 87:
        case 78:
        case 63:
        case 57:
        case 46:
        case 45:
        case 44:
        case 43:
        case 42:
        case 41:
        case 30:
        case 29:
        case 28:
        case 27:
        case 26:
        case 25:
        case 24:
        case 23:
        case 22:
        case 21:
        case 20:
        case 19:
        case 18:
        case 17:
        case 16:
        case 15:
	  return 0;

        default:
	  return 1;

      }
    case 2:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if (arith_operand (operands[2], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        case 211:
        case 206:
        case 205:
        case 204:
        case 203:
        case 198:
        case 196:
        case 195:
        case 193:
        case 192:
        case 190:
        case 188:
        case 187:
        case 186:
        case 183:
        case 182:
        case 168:
        case 149:
        case 146:
        case 144:
        case 139:
        case 136:
        case 134:
        case 132:
        case 130:
        case 128:
        case 126:
        case 124:
        case 120:
        case 115:
        case 105:
        case 103:
        case 87:
        case 78:
        case 63:
        case 57:
        case 46:
        case 45:
        case 44:
        case 43:
        case 42:
        case 41:
        case 30:
        case 29:
        case 28:
        case 27:
        case 26:
        case 25:
        case 24:
        case 23:
        case 22:
        case 21:
        case 20:
        case 19:
        case 18:
        case 17:
        case 16:
        case 15:
	  return 0;

        default:
	  return 1;

      }
    case 1:
      switch (recog_memoized (insn))
	{
        case 188:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((TARGET_JUMP_IN_DELAY) != (0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if (arith_operand (operands[2], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 0) && (which_alternative != 1)) && (((which_alternative == 6) || (which_alternative == 7)) || ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && ((which_alternative != 5) && ((which_alternative != 6) && (which_alternative != 7))))))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) || ((which_alternative == 2) || (which_alternative == 5)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        case 211:
        case 206:
        case 205:
        case 204:
        case 203:
        case 198:
        case 196:
        case 195:
        case 193:
        case 192:
        case 190:
        case 187:
        case 186:
        case 183:
        case 182:
        case 168:
        case 149:
        case 146:
        case 144:
        case 139:
        case 136:
        case 134:
        case 132:
        case 130:
        case 128:
        case 126:
        case 124:
        case 120:
        case 115:
        case 105:
        case 103:
        case 87:
        case 78:
        case 63:
        case 57:
        case 46:
        case 45:
        case 44:
        case 43:
        case 42:
        case 41:
        case 30:
        case 29:
        case 28:
        case 27:
        case 26:
        case 25:
        case 24:
        case 23:
        case 22:
        case 21:
        case 20:
        case 19:
        case 18:
        case 17:
        case 16:
        case 15:
	  return 0;

        default:
	  return 1;

      }
    default:
      abort ();
    }
}

int
eligible_for_annul_true (delay_insn, slot, candidate_insn, flags)
     rtx delay_insn;
     int slot;
     rtx candidate_insn;
     int flags;
{
  rtx insn;

  if (slot >= 1)
    abort ();

  insn = delay_insn;
  switch (recog_memoized (insn))
    {
    case 206:
    case 205:
    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  slot += 4 * 1;
      break;
        }
      else
        {
	  slot += 0 * 1;
      break;
        }
      break;

    case 198:
    case 188:
    case 183:
    case 182:
      slot += 2 * 1;
      break;
      break;

    case 195:
    case 192:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
      slot += 1 * 1;
      break;
      break;

    case 46:
    case 45:
      slot += 3 * 1;
      break;
      break;

    case 44:
    case 43:
    case 42:
    case 41:
      slot += 4 * 1;
      break;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      slot += 0 * 1;
      break;
      break;

    }

  if (slot < 1)
    abort ();

  insn = candidate_insn;
  switch (slot)
    {
    case 4:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if ((arith_operand (operands[2], VOIDmode)) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if ((arith_operand (operands[1], VOIDmode)) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode)))) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 92:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 0) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 6) && (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5)))))) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 2) || (which_alternative == 5)) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 1) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 74:
        case 67:
        case 51:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 7) && ((flags & ATTR_FLAG_forward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 181:
        case 180:
        case 179:
        case 178:
        case 177:
        case 176:
        case 174:
        case 170:
        case 166:
        case 165:
        case 141:
        case 135:
        case 94:
        case 93:
        case 83:
        case 82:
        case 76:
        case 75:
        case 72:
        case 71:
        case 70:
        case 69:
        case 68:
        case 64:
        case 62:
        case 61:
        case 58:
        case 56:
        case 55:
        case 54:
        case 53:
        case 52:
        case 50:
	  if ((flags & ATTR_FLAG_forward) != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 3:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if (arith_operand (operands[2], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if (arith_operand (operands[1], VOIDmode))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 6) && (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5))))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 2) || (which_alternative == 5))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 92:
        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 74:
        case 67:
        case 51:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative != 7)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 181:
        case 180:
        case 179:
        case 178:
        case 177:
        case 176:
        case 174:
        case 170:
        case 166:
        case 165:
        case 141:
        case 135:
        case 94:
        case 93:
        case 83:
        case 82:
        case 76:
        case 75:
        case 72:
        case 71:
        case 70:
        case 69:
        case 68:
        case 64:
        case 62:
        case 61:
        case 58:
        case 56:
        case 55:
        case 54:
        case 53:
        case 52:
        case 50:
	  return 1;

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 2:
      switch (recog_memoized (insn))
	{
        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 1:
      switch (recog_memoized (insn))
	{
        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    default:
      abort ();
    }
}

int
eligible_for_annul_false (delay_insn, slot, candidate_insn, flags)
     rtx delay_insn;
     int slot;
     rtx candidate_insn;
     int flags;
{
  rtx insn;

  if (slot >= 1)
    abort ();

  insn = delay_insn;
  switch (recog_memoized (insn))
    {
    case 206:
    case 205:
    case 204:
    case 203:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 0)
        {
	  slot += 4 * 1;
      break;
        }
      else
        {
	  slot += 0 * 1;
      break;
        }
      break;

    case 198:
    case 188:
    case 183:
    case 182:
      slot += 2 * 1;
      break;
      break;

    case 195:
    case 192:
    case 132:
    case 130:
    case 128:
    case 126:
    case 124:
      slot += 1 * 1;
      break;
      break;

    case 46:
    case 45:
      slot += 3 * 1;
      break;
      break;

    case 44:
    case 43:
    case 42:
    case 41:
      slot += 4 * 1;
      break;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      slot += 0 * 1;
      break;
      break;

    }

  if (slot < 1)
    abort ();

  insn = candidate_insn;
  switch (slot)
    {
    case 4:
      switch (recog_memoized (insn))
	{
        case 210:
        case 209:
        case 208:
        case 207:
        case 202:
        case 201:
        case 200:
        case 199:
        case 197:
        case 175:
        case 172:
        case 171:
        case 167:
        case 145:
        case 142:
        case 137:
        case 121:
        case 119:
        case 116:
	  insn_extract (insn);
	  if ((arith_operand (operands[2], VOIDmode)) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 150:
        case 147:
        case 100:
        case 99:
        case 98:
	  insn_extract (insn);
	  if ((arith_operand (operands[1], VOIDmode)) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 97:
        case 96:
        case 95:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((((which_alternative == 1) && (! (symbolic_memory_operand (operands[1], VOIDmode)))) || ((which_alternative == 0) && (arith_operand (operands[1], VOIDmode)))) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 92:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 0) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative != 6) && (((which_alternative != 0) && (which_alternative != 1)) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5)))))) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (((which_alternative == 2) || (which_alternative == 5)) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 1) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 89:
        case 60:
        case 59:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 0) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 74:
        case 67:
        case 51:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 7) && ((flags & ATTR_FLAG_backward) != 0))
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case 181:
        case 180:
        case 179:
        case 178:
        case 177:
        case 176:
        case 174:
        case 170:
        case 166:
        case 165:
        case 141:
        case 135:
        case 94:
        case 93:
        case 83:
        case 82:
        case 76:
        case 75:
        case 72:
        case 71:
        case 70:
        case 69:
        case 68:
        case 64:
        case 62:
        case 61:
        case 58:
        case 56:
        case 55:
        case 54:
        case 53:
        case 52:
        case 50:
	  if ((flags & ATTR_FLAG_backward) != 0)
	    {
	      return 1;
	    }
	  else
	    {
	      return 0;
	    }

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 3:
      switch (recog_memoized (insn))
	{
        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 2:
      switch (recog_memoized (insn))
	{
        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    case 1:
      switch (recog_memoized (insn))
	{
        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 0;

      }
    default:
      abort ();
    }
}

static int
fp_mpy_unit_blockage (executing_insn, candidate_insn)
     rtx executing_insn;
     rtx candidate_insn;
{
  rtx insn;
  int casenum;

  insn = candidate_insn;
  switch (recog_memoized (insn))
    {
    case 164:
      casenum = 3;
      break;

    case 158:
      casenum = 1;
      break;

    case 157:
      casenum = 2;
      break;

    case 156:
    case 155:
    case 123:
      casenum = 0;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      casenum = 4;
      break;

    }

  insn = executing_insn;
  switch (casenum)
    {
    case 0:
      switch (recog_memoized (insn))
	{
        case 164:
	  return 12 /* 0xc */;

        case 158:
	  return 8;

        case 157:
	  return 10 /* 0xa */;

        case 156:
        case 155:
        case 123:
	  return 2;

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 16 /* 0x10 */;

      }

    case 1:
      return 10 /* 0xa */;

    case 2:
      return 12 /* 0xc */;

    case 3:
      return 14 /* 0xe */;

    case 4:
      return 18 /* 0x12 */;

    }
}

static int
fp_mpy_unit_conflict_cost (executing_insn, candidate_insn)
     rtx executing_insn;
     rtx candidate_insn;
{
  rtx insn;
  int casenum;

  insn = candidate_insn;
  switch (recog_memoized (insn))
    {
    case 164:
      casenum = 3;
      break;

    case 158:
      casenum = 1;
      break;

    case 157:
      casenum = 2;
      break;

    case 156:
    case 155:
    case 123:
      casenum = 0;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      casenum = 4;
      break;

    }

  insn = executing_insn;
  switch (casenum)
    {
    case 0:
      return 2;

    case 1:
      return 10 /* 0xa */;

    case 2:
      return 12 /* 0xc */;

    case 3:
      return 14 /* 0xe */;

    case 4:
      return 18 /* 0x12 */;

    }
}

static int
memory_unit_blockage (executing_insn, candidate_insn)
     rtx executing_insn;
     rtx candidate_insn;
{
  rtx insn;
  int casenum;

  insn = candidate_insn;
  switch (recog_memoized (insn))
    {
    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      casenum = 0;
      break;

    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 3)
        {
	  casenum = 0;
        }
      else if (which_alternative != 2)
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 3) || (which_alternative == 4))
        {
	  casenum = 0;
        }
      else if (((which_alternative == 1) || (which_alternative == 2)) || (which_alternative == 8))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5))))))
        {
	  casenum = 0;
        }
      else if (((which_alternative == 3) || (which_alternative == 4)) || (which_alternative == 2))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  casenum = 0;
        }
      else
        {
	  casenum = 1;
        }
      break;

    case 94:
    case 83:
    case 76:
    case 70:
    case 56:
    case 54:
      casenum = 1;
      break;

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  casenum = 0;
        }
      else if ((which_alternative == 5) || (which_alternative == 9))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 166:
    case 165:
    case 75:
    case 69:
    case 68:
    case 55:
    case 53:
    case 52:
    case 50:
      casenum = 0;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      casenum = 2;
      break;

    }

  insn = executing_insn;
  switch (casenum)
    {
    case 0:
      switch (recog_memoized (insn))
	{
        case 92:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 2) || (which_alternative == 3))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 7) || ((which_alternative == 3) || (which_alternative == 4)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && (which_alternative != 4)))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 97:
        case 96:
        case 95:
        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  return 1;

        case 74:
        case 67:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 4)
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 51:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 8) || (which_alternative == 4))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 166:
        case 165:
        case 93:
        case 82:
        case 75:
        case 69:
        case 68:
        case 55:
        case 53:
        case 52:
        case 50:
	  return 1;

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 2;

      }

    case 1:
      return 3;

    case 2:
      switch (recog_memoized (insn))
	{
        case 92:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 2) || (which_alternative == 3))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 88:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 7) || ((which_alternative == 3) || (which_alternative == 4)))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 81:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && (which_alternative != 4)))))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 97:
        case 96:
        case 95:
        case 90:
        case 79:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  return 1;

        case 74:
        case 67:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if (which_alternative == 4)
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 51:
	  insn_extract (insn);
	  if (! constrain_operands (INSN_CODE (insn), reload_completed))
	    fatal_insn_not_found (insn);
	  if ((which_alternative == 8) || (which_alternative == 4))
	    {
	      return 1;
	    }
	  else
	    {
	      return 2;
	    }

        case 166:
        case 165:
        case 93:
        case 82:
        case 75:
        case 69:
        case 68:
        case 55:
        case 53:
        case 52:
        case 50:
	  return 1;

        case -1:
	  if (GET_CODE (PATTERN (insn)) != ASM_INPUT
	      && asm_noperands (PATTERN (insn)) < 0)
	    fatal_insn_not_found (insn);
        default:
	  return 2;

      }

    }
}

static int
memory_unit_conflict_cost (executing_insn, candidate_insn)
     rtx executing_insn;
     rtx candidate_insn;
{
  rtx insn;
  int casenum;

  insn = candidate_insn;
  switch (recog_memoized (insn))
    {
    case 97:
    case 96:
    case 95:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      casenum = 0;
      break;

    case 92:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 3)
        {
	  casenum = 0;
        }
      else if (which_alternative != 2)
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 88:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative == 3) || (which_alternative == 4))
        {
	  casenum = 0;
        }
      else if (((which_alternative == 1) || (which_alternative == 2)) || (which_alternative == 8))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 81:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if ((which_alternative != 0) && ((which_alternative != 1) && ((which_alternative != 2) && ((which_alternative != 3) && ((which_alternative != 4) && (which_alternative != 5))))))
        {
	  casenum = 0;
        }
      else if (((which_alternative == 3) || (which_alternative == 4)) || (which_alternative == 2))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 74:
    case 67:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  casenum = 0;
        }
      else
        {
	  casenum = 1;
        }
      break;

    case 94:
    case 83:
    case 76:
    case 70:
    case 56:
    case 54:
      casenum = 1;
      break;

    case 51:
      insn_extract (insn);
      if (! constrain_operands (INSN_CODE (insn), reload_completed))
        fatal_insn_not_found (insn);
      if (which_alternative == 4)
        {
	  casenum = 0;
        }
      else if ((which_alternative == 5) || (which_alternative == 9))
        {
	  casenum = 1;
        }
      else
        {
	  casenum = 2;
        }
      break;

    case 166:
    case 165:
    case 75:
    case 69:
    case 68:
    case 55:
    case 53:
    case 52:
    case 50:
      casenum = 0;
      break;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      casenum = 2;
      break;

    }

  insn = executing_insn;
  switch (casenum)
    {
    case 0:
      return 1;

    case 1:
      return 3;

    case 2:
      return 1;

    }
}

struct function_unit_desc function_units[] = {
  {"memory", 1, 1, 0, 0, 3, memory_unit_ready_cost, memory_unit_conflict_cost, 3, memory_unit_blockage_range, memory_unit_blockage}, 
  {"fp_alu", 2, 1, 0, 2, 2, fp_alu_unit_ready_cost, 0, 2, 0, 0}, 
  {"fp_mpy", 4, 1, 0, 0, 18, fp_mpy_unit_ready_cost, fp_mpy_unit_conflict_cost, 18, fp_mpy_unit_blockage_range, fp_mpy_unit_blockage}, 
};

int
const_num_delay_slots (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    default:
      return 1;
    }
}
