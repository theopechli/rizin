// SPDX-FileCopyrightText: 2012-2015 pancake <pancake@nopcode.org>
// SPDX-FileCopyrightText: 2012-2015 Fedor Sakharov <fedor.sakharov@gmail.com>
// SPDX-FileCopyrightText: 2012-2015 Bhootravi <ravi2809@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#include <string.h>
#include <rz_types.h>
#include <rz_lib.h>
#include <rz_asm.h>
#include <rz_analysis.h>
#include <rz_util.h>

#include <h8300_disas.h>

#define emit(frag) rz_strbuf_appendf(&op->esil, frag)
#define emitf(...) rz_strbuf_appendf(&op->esil, __VA_ARGS__)
// setting the appropriate flags, NOTE: semicolon included
#define setZ      rz_strbuf_appendf(&op->esil, ",$z,Z,:=") // zero flag
#define setN      rz_strbuf_appendf(&op->esil, ",15,$s,N,=") // negative(sign) flag
#define setV(val) rz_strbuf_appendf(&op->esil, ",%s,V,=", val) // overflow flag
#define setC_B    rz_strbuf_appendf(&op->esil, ",7,$c,C,:=") // carry flag for byte op
#define setC_W    rz_strbuf_appendf(&op->esil, ",15,$c,C,:=") // carryflag for word op
#define setCb_B   rz_strbuf_appendf(&op->esil, ",7,$b,C,:=") // borrow flag for byte
#define setCb_W   rz_strbuf_appendf(&op->esil, ",15,$b,C,:=") // borrow flag for word
#define setH_B    rz_strbuf_appendf(&op->esil, ",3,$c,H,:=") // half carry(byte)-bcd
#define setH_W    rz_strbuf_appendf(&op->esil, ",11,$c,H,:=") // half carry(word)-bcd
#define setHb_B   rz_strbuf_appendf(&op->esil, ",3,$b,H,:=") // half borrow(byte)-bcd
#define setHb_W   rz_strbuf_appendf(&op->esil, ",11,$b,H,:=") // halfborrow(word)-bcd

// get reg. from opcodes
#define rs()  (buf[1] & 0x70) >> 4 // upper nibble used as source generally
#define rsB() (buf[1] & 0x70) >> 4, buf[1] & 0x80 ? 'l' : 'h' // msb of nibble used for l/h
#define rd()  buf[1] & 0x07 // lower nibble used as dest generally
// a for compact instrs. (some instrs. have rd in 1st byte, others in 2nd byte)
#define rdB(a) buf[a] & 0x07, buf[a] & 0x8 ? 'l' : 'h'
// work around for z flag
// internally r=0xff on incr. stored as 0x100, which doesn't raise the z flag
// NOTE - use the mask and setZ at last, mask will affect other flags
#define mask()   rz_strbuf_appendf(&op->esil, ",0xffff,r%u,&=", rd());
#define maskB(a) rz_strbuf_appendf(&op->esil, ",0xff,r%u%c,&=", rdB(a));

// immediate values are always 2nd byte
#define imm buf[1]
/*
 * Reference Manual:
 *
https://www.classes.cs.uchicago.edu/archive/2006/winter/23000-1/docs/h8300.pdf
 */

static void h8300_analysis_jmp(RzAnalysisOp *op, ut64 addr, const ut8 *buf) {
	switch (buf[0]) {
	case H8300_JMP_1:
		op->type = RZ_ANALYSIS_OP_TYPE_UJMP;
		break;
	case H8300_JMP_2:
		op->type = RZ_ANALYSIS_OP_TYPE_JMP;
		op->jump = rz_read_at_be16(buf, 2);
		break;
	case H8300_JMP_3:
		op->type = RZ_ANALYSIS_OP_TYPE_UJMP;
		op->jump = buf[1];
		break;
	}
}

static void h8300_analysis_jsr(RzAnalysisOp *op, ut64 addr, const ut8 *buf) {
	switch (buf[0]) {
	case H8300_JSR_1:
		op->type = RZ_ANALYSIS_OP_TYPE_UCALL;
		break;
	case H8300_JSR_2:
		op->type = RZ_ANALYSIS_OP_TYPE_CALL;
		op->jump = rz_read_at_be16(buf, 2);
		op->fail = addr + 4;
		break;
	case H8300_JSR_3:
		op->type = RZ_ANALYSIS_OP_TYPE_UCALL;
		op->jump = buf[1];
		break;
	}
}

static int analop_esil(RzAnalysis *a, RzAnalysisOp *op, ut64 addr, const ut8 *buf) {
	int ret = -1;
	ut8 opcode = buf[0];
	if (!op) {
		return 2;
	}
	rz_strbuf_init(&op->esil);
	rz_strbuf_set(&op->esil, "");
	switch (opcode >> 4) {
	case H8300_CMP_4BIT:
		// acc. to manual this is how it's done, could use == in esil
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,-", imm, rdB(0));
		// setZ
		setV("%o");
		setN;
		setHb_B;
		setCb_B;
		maskB(0);
		setZ;
		return 0;
	case H8300_OR_4BIT:
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,|=", imm, rdB(0));
		// setZ
		setV("0");
		setN;
		maskB(0);
		setZ;
		return 0;
	case H8300_XOR_4BIT:
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,^=", imm, rdB(0));
		// setZ
		setN;
		setV("0");
		maskB(0);
		setZ;
		return 0;
	case H8300_AND_4BIT:
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,&=", imm, rdB(0));
		// setZ
		setN;
		setV("0");
		maskB(0);
		setZ;
		return 0;
	case H8300_ADD_4BIT:
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,+=", imm, rdB(0));
		// setZ
		setV("%o");
		setN;
		setH_B;
		setC_B;
		maskB(0);
		setZ;
		return 0;
	case H8300_ADDX_4BIT:
		rz_strbuf_appendf(&op->esil, "0x%02x,C,+,r%u%c,+= ", imm, rdB(0));
		// setZ
		setV("%o");
		setN;
		setH_B;
		setC_B;
		maskB(0);
		setZ;
		return 0;
	case H8300_SUBX_4BIT:
		// Rd – imm – C → Rd
		rz_strbuf_appendf(&op->esil, "0x%02x,r%u%c,-=,C,r%u%c,-=", imm, rdB(0), rdB(0));
		// setZ
		setV("%o");
		setN;
		setHb_B;
		setCb_B;
		maskB(0);
		setZ;
		return 0;
	case H8300_MOV_4BIT_2: /*TODO*/
	case H8300_MOV_4BIT_3: /*TODO*/
	case H8300_MOV_4BIT: /*TODO*/
		return 0;
	default:
		break;
	};

	switch (opcode) {
	case H8300_NOP:
		rz_strbuf_set(&op->esil, ",");
		return 0;
	case H8300_SLEEP: /* TODO */
		return 0;
	case H8300_STC:
		rz_strbuf_appendf(&op->esil, "ccr,r%u%c,=", rdB(1));
		return 0;
	case H8300_LDC:
		rz_strbuf_appendf(&op->esil, "r%u%c,ccr,=", rdB(1));
		return 0;
	case H8300_ORC:
		rz_strbuf_appendf(&op->esil, "0x%02x,ccr,|=", imm);
		return 0;
	case H8300_XORC:
		rz_strbuf_appendf(&op->esil, "0x%02x,ccr,^=", imm);
		return 0;
	case H8300_ANDC:
		rz_strbuf_appendf(&op->esil, "0x%02x,ccr,&=", imm);
		return 0;
	case H8300_LDC_2:
		rz_strbuf_appendf(&op->esil, "0x%02x,ccr,=", imm);
		return 0;
	case H8300_ADDB_DIRECT:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,+=", rsB(), rdB(1));
		setH_B;
		setV("%o");
		setC_B;
		setN;
		// setZ;
		maskB(1);
		setZ;
		return 0;
	case H8300_ADDW_DIRECT:
		rz_strbuf_appendf(&op->esil, "r%u,r%u,+=", rs(), rd());
		setH_W;
		setV("%o");
		setC_W;
		setN;
		mask();
		setZ;
		return 0;
	case H8300_INC:
		rz_strbuf_appendf(&op->esil, "1,r%u%c,+=", rdB(1));
		// setZ
		setV("%o");
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_ADDS:
		rz_strbuf_appendf(&op->esil, "%d,r%u,+=",
			((buf[1] & 0xf0) == 0x80) ? 2 : 1, rd());
		return 0;
	case H8300_MOV_1:
		/*TODO check if flags are set internally or not*/
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,=", rsB(), rdB(1));
		// setZ
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_MOV_2:
		rz_strbuf_appendf(&op->esil, "r%u,r%u,=", rs(), rd());
		// setZ
		setN;
		mask();
		setZ;
		return 0;
	case H8300_ADDX:
		// Rd + (Rs) + C → Rd
		rz_strbuf_appendf(&op->esil, "r%u%c,C,+,r%u%c,+=",
			rsB(), rdB(1));
		// setZ
		setV("%o");
		setN;
		setH_B;
		setC_B;
		maskB(1);
		setZ;
		return 0;
	case H8300_DAA: /*TODO*/
		return 0;
	case H8300_SHL: /*TODO*/
		return 0;
	case H8300_SHR: /*TODO*/
		return 0;
	case H8300_ROTL: /*TODO*/
		return 0;
	case H8300_ROTR: /*TODO*/
		return 0;
	case H8300_OR:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,|=", rsB(), rdB(1));
		// setZ
		setV("0");
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_XOR:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,^=", rsB(), rdB(1));
		// setZ
		setV("0");
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_AND:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,&=", rsB(), rdB(1));
		// setZ
		setV("0");
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_NOT_NEG:
		if ((buf[1] & 0xf0) == 0x80) { // NEG
			rz_strbuf_appendf(&op->esil, "r%u%c,0,-,r%u%c,=", rdB(1), rdB(1));
			// setZ
			setHb_B;
			setV("%o");
			setCb_B;
			setN;
			maskB(1);
			setZ;
		} else if ((buf[1] & 0xf0) == 0x00) { // NOT
			rz_strbuf_appendf(&op->esil, "r%u%c,!=", rdB(1));
			// setZ
			setV("0");
			setN;
			maskB(1);
			setZ;
		}
		return 0;
	case H8300_SUB_1:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,-=", rsB(), rdB(1));
		// setZ
		setHb_B;
		setV("%o");
		setCb_B;
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_SUBW:
		rz_strbuf_appendf(&op->esil, "r%u,r%u,-=", rs(), rd());
		setHb_W;
		setV("%o");
		setCb_W;
		setN;
		mask();
		setZ;
		return 0;
	case H8300_DEC:
		rz_strbuf_appendf(&op->esil, "1,r%u%c,-=", rdB(1));
		// setZ
		setV("%o");
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_SUBS:
		rz_strbuf_appendf(&op->esil, "%d,r%u,-=",
			((buf[1] & 0xf0) == 0x80) ? 2 : 1, rd());
		return 0;
	case H8300_CMP_1:
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,-", rsB(), rdB(1));
		// setZ
		setHb_B;
		setV("%o");
		setCb_B;
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_CMP_2:
		rz_strbuf_appendf(&op->esil, "r%u,r%u,-", rs(), rd());
		// setZ
		setHb_W;
		setV("%o");
		setCb_W;
		setN;
		mask();
		setZ;
		return 0;
	case H8300_SUBX:
		// Rd – (Rs) – C → Rd
		rz_strbuf_appendf(&op->esil, "r%u%c,r%u%c,-=,C,r%u%c,-=",
			rsB(), rdB(1), rdB(1));
		// setZ
		setHb_B;
		setV("%o");
		setCb_B;
		setN;
		maskB(1);
		setZ;
		return 0;
	case H8300_DAS: /*TODO*/
		return 0;
	case H8300_BRA:
		rz_strbuf_appendf(&op->esil, "0x%02x,pc,+=", buf[1]);
		return 0;
	case H8300_BRN:
		rz_strbuf_appendf(&op->esil, ",");
		return 0;
	case H8300_BHI:
		rz_strbuf_appendf(&op->esil, "C,Z,|,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BLS:
		rz_strbuf_appendf(&op->esil, "C,Z,|,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BCC:
		rz_strbuf_appendf(&op->esil, "C,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BCS:
		rz_strbuf_appendf(&op->esil, "C,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BNE:
		rz_strbuf_appendf(&op->esil, "Z,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BEQ:
		rz_strbuf_appendf(&op->esil, "Z,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BVC:
		rz_strbuf_appendf(&op->esil, "V,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BVS:
		rz_strbuf_appendf(&op->esil, "V,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BPL:
		rz_strbuf_appendf(&op->esil, "N,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BMI:
		rz_strbuf_appendf(&op->esil, "N,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BGE:
		rz_strbuf_appendf(&op->esil, "N,V,^,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BLT:
		rz_strbuf_appendf(&op->esil, "N,V,^,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BGT:
		rz_strbuf_appendf(&op->esil, "Z,N,V,^,|,!,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_BLE:
		rz_strbuf_appendf(&op->esil, "Z,N,V,^,|,?{0x%02x,pc,+=}", buf[1]);
		return 0;
	case H8300_MULXU:
		// Refer to pg. 100 of the manual linked at the beginning
		rz_strbuf_appendf(&op->esil, "r%u%c,r%ul,*,r%u,=",
			rsB(), rd(), rd());
		return 0;
	case H8300_DIVXU: /*TODO*/ return 0;
	case H8300_RTS: /*TODO*/ return 0;
	case H8300_BSR: /*TODO*/ return 0;
	case H8300_RTE: /*TODO*/ return 0;
	case H8300_JMP_1: /*TODO*/ return 0;
	case H8300_JMP_2: /*TODO*/ return 0;
	case H8300_JMP_3: /*TODO*/ return 0;
	case H8300_JSR_1: /*TODO*/ return 0;
	case H8300_JSR_2: /*TODO*/ return 0;
	case H8300_JSR_3: /*TODO*/
		return 0;
		// NOTE - cases marked with TODO have mem. access also(not impl.)
	case H8300_BSET_1: /*TODO*/
		// set rs&0x7th bit of rd. expr.- rd|= 1<<(rs&0x07)
		rz_strbuf_appendf(&op->esil, "0x7,r%u%c,&,1,<<,r%u%c,|=", rsB(), rdB(1));
		return 0;
	case H8300_BNOT_1: /*TODO*/
		// invert rs&0x7th bit of rd. expr.- rd^= 1<<(rs&0x07)
		rz_strbuf_appendf(&op->esil, "0x07,r%u%c,&,1,<<,r%u%c,^=", rsB(), rdB(1));
		return 0;
	case H8300_BCLR_R2R8: /*TODO*/
		// clear rs&0x7th bit of rd. expr.- rd&= !(1<<(rs&0x07))
		rz_strbuf_appendf(&op->esil, "0x7,r%u%c,&,1,<<,!,r%u%c,&=", rsB(), rdB(1));
		return 0;
	case H8300_BTST_R2R8: /*TODO*/
		//¬ (<Bit No.> of <EAd>) → Z, extract bit value and shift it back
		rz_strbuf_appendf(&op->esil, "0x7,r%u%c,&,0x7,r%u%c,&,1,<<,r%u%c,&,>>,!,Z,=",
			rsB(), rsB(), rdB(1));
		return 0;
	case H8300_BST_BIST: /*TODO*/
		if (!(buf[1] & 0x80)) { // BST
			rz_strbuf_appendf(&op->esil, "%d,C,<<,r%u%c,|=", rs(), rdB(1));
		} else { // BIST
			rz_strbuf_appendf(&op->esil, "%d,C,!,<<,r%u%c,|=", rs(), rdB(1));
		}
		return 0;
	case H8300_MOV_R82IND16: /*TODO*/ return 0;
	case H8300_MOV_IND162R16: /*TODO*/ return 0;
	case H8300_MOV_R82ABS16: /*TODO*/ return 0;
	case H8300_MOV_ABS162R16: /*TODO*/ return 0;
	case H8300_MOV_R82RDEC16: /*TODO*/ return 0;
	case H8300_MOV_INDINC162R16: /*TODO*/ return 0;
	case H8300_MOV_R82DISPR16: /*TODO*/ return 0;
	case H8300_MOV_DISP162R16: /*TODO*/
		return 0;
	case H8300_BSET_2: /*TODO*/
		// set imm bit of rd. expr.- rd|= (1<<imm)
		rz_strbuf_appendf(&op->esil, "%d,1,<<,r%u%c,|=", rs(), rdB(1));
		return 0;
	case H8300_BNOT_2: /*TODO*/
		// inv. imm bit of rd. expr.- rd^= (1<<imm)
		rz_strbuf_appendf(&op->esil, "%d,1,<<,r%u%c,^=", rs(), rdB(1));
		return 0;
	case H8300_BCLR_IMM2R8:
		// clear imm bit of rd. expr.- rd&= !(1<<imm)
		rz_strbuf_appendf(&op->esil, "%d,1,<<,!,r%u%c,&=", rs(), rdB(1));
		return 0;
	case H8300_BTST: /*TODO*/
		// see BTST above
		rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,!,Z,=",
			rs(), rs(), rdB(1));
		return 0;
	case H8300_BOR_BIOR: /*TODO*/
		if (!(buf[1] & 0x80)) { // BOR
			// C|=(rd&(1<<imm))>>imm
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,C,|=",
				rs(), rs(), rdB(1));
		} else { // BIOR
			// C|=!(rd&(1<<imm))>>imm
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,!,C,|=",
				rs(), rs(), rdB(1));
		}
		return 0;
	case H8300_BXOR_BIXOR: /*TODO*/
		if (!(buf[1] & 0x80)) { // BXOR
			// C^=(rd&(1<<imm))>>imm
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,C,^=",
				rs(), rs(), rdB(1));
		} else { // BIXOR
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,!,C,^=",
				rs(), rs(), rdB(1));
		}
		return 0;
	case H8300_BAND_BIAND:
		/*TODO check functionality*/
		// C&=(rd&(1<<imm))>>imm
		if (!(buf[1] & 0x80)) { // BAND
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,C,&=",
				rs(), rs(), rdB(1));
		} else { // BIAND
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,!,C,&=",
				rs(), rs(), rdB(1));
		}
		return 0;
	case H8300_BILD_IMM2R8:
		/*TODO*/
		if (!(buf[1] & 0x80)) { // BLD
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,C,=",
				rs(), rs(), rdB(1));
		} else { // BILD
			rz_strbuf_appendf(&op->esil, "%d,%d,1,<<,r%u%c,&,>>,!,C,=",
				rs(), rs(), rdB(1));
		}
		return 0;
	case H8300_MOV_IMM162R16: /*TODO*/ return 0;
	case H8300_EEPMOV: /*TODO*/ return 0;
	case H8300_BIAND_IMM2IND16: /*TODO*/ return 0;
	case H8300_BCLR_R2IND16: /*TODO*/ return 0;
	case H8300_BIAND_IMM2ABS8: /*TODO*/ return 0;
	case H8300_BCLR_R2ABS8: /*TODO*/ return 0;
	default:
		break;
	};

	return ret;
}

static int h8300_op(RzAnalysis *analysis, RzAnalysisOp *op, ut64 addr,
	const ut8 *buf, int len, RzAnalysisOpMask mask) {
	int ret;
	ut8 opcode = buf[0];
	struct h8300_cmd cmd;

	if (!op) {
		return 2;
	}

	op->addr = addr;
	ret = op->size = h8300_decode_command(buf, &cmd);

	if (ret < 0) {
		return ret;
	}
	switch (opcode >> 4) {
	case H8300_MOV_4BIT_2:
	case H8300_MOV_4BIT_3:
	case H8300_MOV_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_MOV;
		break;
	case H8300_CMP_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_CMP;
		break;
	case H8300_XOR_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_XOR;
		break;
	case H8300_AND_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_AND;
		break;
	case H8300_ADD_4BIT:
	case H8300_ADDX_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_AND;
		break;
	case H8300_SUBX_4BIT:
		op->type = RZ_ANALYSIS_OP_TYPE_SUB;
		break;
	default:
		op->type = RZ_ANALYSIS_OP_TYPE_UNK;
		break;
	};

	if (op->type != RZ_ANALYSIS_OP_TYPE_UNK) {
		analop_esil(analysis, op, addr, buf);
		return ret;
	}
	switch (opcode) {
	case H8300_MOV_R82IND16:
	case H8300_MOV_IND162R16:
	case H8300_MOV_R82ABS16:
	case H8300_MOV_ABS162R16:
	case H8300_MOV_R82RDEC16:
	case H8300_MOV_INDINC162R16:
	case H8300_MOV_R82DISPR16:
	case H8300_MOV_DISP162R16:
	case H8300_MOV_IMM162R16:
	case H8300_MOV_1:
	case H8300_MOV_2:
	case H8300_EEPMOV:
		op->type = RZ_ANALYSIS_OP_TYPE_MOV;
		break;
	case H8300_RTS:
		op->type = RZ_ANALYSIS_OP_TYPE_RET;
		break;
	case H8300_CMP_1:
	case H8300_CMP_2:
	case H8300_BTST_R2R8:
	case H8300_BTST:
		op->type = RZ_ANALYSIS_OP_TYPE_CMP;
		break;
	case H8300_SHL:
		op->type = RZ_ANALYSIS_OP_TYPE_SHL;
		break;
	case H8300_SHR:
		op->type = RZ_ANALYSIS_OP_TYPE_SHR;
		break;
	case H8300_XOR:
	case H8300_XORC:
		op->type = RZ_ANALYSIS_OP_TYPE_XOR;
		break;
	case H8300_MULXU:
		op->type = RZ_ANALYSIS_OP_TYPE_MUL;
		break;
	case H8300_ANDC:
		op->type = RZ_ANALYSIS_OP_TYPE_AND;
		break;
	case H8300_ADDB_DIRECT:
	case H8300_ADDW_DIRECT:
	case H8300_ADDS:
	case H8300_ADDX:
		op->type = RZ_ANALYSIS_OP_TYPE_ADD;
		break;
	case H8300_SUB_1:
	case H8300_SUBW:
	case H8300_SUBS:
	case H8300_SUBX:
		op->type = RZ_ANALYSIS_OP_TYPE_SUB;
		break;
	case H8300_NOP:
		op->type = RZ_ANALYSIS_OP_TYPE_NOP;
		break;
	case H8300_JSR_1:
	case H8300_JSR_2:
	case H8300_JSR_3:
		h8300_analysis_jsr(op, addr, buf);
		break;
	case H8300_JMP_1:
	case H8300_JMP_2:
	case H8300_JMP_3:
		h8300_analysis_jmp(op, addr, buf);
		break;
	case H8300_BRA:
	case H8300_BRN:
	case H8300_BHI:
	case H8300_BLS:
	case H8300_BCC:
	case H8300_BCS:
	case H8300_BNE:
	case H8300_BEQ:
	case H8300_BVC:
	case H8300_BVS:
	case H8300_BPL:
	case H8300_BMI:
	case H8300_BGE:
	case H8300_BLT:
	case H8300_BGT:
	case H8300_BLE:
		op->type = RZ_ANALYSIS_OP_TYPE_CJMP;
		op->jump = addr + 2 + (st8)(buf[1]);
		op->fail = addr + 2;
		break;
	default:
		op->type = RZ_ANALYSIS_OP_TYPE_UNK;
		break;
	};
	if (mask & RZ_ANALYSIS_OP_MASK_ESIL) {
		analop_esil(analysis, op, addr, buf);
	}
	return ret;
}

static char *get_reg_profile(RzAnalysis *analysis) {
	char *p =
		"=PC	pc\n"
		"=SP	r7\n"
		"=A0	r0\n"
		"gpr	r0	.16	0	0\n"
		"gpr	r0h	.8	0	0\n"
		"gpr	r0l	.8	1	0\n"
		"gpr	r1	.16	2	0\n"
		"gpr	r1h	.8	2	0\n"
		"gpr	r1l	.8	3	0\n"
		"gpr	r2	.16	4	0\n"
		"gpr	r2h	.8	4	0\n"
		"gpr	r2l	.8	5	0\n"
		"gpr	r3	.16	6	0\n"
		"gpr	r3h	.8	6	0\n"
		"gpr	r3l	.8	7	0\n"
		"gpr	r4	.16	8	0\n"
		"gpr	r4h	.8	8	0\n"
		"gpr	r4l	.8	9	0\n"
		"gpr	r5	.16	10	0\n"
		"gpr	r5h	.8	10	0\n"
		"gpr	r5l	.8	11	0\n"
		"gpr	r6	.16	12	0\n"
		"gpr	r6h	.8	12	0\n"
		"gpr	r6l	.8	13	0\n"
		"gpr	r7	.16	14	0\n"
		"gpr	r7h	.8	14	0\n"
		"gpr	r7l	.8	15	0\n"
		"gpr	pc	.16	16	0\n"
		"gpr	ccr	.8	18	0\n"
		"gpr	I	.1	.151	0\n"
		"gpr	U1	.1	.150	0\n"
		"gpr	H	.1	.149	0\n"
		"gpr	U2	.1	.148	0\n"
		"gpr	N	.1	.147	0\n"
		"gpr	Z	.1	.146	0\n"
		"gpr	V	.1	.145	0\n"
		"gpr	C	.1	.144	0\n";
	return strdup(p);
}

RzAnalysisPlugin rz_analysis_plugin_h8300 = {
	.name = "h8300",
	.desc = "H8300 code analysis plugin",
	.license = "LGPL3",
	.arch = "h8300",
	.bits = 16,
	.op = &h8300_op,
	.esil = true,
	.get_reg_profile = get_reg_profile,
};

#ifndef RZ_PLUGIN_INCORE
struct rz_lib_struct_t rizin_plugin = {
	.type = RZ_LIB_TYPE_ANALYSIS,
	.data = &rz_analysis_plugin_h8300,
	.version = RZ_VERSION
};
#endif
