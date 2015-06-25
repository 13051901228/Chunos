/*
 * arch/x86/kernel/idt.c
 *
 * Created By Le Min at 2015/01/03
 */
#include "include/idt.h"
#include <os/kernel.h>
#include <os/init.h>
#include "include/gdt.h"
#include "include/trap.h"
#include <os/panic.h>

void x86_trap_handler_de(int errcode)
{
	panic("DE FAULT");
}

void x86_trap_handler_db(int errcode)
{
	panic("DB FAULT");
}

void x86_trap_handler_nmi(int errcode)
{
	panic("NMI FAULT");
}

void x86_trap_handler_bp(int errcode)
{
	panic("BP FAULT");
}

void x86_trap_handler_of(int errcode)
{
	panic("OF FAULT");
}

void x86_trap_handler_br(int errcode)
{
	panic("BR FAULT");
}

void x86_trap_handler_ud(int errcode)
{
	panic("UD FAULT");
}

void x86_trap_handler_nm(int errcode)
{
	panic("NM FAULT");
}

void x86_trap_handler_df(int errcode)
{
	panic("DF FAULT");
}

void x86_trap_handler_old_mf(int errcode)
{
	panic("MF FAULT");
}

void x86_trap_handler_ts(int errcode)
{
	panic("TS FAULT");
}

void x86_trap_handler_np(int errcode)
{
	panic("NP FAULT");
}

void x86_trap_handler_ss(int errcode)
{
	panic("SS FAULT");
}

void x86_trap_handler_gp(int errcode)
{
	panic("GP FAULT");
}

void x86_trap_handler_pf(int errcode)
{
	panic("PF FAULT");
}

void x86_trap_handler_spurious(int errcode)
{
	panic("SPURIOUS FAULT");
}

void x86_trap_handler_mf(int errcode)
{
	panic("MF FAULT");
}

void x86_trap_handler_ac(int errcode)
{
	panic("AC FAULT");
}

void x86_trap_handler_nc(int errcode)
{
	panic("NC FAULT");
}

void x86_trap_handler_xf(int errcode)
{
	panic("XF FAULT");
}

void x86_trap_handler_undef(int errcode)
{
	panic("UNDEF isr");
}

void __init_text trap_init(void)
{
	install_trap_gate(X86_TRAP_DE, (unsigned long)x86_trap_de);
	install_trap_gate(X86_TRAP_DB, (unsigned long)x86_trap_db);
	install_trap_gate(X86_TRAP_NMI, (unsigned long)x86_trap_nmi);
	install_trap_gate(X86_TRAP_BP, (unsigned long)x86_trap_bp);
	install_trap_gate(X86_TRAP_OF, (unsigned long)x86_trap_of);
	install_trap_gate(X86_TRAP_BR, (unsigned long)x86_trap_br);
	install_trap_gate(X86_TRAP_UD, (unsigned long)x86_trap_ud);
	install_trap_gate(X86_TRAP_NM, (unsigned long)x86_trap_nm);
	install_trap_gate(X86_TRAP_DF, (unsigned long)x86_trap_df);
	install_trap_gate(X86_TRAP_OLD_MF, (unsigned long)x86_trap_old_mf);
	install_trap_gate(X86_TRAP_TS, (unsigned long)x86_trap_ts);
	install_trap_gate(X86_TRAP_NP, (unsigned long)x86_trap_np);
	install_trap_gate(X86_TRAP_SS, (unsigned long)x86_trap_ss);
	install_trap_gate(X86_TRAP_GP, (unsigned long)x86_trap_gp);
	install_trap_gate(X86_TRAP_PF, (unsigned long)x86_trap_pf);
	install_trap_gate(X86_TRAP_SPURIOUS, (unsigned long)x86_trap_spurious);
	install_trap_gate(X86_TRAP_MF, (unsigned long)x86_trap_mf);
	install_trap_gate(X86_TRAP_AC, (unsigned long)x86_trap_ac);
	install_trap_gate(X86_TRAP_MC, (unsigned long)x86_trap_nc);
	install_trap_gate(X86_TRAP_XF, (unsigned long)x86_trap_xf);

	install_trap_gate(20, (unsigned long)x86_trap_undef);
	install_trap_gate(21, (unsigned long)x86_trap_undef);
	install_trap_gate(22, (unsigned long)x86_trap_undef);
	install_trap_gate(23, (unsigned long)x86_trap_undef);
	install_trap_gate(24, (unsigned long)x86_trap_undef);
	install_trap_gate(25, (unsigned long)x86_trap_undef);
	install_trap_gate(26, (unsigned long)x86_trap_undef);
	install_trap_gate(27, (unsigned long)x86_trap_undef);
	install_trap_gate(28, (unsigned long)x86_trap_undef);
	install_trap_gate(29, (unsigned long)x86_trap_undef);
	install_trap_gate(30, (unsigned long)x86_trap_undef);
	install_trap_gate(31, (unsigned long)x86_trap_undef);

	install_interrupt_gate(32, (unsigned long)x86_irq_0);
	install_interrupt_gate(33, (unsigned long)x86_irq_1);
	install_interrupt_gate(34, (unsigned long)x86_irq_2);
	install_interrupt_gate(35, (unsigned long)x86_irq_3);
	install_interrupt_gate(36, (unsigned long)x86_irq_4);
	install_interrupt_gate(37, (unsigned long)x86_irq_5);
	install_interrupt_gate(38, (unsigned long)x86_irq_6);
	install_interrupt_gate(39, (unsigned long)x86_irq_7);
	install_interrupt_gate(40, (unsigned long)x86_irq_8);
	install_interrupt_gate(41, (unsigned long)x86_irq_9);
	install_interrupt_gate(42, (unsigned long)x86_irq_10);
	install_interrupt_gate(43, (unsigned long)x86_irq_11);
	install_interrupt_gate(44, (unsigned long)x86_irq_12);
	install_interrupt_gate(45, (unsigned long)x86_irq_13);
	install_interrupt_gate(46, (unsigned long)x86_irq_14);
	install_interrupt_gate(47, (unsigned long)x86_irq_15);
	install_interrupt_gate(48, (unsigned long)x86_irq_16);
	install_interrupt_gate(49, (unsigned long)x86_irq_17);
	install_interrupt_gate(50, (unsigned long)x86_irq_18);
	install_interrupt_gate(51, (unsigned long)x86_irq_19);
	install_interrupt_gate(52, (unsigned long)x86_irq_20);
	install_interrupt_gate(53, (unsigned long)x86_irq_21);
	install_interrupt_gate(54, (unsigned long)x86_irq_22);
	install_interrupt_gate(55, (unsigned long)x86_irq_23);
	install_interrupt_gate(56, (unsigned long)x86_irq_24);
	install_interrupt_gate(57, (unsigned long)x86_irq_25);
	install_interrupt_gate(58, (unsigned long)x86_irq_26);
	install_interrupt_gate(59, (unsigned long)x86_irq_27);
	install_interrupt_gate(60, (unsigned long)x86_irq_28);
	install_interrupt_gate(61, (unsigned long)x86_irq_29);
	install_interrupt_gate(62, (unsigned long)x86_irq_30);
	install_interrupt_gate(63, (unsigned long)x86_irq_31);
	install_interrupt_gate(64, (unsigned long)x86_irq_32);
	install_interrupt_gate(65, (unsigned long)x86_irq_33);
	install_interrupt_gate(66, (unsigned long)x86_irq_34);
	install_interrupt_gate(67, (unsigned long)x86_irq_35);
	install_interrupt_gate(68, (unsigned long)x86_irq_36);
	install_interrupt_gate(69, (unsigned long)x86_irq_37);
	install_interrupt_gate(70, (unsigned long)x86_irq_38);
	install_interrupt_gate(71, (unsigned long)x86_irq_39);
	install_interrupt_gate(72, (unsigned long)x86_irq_40);
	install_interrupt_gate(73, (unsigned long)x86_irq_41);
	install_interrupt_gate(74, (unsigned long)x86_irq_42);
	install_interrupt_gate(75, (unsigned long)x86_irq_43);
	install_interrupt_gate(76, (unsigned long)x86_irq_44);
	install_interrupt_gate(77, (unsigned long)x86_irq_45);
	install_interrupt_gate(78, (unsigned long)x86_irq_46);
	install_interrupt_gate(79, (unsigned long)x86_irq_47);
	install_interrupt_gate(80, (unsigned long)x86_irq_48);
	install_interrupt_gate(81, (unsigned long)x86_irq_49);
	install_interrupt_gate(82, (unsigned long)x86_irq_50);
	install_interrupt_gate(83, (unsigned long)x86_irq_51);
	install_interrupt_gate(84, (unsigned long)x86_irq_52);
	install_interrupt_gate(85, (unsigned long)x86_irq_53);
	install_interrupt_gate(86, (unsigned long)x86_irq_54);
	install_interrupt_gate(87, (unsigned long)x86_irq_55);
	install_interrupt_gate(88, (unsigned long)x86_irq_56);
	install_interrupt_gate(89, (unsigned long)x86_irq_57);
	install_interrupt_gate(90, (unsigned long)x86_irq_58);
	install_interrupt_gate(91, (unsigned long)x86_irq_59);
	install_interrupt_gate(92, (unsigned long)x86_irq_60);
	install_interrupt_gate(93, (unsigned long)x86_irq_61);
	install_interrupt_gate(94, (unsigned long)x86_irq_62);
	install_interrupt_gate(95, (unsigned long)x86_irq_63);
	install_interrupt_gate(96, (unsigned long)x86_irq_64);
	install_interrupt_gate(97, (unsigned long)x86_irq_65);
	install_interrupt_gate(98, (unsigned long)x86_irq_66);
	install_interrupt_gate(99, (unsigned long)x86_irq_67);
	install_interrupt_gate(100, (unsigned long)x86_irq_68);
	install_interrupt_gate(101, (unsigned long)x86_irq_69);
	install_interrupt_gate(102, (unsigned long)x86_irq_70);
	install_interrupt_gate(103, (unsigned long)x86_irq_71);
	install_interrupt_gate(104, (unsigned long)x86_irq_72);
	install_interrupt_gate(105, (unsigned long)x86_irq_73);
	install_interrupt_gate(106, (unsigned long)x86_irq_74);
	install_interrupt_gate(107, (unsigned long)x86_irq_75);
	install_interrupt_gate(108, (unsigned long)x86_irq_76);
	install_interrupt_gate(109, (unsigned long)x86_irq_77);
	install_interrupt_gate(110, (unsigned long)x86_irq_78);
	install_interrupt_gate(111, (unsigned long)x86_irq_79);
	install_interrupt_gate(112, (unsigned long)x86_irq_80);
	install_interrupt_gate(113, (unsigned long)x86_irq_81);
	install_interrupt_gate(114, (unsigned long)x86_irq_82);
	install_interrupt_gate(115, (unsigned long)x86_irq_83);
	install_interrupt_gate(116, (unsigned long)x86_irq_84);
	install_interrupt_gate(117, (unsigned long)x86_irq_85);
	install_interrupt_gate(118, (unsigned long)x86_irq_86);
	install_interrupt_gate(119, (unsigned long)x86_irq_87);
	install_interrupt_gate(120, (unsigned long)x86_irq_88);
	install_interrupt_gate(121, (unsigned long)x86_irq_89);
	install_interrupt_gate(122, (unsigned long)x86_irq_90);
	install_interrupt_gate(123, (unsigned long)x86_irq_91);
	install_interrupt_gate(124, (unsigned long)x86_irq_92);
	install_interrupt_gate(125, (unsigned long)x86_irq_93);
	install_interrupt_gate(126, (unsigned long)x86_irq_94);
	install_interrupt_gate(127, (unsigned long)x86_irq_95);

	install_syscall_gate(128, (unsigned long)x86_irq_syscall);

	install_interrupt_gate(129, (unsigned long)x86_irq_97);
	install_interrupt_gate(130, (unsigned long)x86_irq_98);
	install_interrupt_gate(131, (unsigned long)x86_irq_99);
	install_interrupt_gate(132, (unsigned long)x86_irq_100);
	install_interrupt_gate(133, (unsigned long)x86_irq_101);
	install_interrupt_gate(134, (unsigned long)x86_irq_102);
	install_interrupt_gate(135, (unsigned long)x86_irq_103);
	install_interrupt_gate(136, (unsigned long)x86_irq_104);
	install_interrupt_gate(137, (unsigned long)x86_irq_105);
	install_interrupt_gate(138, (unsigned long)x86_irq_106);
	install_interrupt_gate(139, (unsigned long)x86_irq_107);
	install_interrupt_gate(140, (unsigned long)x86_irq_108);
	install_interrupt_gate(141, (unsigned long)x86_irq_109);
	install_interrupt_gate(142, (unsigned long)x86_irq_110);
	install_interrupt_gate(143, (unsigned long)x86_irq_111);
	install_interrupt_gate(144, (unsigned long)x86_irq_112);
	install_interrupt_gate(145, (unsigned long)x86_irq_113);
	install_interrupt_gate(146, (unsigned long)x86_irq_114);
	install_interrupt_gate(147, (unsigned long)x86_irq_115);
	install_interrupt_gate(148, (unsigned long)x86_irq_116);
	install_interrupt_gate(149, (unsigned long)x86_irq_117);
	install_interrupt_gate(150, (unsigned long)x86_irq_118);
	install_interrupt_gate(151, (unsigned long)x86_irq_119);
	install_interrupt_gate(152, (unsigned long)x86_irq_120);
	install_interrupt_gate(153, (unsigned long)x86_irq_121);
	install_interrupt_gate(154, (unsigned long)x86_irq_122);
	install_interrupt_gate(155, (unsigned long)x86_irq_123);
	install_interrupt_gate(156, (unsigned long)x86_irq_124);
	install_interrupt_gate(157, (unsigned long)x86_irq_125);
	install_interrupt_gate(158, (unsigned long)x86_irq_126);
	install_interrupt_gate(159, (unsigned long)x86_irq_127);
	install_interrupt_gate(160, (unsigned long)x86_irq_128);
	install_interrupt_gate(161, (unsigned long)x86_irq_129);
	install_interrupt_gate(162, (unsigned long)x86_irq_130);
	install_interrupt_gate(163, (unsigned long)x86_irq_131);
	install_interrupt_gate(164, (unsigned long)x86_irq_132);
	install_interrupt_gate(165, (unsigned long)x86_irq_133);
	install_interrupt_gate(166, (unsigned long)x86_irq_134);
	install_interrupt_gate(167, (unsigned long)x86_irq_135);
	install_interrupt_gate(168, (unsigned long)x86_irq_136);
	install_interrupt_gate(169, (unsigned long)x86_irq_137);
	install_interrupt_gate(170, (unsigned long)x86_irq_138);
	install_interrupt_gate(171, (unsigned long)x86_irq_139);
	install_interrupt_gate(172, (unsigned long)x86_irq_140);
	install_interrupt_gate(173, (unsigned long)x86_irq_141);
	install_interrupt_gate(174, (unsigned long)x86_irq_142);
	install_interrupt_gate(175, (unsigned long)x86_irq_143);
	install_interrupt_gate(176, (unsigned long)x86_irq_144);
	install_interrupt_gate(177, (unsigned long)x86_irq_145);
	install_interrupt_gate(178, (unsigned long)x86_irq_146);
	install_interrupt_gate(179, (unsigned long)x86_irq_147);
	install_interrupt_gate(180, (unsigned long)x86_irq_148);
	install_interrupt_gate(181, (unsigned long)x86_irq_149);
	install_interrupt_gate(182, (unsigned long)x86_irq_150);
	install_interrupt_gate(183, (unsigned long)x86_irq_151);
	install_interrupt_gate(184, (unsigned long)x86_irq_152);
	install_interrupt_gate(185, (unsigned long)x86_irq_153);
	install_interrupt_gate(186, (unsigned long)x86_irq_154);
	install_interrupt_gate(187, (unsigned long)x86_irq_155);
	install_interrupt_gate(188, (unsigned long)x86_irq_156);
	install_interrupt_gate(189, (unsigned long)x86_irq_157);
	install_interrupt_gate(190, (unsigned long)x86_irq_158);
	install_interrupt_gate(191, (unsigned long)x86_irq_159);
	install_interrupt_gate(192, (unsigned long)x86_irq_160);
	install_interrupt_gate(193, (unsigned long)x86_irq_161);
	install_interrupt_gate(194, (unsigned long)x86_irq_162);
	install_interrupt_gate(195, (unsigned long)x86_irq_163);
	install_interrupt_gate(196, (unsigned long)x86_irq_164);
	install_interrupt_gate(197, (unsigned long)x86_irq_165);
	install_interrupt_gate(198, (unsigned long)x86_irq_166);
	install_interrupt_gate(199, (unsigned long)x86_irq_167);
	install_interrupt_gate(200, (unsigned long)x86_irq_168);
	install_interrupt_gate(201, (unsigned long)x86_irq_169);
	install_interrupt_gate(202, (unsigned long)x86_irq_170);
	install_interrupt_gate(203, (unsigned long)x86_irq_171);
	install_interrupt_gate(204, (unsigned long)x86_irq_172);
	install_interrupt_gate(205, (unsigned long)x86_irq_173);
	install_interrupt_gate(206, (unsigned long)x86_irq_174);
	install_interrupt_gate(207, (unsigned long)x86_irq_175);
	install_interrupt_gate(208, (unsigned long)x86_irq_176);
	install_interrupt_gate(209, (unsigned long)x86_irq_177);
	install_interrupt_gate(210, (unsigned long)x86_irq_178);
	install_interrupt_gate(211, (unsigned long)x86_irq_179);
	install_interrupt_gate(212, (unsigned long)x86_irq_180);
	install_interrupt_gate(213, (unsigned long)x86_irq_181);
	install_interrupt_gate(214, (unsigned long)x86_irq_182);
	install_interrupt_gate(215, (unsigned long)x86_irq_183);
	install_interrupt_gate(216, (unsigned long)x86_irq_184);
	install_interrupt_gate(217, (unsigned long)x86_irq_185);
	install_interrupt_gate(218, (unsigned long)x86_irq_186);
	install_interrupt_gate(219, (unsigned long)x86_irq_187);
	install_interrupt_gate(220, (unsigned long)x86_irq_188);
	install_interrupt_gate(221, (unsigned long)x86_irq_189);
	install_interrupt_gate(222, (unsigned long)x86_irq_190);
	install_interrupt_gate(223, (unsigned long)x86_irq_191);
	install_interrupt_gate(224, (unsigned long)x86_irq_192);
	install_interrupt_gate(225, (unsigned long)x86_irq_193);
	install_interrupt_gate(226, (unsigned long)x86_irq_194);
	install_interrupt_gate(227, (unsigned long)x86_irq_195);
	install_interrupt_gate(228, (unsigned long)x86_irq_196);
	install_interrupt_gate(229, (unsigned long)x86_irq_197);
	install_interrupt_gate(230, (unsigned long)x86_irq_198);
	install_interrupt_gate(231, (unsigned long)x86_irq_199);
	install_interrupt_gate(232, (unsigned long)x86_irq_200);
	install_interrupt_gate(233, (unsigned long)x86_irq_201);
	install_interrupt_gate(234, (unsigned long)x86_irq_202);
	install_interrupt_gate(235, (unsigned long)x86_irq_203);
	install_interrupt_gate(236, (unsigned long)x86_irq_204);
	install_interrupt_gate(237, (unsigned long)x86_irq_205);
	install_interrupt_gate(238, (unsigned long)x86_irq_206);
	install_interrupt_gate(239, (unsigned long)x86_irq_207);
	install_interrupt_gate(240, (unsigned long)x86_irq_208);
	install_interrupt_gate(241, (unsigned long)x86_irq_209);
	install_interrupt_gate(242, (unsigned long)x86_irq_210);
	install_interrupt_gate(243, (unsigned long)x86_irq_211);
	install_interrupt_gate(244, (unsigned long)x86_irq_212);
	install_interrupt_gate(245, (unsigned long)x86_irq_213);
	install_interrupt_gate(246, (unsigned long)x86_irq_214);
	install_interrupt_gate(247, (unsigned long)x86_irq_215);
	install_interrupt_gate(248, (unsigned long)x86_irq_216);
	install_interrupt_gate(249, (unsigned long)x86_irq_217);
	install_interrupt_gate(250, (unsigned long)x86_irq_218);
	install_interrupt_gate(251, (unsigned long)x86_irq_219);
	install_interrupt_gate(252, (unsigned long)x86_irq_220);
	install_interrupt_gate(253, (unsigned long)x86_irq_221);
	install_interrupt_gate(254, (unsigned long)x86_irq_222);
	install_interrupt_gate(255, (unsigned long)x86_irq_223);
}
