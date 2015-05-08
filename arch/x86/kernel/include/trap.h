/*
 * arch/x86/kernel/include/trap.h
 *
 * Created By Le Min at 2015/01/03
 */

#ifndef _TRAP_H_
#define _TRAP_H

/* copy from linux */
enum {
	X86_TRAP_DE = 0,	/*  0, Divide-by-zero */
	X86_TRAP_DB,		/*  1, Debug */
	X86_TRAP_NMI,		/*  2, Non-maskable Interrupt */
	X86_TRAP_BP,		/*  3, Breakpoint */
	X86_TRAP_OF,		/*  4, Overflow */
	X86_TRAP_BR,		/*  5, Bound Range Exceeded */
	X86_TRAP_UD,		/*  6, Invalid Opcode */
	X86_TRAP_NM,		/*  7, Device Not Available */
	X86_TRAP_DF,		/*  8, Double Fault */
	X86_TRAP_OLD_MF,	/*  9, Coprocessor Segment Overrun */
	X86_TRAP_TS,		/* 10, Invalid TSS */
	X86_TRAP_NP,		/* 11, Segment Not Present */
	X86_TRAP_SS,		/* 12, Stack Segment Fault */
	X86_TRAP_GP,		/* 13, General Protection Fault */
	X86_TRAP_PF,		/* 14, Page Fault */
	X86_TRAP_SPURIOUS,	/* 15, Spurious Interrupt */
	X86_TRAP_MF,		/* 16, x87 Floating-Point Exception */
	X86_TRAP_AC,		/* 17, Alignment Check */
	X86_TRAP_MC,		/* 18, Machine Check */
	X86_TRAP_XF,		/* 19, SIMD Floating-Point Exception */
};

void x86_trap_de(void);
void x86_trap_db(void);
void x86_trap_nmi(void);
void x86_trap_bp(void);
void x86_trap_of(void);
void x86_trap_br(void);
void x86_trap_ud(void);
void x86_trap_nm(void);
void x86_trap_df(void);
void x86_trap_old_mf(void);
void x86_trap_ts(void);
void x86_trap_np(void);
void x86_trap_ss(void);
void x86_trap_gp(void);
void x86_trap_pf(void);
void x86_trap_spurious(void);
void x86_trap_mf(void);
void x86_trap_ac(void);
void x86_trap_nc(void);
void x86_trap_xf(void);
void x86_trap_undef(void);

void x86_irq_0(void);
void x86_irq_1(void);
void x86_irq_2(void);
void x86_irq_3(void);
void x86_irq_4(void);
void x86_irq_5(void);
void x86_irq_6(void);
void x86_irq_7(void);
void x86_irq_8(void);
void x86_irq_9(void);
void x86_irq_10(void);
void x86_irq_11(void);
void x86_irq_12(void);
void x86_irq_13(void);
void x86_irq_14(void);
void x86_irq_15(void);
void x86_irq_16(void);
void x86_irq_17(void);
void x86_irq_18(void);
void x86_irq_19(void);
void x86_irq_20(void);
void x86_irq_21(void);
void x86_irq_22(void);
void x86_irq_23(void);
void x86_irq_24(void);
void x86_irq_25(void);
void x86_irq_26(void);
void x86_irq_27(void);
void x86_irq_28(void);
void x86_irq_29(void);
void x86_irq_30(void);
void x86_irq_31(void);
void x86_irq_32(void);
void x86_irq_33(void);
void x86_irq_34(void);
void x86_irq_35(void);
void x86_irq_36(void);
void x86_irq_37(void);
void x86_irq_38(void);
void x86_irq_39(void);
void x86_irq_40(void);
void x86_irq_41(void);
void x86_irq_42(void);
void x86_irq_43(void);
void x86_irq_44(void);
void x86_irq_45(void);
void x86_irq_46(void);
void x86_irq_47(void);
void x86_irq_48(void);
void x86_irq_49(void);
void x86_irq_50(void);
void x86_irq_51(void);
void x86_irq_52(void);
void x86_irq_53(void);
void x86_irq_54(void);
void x86_irq_55(void);
void x86_irq_56(void);
void x86_irq_57(void);
void x86_irq_58(void);
void x86_irq_59(void);
void x86_irq_60(void);
void x86_irq_61(void);
void x86_irq_62(void);
void x86_irq_63(void);
void x86_irq_64(void);
void x86_irq_65(void);
void x86_irq_66(void);
void x86_irq_67(void);
void x86_irq_68(void);
void x86_irq_69(void);
void x86_irq_70(void);
void x86_irq_71(void);
void x86_irq_72(void);
void x86_irq_73(void);
void x86_irq_74(void);
void x86_irq_75(void);
void x86_irq_76(void);
void x86_irq_77(void);
void x86_irq_78(void);
void x86_irq_79(void);
void x86_irq_80(void);
void x86_irq_81(void);
void x86_irq_82(void);
void x86_irq_83(void);
void x86_irq_84(void);
void x86_irq_85(void);
void x86_irq_86(void);
void x86_irq_87(void);
void x86_irq_88(void);
void x86_irq_89(void);
void x86_irq_90(void);
void x86_irq_91(void);
void x86_irq_92(void);
void x86_irq_93(void);
void x86_irq_94(void);
void x86_irq_95(void);
void x86_irq_96(void);
void x86_irq_97(void);
void x86_irq_98(void);
void x86_irq_99(void);
void x86_irq_100(void);
void x86_irq_101(void);
void x86_irq_102(void);
void x86_irq_103(void);
void x86_irq_104(void);
void x86_irq_105(void);
void x86_irq_106(void);
void x86_irq_107(void);
void x86_irq_108(void);
void x86_irq_109(void);
void x86_irq_110(void);
void x86_irq_111(void);
void x86_irq_112(void);
void x86_irq_113(void);
void x86_irq_114(void);
void x86_irq_115(void);
void x86_irq_116(void);
void x86_irq_117(void);
void x86_irq_118(void);
void x86_irq_119(void);
void x86_irq_120(void);
void x86_irq_121(void);
void x86_irq_122(void);
void x86_irq_123(void);
void x86_irq_124(void);
void x86_irq_125(void);
void x86_irq_126(void);
void x86_irq_127(void);
void x86_irq_128(void);
void x86_irq_129(void);
void x86_irq_130(void);
void x86_irq_131(void);
void x86_irq_132(void);
void x86_irq_133(void);
void x86_irq_134(void);
void x86_irq_135(void);
void x86_irq_136(void);
void x86_irq_137(void);
void x86_irq_138(void);
void x86_irq_139(void);
void x86_irq_140(void);
void x86_irq_141(void);
void x86_irq_142(void);
void x86_irq_143(void);
void x86_irq_144(void);
void x86_irq_145(void);
void x86_irq_146(void);
void x86_irq_147(void);
void x86_irq_148(void);
void x86_irq_149(void);
void x86_irq_150(void);
void x86_irq_151(void);
void x86_irq_152(void);
void x86_irq_153(void);
void x86_irq_154(void);
void x86_irq_155(void);
void x86_irq_156(void);
void x86_irq_157(void);
void x86_irq_158(void);
void x86_irq_159(void);
void x86_irq_160(void);
void x86_irq_161(void);
void x86_irq_162(void);
void x86_irq_163(void);
void x86_irq_164(void);
void x86_irq_165(void);
void x86_irq_166(void);
void x86_irq_167(void);
void x86_irq_168(void);
void x86_irq_169(void);
void x86_irq_170(void);
void x86_irq_171(void);
void x86_irq_172(void);
void x86_irq_173(void);
void x86_irq_174(void);
void x86_irq_175(void);
void x86_irq_176(void);
void x86_irq_177(void);
void x86_irq_178(void);
void x86_irq_179(void);
void x86_irq_180(void);
void x86_irq_181(void);
void x86_irq_182(void);
void x86_irq_183(void);
void x86_irq_184(void);
void x86_irq_185(void);
void x86_irq_186(void);
void x86_irq_187(void);
void x86_irq_188(void);
void x86_irq_189(void);
void x86_irq_190(void);
void x86_irq_191(void);
void x86_irq_192(void);
void x86_irq_193(void);
void x86_irq_194(void);
void x86_irq_195(void);
void x86_irq_196(void);
void x86_irq_197(void);
void x86_irq_198(void);
void x86_irq_199(void);
void x86_irq_200(void);
void x86_irq_201(void);
void x86_irq_202(void);
void x86_irq_203(void);
void x86_irq_204(void);
void x86_irq_205(void);
void x86_irq_206(void);
void x86_irq_207(void);
void x86_irq_208(void);
void x86_irq_209(void);
void x86_irq_210(void);
void x86_irq_211(void);
void x86_irq_212(void);
void x86_irq_213(void);
void x86_irq_214(void);
void x86_irq_215(void);
void x86_irq_216(void);
void x86_irq_217(void);
void x86_irq_218(void);
void x86_irq_219(void);
void x86_irq_220(void);
void x86_irq_221(void);
void x86_irq_222(void);
void x86_irq_223(void);

#endif
