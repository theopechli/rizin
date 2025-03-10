// SPDX-FileCopyrightText: 2013-2017 pancake <pancake@nopcode.org>
// SPDX-License-Identifier: LGPL-3.0-only
// r2 -Desil ls

#include <rz_debug.h>

#if 0
static int is_io_esil(RzDebug *dbg) {
	RzIODesc *d = dbg->iob.io->desc;
	if (d && d->plugin && d->plugin->name)
		if (!strcmp ("esil", d->plugin->name))
			return true;
	return false;
}
#endif

static int __esil_step_over(RzDebug *dbg) {
	eprintf("TODO: ESIL STEP OVER\n");
	return true;
}

static int __esil_step(RzDebug *dbg) {
	int oplen;
	ut8 buf[64];
	ut64 pc = 0LL; // getreg("pc")
	RzAnalysisOp op = { 0 };

	rz_debug_reg_sync(dbg, RZ_REG_TYPE_GPR, false);
	pc = rz_debug_reg_get(dbg, "PC");
	eprintf("PC = 0x%" PFMT64x "\n", pc);
	/// XXX. hack to trick vaddr issue
	// pc = 0x100001478;
	// memset (buf, 0, sizeof (buf));
	dbg->iob.read_at(dbg->iob.io, pc, buf, 64);
	eprintf("READ 0x%08" PFMT64x " %02x %02x %02x\n", pc, buf[0], buf[1], buf[2]);
	oplen = rz_analysis_op(dbg->analysis, &op, pc, buf, sizeof(buf), RZ_ANALYSIS_OP_MASK_ESIL);
	if (oplen > 0) {
		if (*RZ_STRBUF_SAFEGET(&op.esil)) {
			eprintf("ESIL: %s\n", RZ_STRBUF_SAFEGET(&op.esil));
			rz_analysis_esil_parse(dbg->analysis->esil, RZ_STRBUF_SAFEGET(&op.esil));
		}
	}
	rz_analysis_op_fini(&op);
	eprintf("TODO: ESIL STEP\n");
	return true;
}

static bool __esil_init(RzDebug *dbg, void **user) {
	dbg->tid = dbg->pid = 1;
	// aeim
	// aei
	return true;
}

static int __esil_continue(RzDebug *dbg, int pid, int tid, int sig) {
	eprintf("TODO continue\n");
	return true;
}

static int __esil_continue_syscall(RzDebug *dbg, int pid, int num) {
	eprintf("TODO: esil continue until syscall\n");
	return true;
}

static RzDebugReasonType __esil_wait(RzDebug *dbg, int pid) {
	/* do nothing */
	return RZ_DEBUG_REASON_NONE;
}

static int __esil_attach(RzDebug *dbg, int pid) {
	eprintf("OK attach\n");
	return true;
#if 0
	if (!is_io_esil (dbg))
		return false;
#endif
#if 0
	RzIOBdescbg *o;
	o = dbg->iob.io->desc->data;
eprintf ("base = %llx\n", o->bfvm->base);
eprintf ("screen = %llx\n", o->bfvm->screen);
eprintf ("input = %llx\n", o->bfvm->input);
#endif
	return true;
}

static int __esil_detach(RzDebug *dbg, int pid) {
	// reset vm?
	return true;
}

static char *__esil_reg_profile(RzDebug *dbg) {
	if (!strcmp(dbg->arch, "bf")) {
		return strdup(
			"=PC	pc\n"
			"=SP	esp\n"
			"=BP	ptr\n"
			"=A0	rax\n"
			"gpr	rax	.32	0	0\n"
			"gpr	pc	.32	0	0\n"
			"gpr	ptr	.32	4	0\n"
			"gpr	esp	.32	8	0\n"
			"gpr	scr	.32	12	0\n"
			"gpr	scri	.32	16	0\n"
			"gpr	inp	.32	20	0\n"
			"gpr	inpi	.32	24	0\n"
			"gpr	mem	.32	28	0\n"
			"gpr	memi	.32	32	0\n");
	}
	return rz_analysis_get_reg_profile(dbg->analysis);
}

static int __esil_breakpoint(RzBreakpoint *bp, RzBreakpointItem *b, bool set) {
	// rz_io_system (dbg->iob.io, "db");
	return false;
}

static bool __esil_kill(RzDebug *dbg, int pid, int tid, int sig) {
	// TODO: ESIL reset
	return true;
}

static int __esil_stop(RzDebug *dbg) {
	eprintf("ESIL: stop\n");
	return true;
}

static int __reg_read(RzDebug *dbg, int type, ut8 *buf, int size) {
	int sz;
	/* do nothing */
	ut8 *bytes = rz_reg_get_bytes(dbg->analysis->reg, type, &sz);
	memcpy(buf, bytes, RZ_MIN(size, sz));
	free(bytes);
	return size;
}

RzDebugPlugin rz_debug_plugin_esil = {
	.name = "esil",
	.license = "LGPL3",
	.arch = "any", // TODO: exception!
	.bits = RZ_SYS_BITS_32 | RZ_SYS_BITS_64,
	.init = __esil_init,
	.step = __esil_step,
	.step_over = __esil_step_over,
	.cont = __esil_continue,
	.contsc = __esil_continue_syscall,
	.attach = &__esil_attach,
	.detach = &__esil_detach,
	.wait = &__esil_wait,
	.stop = __esil_stop,
	.kill = __esil_kill,
	.breakpoint = __esil_breakpoint,
	.reg_profile = __esil_reg_profile,
	.reg_read = __reg_read,
};

#ifndef RZ_PLUGIN_INCORE
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_DBG,
	.data = &rz_debug_plugin_esil,
	.version = RZ_VERSION
};
#endif
