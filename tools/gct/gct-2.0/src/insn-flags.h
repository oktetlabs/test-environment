/* Generated automatically by the program `genflags'
from the machine description file `md'.  */

#define HAVE_cmpsi 1
#define HAVE_cmpsf 1
#define HAVE_cmpdf 1
#define HAVE_seq 1
#define HAVE_sne 1
#define HAVE_slt 1
#define HAVE_sgt 1
#define HAVE_sle 1
#define HAVE_sge 1
#define HAVE_sltu 1
#define HAVE_sgtu 1
#define HAVE_sleu 1
#define HAVE_sgeu 1
#define HAVE_scc 1
#define HAVE_negscc 1
#define HAVE_incscc 1
#define HAVE_decscc 1
#define HAVE_sminsi3 1
#define HAVE_uminsi3 1
#define HAVE_smaxsi3 1
#define HAVE_umaxsi3 1
#define HAVE_beq 1
#define HAVE_bne 1
#define HAVE_bgt 1
#define HAVE_blt 1
#define HAVE_bge 1
#define HAVE_ble 1
#define HAVE_bgtu 1
#define HAVE_bltu 1
#define HAVE_bgeu 1
#define HAVE_bleu 1
#define HAVE_movsi 1
#define HAVE_reload_insi 1
#define HAVE_reload_outsi 1
#define HAVE_pre_ldwm 1
#define HAVE_pre_stwm 1
#define HAVE_post_ldwm 1
#define HAVE_post_stwm 1
#define HAVE_add_high_const (reload_completed)
#define HAVE_movhi 1
#define HAVE_movqi 1
#define HAVE_movstrsi 1
#define HAVE_movdf 1
#define HAVE_movdi 1
#define HAVE_reload_indi 1
#define HAVE_reload_outdi 1
#define HAVE_movsf 1
#define HAVE_zero_extendhisi2 1
#define HAVE_zero_extendqihi2 1
#define HAVE_zero_extendqisi2 1
#define HAVE_extendhisi2 1
#define HAVE_extendqihi2 1
#define HAVE_extendqisi2 1
#define HAVE_extendsfdf2 1
#define HAVE_truncdfsf2 1
#define HAVE_floatsisf2 1
#define HAVE_floatsidf2 1
#define HAVE_floatunssisf2 (TARGET_SNAKE)
#define HAVE_floatunssidf2 (TARGET_SNAKE)
#define HAVE_floatdisf2 (TARGET_SNAKE)
#define HAVE_floatdidf2 (TARGET_SNAKE)
#define HAVE_fix_truncsfsi2 1
#define HAVE_fix_truncdfsi2 1
#define HAVE_fix_truncsfdi2 (TARGET_SNAKE)
#define HAVE_fix_truncdfdi2 (TARGET_SNAKE)
#define HAVE_adddi3 1
#define HAVE_addsi3 1
#define HAVE_subdi3 1
#define HAVE_subsi3 1
#define HAVE_mulsi3 1
#define HAVE_umulsidi3 (TARGET_SNAKE && ! TARGET_DISABLE_FPREGS)
#define HAVE_divsi3 1
#define HAVE_udivsi3 1
#define HAVE_modsi3 1
#define HAVE_umodsi3 1
#define HAVE_anddi3 1
#define HAVE_andsi3 1
#define HAVE_iordi3 1
#define HAVE_iorsi3 1
#define HAVE_xordi3 1
#define HAVE_xorsi3 1
#define HAVE_negdi2 1
#define HAVE_negsi2 1
#define HAVE_one_cmpldi2 1
#define HAVE_one_cmplsi2 1
#define HAVE_adddf3 1
#define HAVE_addsf3 1
#define HAVE_subdf3 1
#define HAVE_subsf3 1
#define HAVE_muldf3 1
#define HAVE_mulsf3 1
#define HAVE_divdf3 1
#define HAVE_divsf3 1
#define HAVE_negdf2 1
#define HAVE_negsf2 1
#define HAVE_absdf2 1
#define HAVE_abssf2 1
#define HAVE_sqrtdf2 1
#define HAVE_sqrtsf2 1
#define HAVE_ashlsi3 1
#define HAVE_zvdep32 1
#define HAVE_zvdep_imm 1
#define HAVE_ashrsi3 1
#define HAVE_vextrs32 1
#define HAVE_lshrsi3 1
#define HAVE_rotrsi3 1
#define HAVE_rotlsi3 1
#define HAVE_return (hppa_can_use_return_insn_p ())
#define HAVE_return_internal 1
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_call_profiler 1
#define HAVE_blockage 1
#define HAVE_jump 1
#define HAVE_casesi 1
#define HAVE_casesi0 1
#define HAVE_call 1
#define HAVE_call_internal_symref (! TARGET_LONG_CALLS)
#define HAVE_call_internal_reg 1
#define HAVE_call_value 1
#define HAVE_call_value_internal_symref (! TARGET_LONG_CALLS)
#define HAVE_call_value_internal_reg 1
#define HAVE_nop 1
#define HAVE_indirect_jump 1
#define HAVE_extzv 1
#define HAVE_extv 1
#define HAVE_insv 1
#define HAVE_decrement_and_branch_until_zero 1
#define HAVE_cacheflush 1

#ifndef NO_MD_PROTOTYPES
extern rtx gen_cmpsi                           PROTO((rtx, rtx));
extern rtx gen_cmpsf                           PROTO((rtx, rtx));
extern rtx gen_cmpdf                           PROTO((rtx, rtx));
extern rtx gen_seq                             PROTO((rtx));
extern rtx gen_sne                             PROTO((rtx));
extern rtx gen_slt                             PROTO((rtx));
extern rtx gen_sgt                             PROTO((rtx));
extern rtx gen_sle                             PROTO((rtx));
extern rtx gen_sge                             PROTO((rtx));
extern rtx gen_sltu                            PROTO((rtx));
extern rtx gen_sgtu                            PROTO((rtx));
extern rtx gen_sleu                            PROTO((rtx));
extern rtx gen_sgeu                            PROTO((rtx));
extern rtx gen_scc                             PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_negscc                          PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_incscc                          PROTO((rtx, rtx, rtx, rtx, rtx));
extern rtx gen_decscc                          PROTO((rtx, rtx, rtx, rtx, rtx));
extern rtx gen_sminsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_uminsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_smaxsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_umaxsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_beq                             PROTO((rtx));
extern rtx gen_bne                             PROTO((rtx));
extern rtx gen_bgt                             PROTO((rtx));
extern rtx gen_blt                             PROTO((rtx));
extern rtx gen_bge                             PROTO((rtx));
extern rtx gen_ble                             PROTO((rtx));
extern rtx gen_bgtu                            PROTO((rtx));
extern rtx gen_bltu                            PROTO((rtx));
extern rtx gen_bgeu                            PROTO((rtx));
extern rtx gen_bleu                            PROTO((rtx));
extern rtx gen_movsi                           PROTO((rtx, rtx));
extern rtx gen_reload_insi                     PROTO((rtx, rtx, rtx));
extern rtx gen_reload_outsi                    PROTO((rtx, rtx, rtx));
extern rtx gen_pre_ldwm                        PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_pre_stwm                        PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_post_ldwm                       PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_post_stwm                       PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_add_high_const                  PROTO((rtx, rtx, rtx));
extern rtx gen_movhi                           PROTO((rtx, rtx));
extern rtx gen_movqi                           PROTO((rtx, rtx));
extern rtx gen_movstrsi                        PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_movdf                           PROTO((rtx, rtx));
extern rtx gen_movdi                           PROTO((rtx, rtx));
extern rtx gen_reload_indi                     PROTO((rtx, rtx, rtx));
extern rtx gen_reload_outdi                    PROTO((rtx, rtx, rtx));
extern rtx gen_movsf                           PROTO((rtx, rtx));
extern rtx gen_zero_extendhisi2                PROTO((rtx, rtx));
extern rtx gen_zero_extendqihi2                PROTO((rtx, rtx));
extern rtx gen_zero_extendqisi2                PROTO((rtx, rtx));
extern rtx gen_extendhisi2                     PROTO((rtx, rtx));
extern rtx gen_extendqihi2                     PROTO((rtx, rtx));
extern rtx gen_extendqisi2                     PROTO((rtx, rtx));
extern rtx gen_extendsfdf2                     PROTO((rtx, rtx));
extern rtx gen_truncdfsf2                      PROTO((rtx, rtx));
extern rtx gen_floatsisf2                      PROTO((rtx, rtx));
extern rtx gen_floatsidf2                      PROTO((rtx, rtx));
extern rtx gen_floatunssisf2                   PROTO((rtx, rtx));
extern rtx gen_floatunssidf2                   PROTO((rtx, rtx));
extern rtx gen_floatdisf2                      PROTO((rtx, rtx));
extern rtx gen_floatdidf2                      PROTO((rtx, rtx));
extern rtx gen_fix_truncsfsi2                  PROTO((rtx, rtx));
extern rtx gen_fix_truncdfsi2                  PROTO((rtx, rtx));
extern rtx gen_fix_truncsfdi2                  PROTO((rtx, rtx));
extern rtx gen_fix_truncdfdi2                  PROTO((rtx, rtx));
extern rtx gen_adddi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_addsi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_subdi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_subsi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_mulsi3                          PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_umulsidi3                       PROTO((rtx, rtx, rtx));
extern rtx gen_divsi3                          PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_udivsi3                         PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_modsi3                          PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_umodsi3                         PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_anddi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_andsi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_iordi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_iorsi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_xordi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_xorsi3                          PROTO((rtx, rtx, rtx));
extern rtx gen_negdi2                          PROTO((rtx, rtx));
extern rtx gen_negsi2                          PROTO((rtx, rtx));
extern rtx gen_one_cmpldi2                     PROTO((rtx, rtx));
extern rtx gen_one_cmplsi2                     PROTO((rtx, rtx));
extern rtx gen_adddf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_addsf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_subdf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_subsf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_muldf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_mulsf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_divdf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_divsf3                          PROTO((rtx, rtx, rtx));
extern rtx gen_negdf2                          PROTO((rtx, rtx));
extern rtx gen_negsf2                          PROTO((rtx, rtx));
extern rtx gen_absdf2                          PROTO((rtx, rtx));
extern rtx gen_abssf2                          PROTO((rtx, rtx));
extern rtx gen_sqrtdf2                         PROTO((rtx, rtx));
extern rtx gen_sqrtsf2                         PROTO((rtx, rtx));
extern rtx gen_ashlsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_zvdep32                         PROTO((rtx, rtx, rtx));
extern rtx gen_zvdep_imm                       PROTO((rtx, rtx, rtx));
extern rtx gen_ashrsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_vextrs32                        PROTO((rtx, rtx, rtx));
extern rtx gen_lshrsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_rotrsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_rotlsi3                         PROTO((rtx, rtx, rtx));
extern rtx gen_return                          PROTO((void));
extern rtx gen_return_internal                 PROTO((void));
extern rtx gen_prologue                        PROTO((void));
extern rtx gen_epilogue                        PROTO((void));
extern rtx gen_call_profiler                   PROTO((rtx));
extern rtx gen_blockage                        PROTO((void));
extern rtx gen_jump                            PROTO((rtx));
extern rtx gen_casesi                          PROTO((rtx, rtx, rtx, rtx, rtx));
extern rtx gen_casesi0                         PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_call_internal_symref            PROTO((rtx, rtx));
extern rtx gen_call_internal_reg               PROTO((rtx, rtx));
extern rtx gen_call_value_internal_symref      PROTO((rtx, rtx, rtx));
extern rtx gen_call_value_internal_reg         PROTO((rtx, rtx, rtx));
extern rtx gen_nop                             PROTO((void));
extern rtx gen_indirect_jump                   PROTO((rtx));
extern rtx gen_extzv                           PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_extv                            PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_insv                            PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_decrement_and_branch_until_zero PROTO((rtx, rtx, rtx, rtx));
extern rtx gen_cacheflush                      PROTO((rtx, rtx));

#ifdef MD_CALL_PROTOTYPES
extern rtx gen_call                            PROTO((rtx, rtx));
extern rtx gen_call_value                      PROTO((rtx, rtx, rtx));

#else /* !MD_CALL_PROTOTYPES */
extern rtx gen_call ();
extern rtx gen_call_value ();
#endif /* !MD_CALL_PROTOTYPES */

#else  /* NO_MD_PROTOTYPES */
extern rtx gen_cmpsi ();
extern rtx gen_cmpsf ();
extern rtx gen_cmpdf ();
extern rtx gen_seq ();
extern rtx gen_sne ();
extern rtx gen_slt ();
extern rtx gen_sgt ();
extern rtx gen_sle ();
extern rtx gen_sge ();
extern rtx gen_sltu ();
extern rtx gen_sgtu ();
extern rtx gen_sleu ();
extern rtx gen_sgeu ();
extern rtx gen_scc ();
extern rtx gen_negscc ();
extern rtx gen_incscc ();
extern rtx gen_decscc ();
extern rtx gen_sminsi3 ();
extern rtx gen_uminsi3 ();
extern rtx gen_smaxsi3 ();
extern rtx gen_umaxsi3 ();
extern rtx gen_beq ();
extern rtx gen_bne ();
extern rtx gen_bgt ();
extern rtx gen_blt ();
extern rtx gen_bge ();
extern rtx gen_ble ();
extern rtx gen_bgtu ();
extern rtx gen_bltu ();
extern rtx gen_bgeu ();
extern rtx gen_bleu ();
extern rtx gen_movsi ();
extern rtx gen_reload_insi ();
extern rtx gen_reload_outsi ();
extern rtx gen_pre_ldwm ();
extern rtx gen_pre_stwm ();
extern rtx gen_post_ldwm ();
extern rtx gen_post_stwm ();
extern rtx gen_add_high_const ();
extern rtx gen_movhi ();
extern rtx gen_movqi ();
extern rtx gen_movstrsi ();
extern rtx gen_movdf ();
extern rtx gen_movdi ();
extern rtx gen_reload_indi ();
extern rtx gen_reload_outdi ();
extern rtx gen_movsf ();
extern rtx gen_zero_extendhisi2 ();
extern rtx gen_zero_extendqihi2 ();
extern rtx gen_zero_extendqisi2 ();
extern rtx gen_extendhisi2 ();
extern rtx gen_extendqihi2 ();
extern rtx gen_extendqisi2 ();
extern rtx gen_extendsfdf2 ();
extern rtx gen_truncdfsf2 ();
extern rtx gen_floatsisf2 ();
extern rtx gen_floatsidf2 ();
extern rtx gen_floatunssisf2 ();
extern rtx gen_floatunssidf2 ();
extern rtx gen_floatdisf2 ();
extern rtx gen_floatdidf2 ();
extern rtx gen_fix_truncsfsi2 ();
extern rtx gen_fix_truncdfsi2 ();
extern rtx gen_fix_truncsfdi2 ();
extern rtx gen_fix_truncdfdi2 ();
extern rtx gen_adddi3 ();
extern rtx gen_addsi3 ();
extern rtx gen_subdi3 ();
extern rtx gen_subsi3 ();
extern rtx gen_mulsi3 ();
extern rtx gen_umulsidi3 ();
extern rtx gen_divsi3 ();
extern rtx gen_udivsi3 ();
extern rtx gen_modsi3 ();
extern rtx gen_umodsi3 ();
extern rtx gen_anddi3 ();
extern rtx gen_andsi3 ();
extern rtx gen_iordi3 ();
extern rtx gen_iorsi3 ();
extern rtx gen_xordi3 ();
extern rtx gen_xorsi3 ();
extern rtx gen_negdi2 ();
extern rtx gen_negsi2 ();
extern rtx gen_one_cmpldi2 ();
extern rtx gen_one_cmplsi2 ();
extern rtx gen_adddf3 ();
extern rtx gen_addsf3 ();
extern rtx gen_subdf3 ();
extern rtx gen_subsf3 ();
extern rtx gen_muldf3 ();
extern rtx gen_mulsf3 ();
extern rtx gen_divdf3 ();
extern rtx gen_divsf3 ();
extern rtx gen_negdf2 ();
extern rtx gen_negsf2 ();
extern rtx gen_absdf2 ();
extern rtx gen_abssf2 ();
extern rtx gen_sqrtdf2 ();
extern rtx gen_sqrtsf2 ();
extern rtx gen_ashlsi3 ();
extern rtx gen_zvdep32 ();
extern rtx gen_zvdep_imm ();
extern rtx gen_ashrsi3 ();
extern rtx gen_vextrs32 ();
extern rtx gen_lshrsi3 ();
extern rtx gen_rotrsi3 ();
extern rtx gen_rotlsi3 ();
extern rtx gen_return ();
extern rtx gen_return_internal ();
extern rtx gen_prologue ();
extern rtx gen_epilogue ();
extern rtx gen_call_profiler ();
extern rtx gen_blockage ();
extern rtx gen_jump ();
extern rtx gen_casesi ();
extern rtx gen_casesi0 ();
extern rtx gen_call_internal_symref ();
extern rtx gen_call_internal_reg ();
extern rtx gen_call_value_internal_symref ();
extern rtx gen_call_value_internal_reg ();
extern rtx gen_nop ();
extern rtx gen_indirect_jump ();
extern rtx gen_extzv ();
extern rtx gen_extv ();
extern rtx gen_insv ();
extern rtx gen_decrement_and_branch_until_zero ();
extern rtx gen_cacheflush ();
extern rtx gen_call ();
extern rtx gen_call_value ();
#endif  /* NO_MD_PROTOTYPES */
