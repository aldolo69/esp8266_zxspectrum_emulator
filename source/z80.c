/** Z80: portable Z80 emulator *******************************/
/**                                                         **/
/**                           Z80.c                         **/
/**                                                         **/
/** This file contains implementation for Z80 CPU. Don't    **/
/** forget to provide RdZ80(), WrZ80(), InZ80(), OutZ80(),  **/
/** LoopZ80(), and PatchZ80() functions to accomodate the   **/
/** emulated machine's architecture.                        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2007                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include<Arduino.h>
#include "z80.h"
#include "Tables.h"
#include <pgmspace.h>
#include "Zxsound.h"
/** INLINE ***************************************************/
/** C99 standard has "inline", but older compilers used     **/
/** __inline for the same purpose.                          **/
/*************************************************************/
#ifdef __C99__
#define INLINE static inline
#else
#define INLINE static __inline
#endif

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/

#define RdZ80 RDZ80
#define WrZ80 WRZ80
INLINE byte RdZ80(word16 A)        {
  //return (A < ROMSIZE ?  ROM[A]  : RAM[A - ROMSIZE]);
  return (A < ROMSIZE ? pgm_read_byte(ROM + A) : RAM[A - ROMSIZE]);
}
INLINE void WrZ80(word16 A, byte V) {
  if (A >= ROMSIZE && A < (ROMSIZE + RAMSIZE)) RAM[A - ROMSIZE] = V;
}


 

void OutZ80(register word16 Port, register byte Value)
{
  if ((Port & 0xFF) == 0xFE) {
    // BorderColor und Ton-Out speichern
    //    out_ram=Value;
    // Ton-Signal schalten
    zxDisplayBorderSet(Value & 7);

    if ( Value & 0x10 ) {
      zxSoundSet(1); \
    }
    else {
      zxSoundSet(0);
    }
  }
  if (Port == 0xAAAA) {
    // 1bit Digital-OUT [Test von UB]
    //    ZX_Spectrum.led_mode=1;
    if ((Value & 0x01) == 0) {
      //      LED_RED_PORT->BSRRH = LED_RED_PIN; // Bit auf Lo
    }
    else {
      //      LED_RED_PORT->BSRRL = LED_RED_PIN; // Bit auf Hi
    }
  }
}


byte InZ80(register word16 Port)
{
  byte ret_wert = 0xFF;


  //zxDisplayWriteSerial(Port);


  if ((Port & 0xFF) == 0xFE) {
    // Abfrage der Tastatur und Ton-IN
    if (!(Port & 0x0100)) ret_wert &= KEY[0];
    if (!(Port & 0x0200)) ret_wert &= KEY[1];
    if (!(Port & 0x0400)) ret_wert &= KEY[2];
    if (!(Port & 0x0800)) ret_wert &= KEY[3];
    if (!(Port & 0x1000)) ret_wert &= KEY[4];
    if (!(Port & 0x2000)) ret_wert &= KEY[5];
    if (!(Port & 0x4000)) ret_wert &= KEY[6];
    if (!(Port & 0x8000)) ret_wert &= KEY[7];
  }
  else if ((Port & 0xFF) == 0x1F) {
    // Abfrage vom Kempston-Joystick
   ret_wert=KEMPSTONJOYSTICK;
  }
  else {
    ret_wert = 0x01;
  }


  return (ret_wert);
}


void PatchZ80(register Z80 *R)
{
  // nothing to do
}



/** FAST_RDOP ************************************************/
/** With this #define not present, RdZ80() should perform   **/
/** the functions of OpZ80().                               **/
/*************************************************************/
#ifndef FAST_RDOP
#define OpZ80(A) RdZ80(A)
#endif

#define S(Fl)        R->AF.B.l|=Fl
#define R(Fl)        R->AF.B.l&=~(Fl)
#define FLAGS(Rg,Fl) R->AF.B.l=Fl|ZSTable[Rg]

#define M_RLC(Rg)      \
  R->AF.B.l=Rg>>7;Rg=(Rg<<1)|R->AF.B.l;R->AF.B.l|=PZSTable[Rg]
#define M_RRC(Rg)      \
  R->AF.B.l=Rg&0x01;Rg=(Rg>>1)|(R->AF.B.l<<7);R->AF.B.l|=PZSTable[Rg]
#define M_RL(Rg)       \
  if(Rg&0x80)          \
  {                    \
    Rg=(Rg<<1)|(R->AF.B.l&C_FLAG); \
    R->AF.B.l=PZSTable[Rg]|C_FLAG; \
  }                    \
  else                 \
  {                    \
    Rg=(Rg<<1)|(R->AF.B.l&C_FLAG); \
    R->AF.B.l=PZSTable[Rg];        \
  }
#define M_RR(Rg)       \
  if(Rg&0x01)          \
  {                    \
    Rg=(Rg>>1)|(R->AF.B.l<<7);     \
    R->AF.B.l=PZSTable[Rg]|C_FLAG; \
  }                    \
  else                 \
  {                    \
    Rg=(Rg>>1)|(R->AF.B.l<<7);     \
    R->AF.B.l=PZSTable[Rg];        \
  }

#define M_SLA(Rg)      \
  R->AF.B.l=Rg>>7;Rg<<=1;R->AF.B.l|=PZSTable[Rg]
#define M_SRA(Rg)      \
  R->AF.B.l=Rg&C_FLAG;Rg=(Rg>>1)|(Rg&0x80);R->AF.B.l|=PZSTable[Rg]

#define M_SLL(Rg)      \
  R->AF.B.l=Rg>>7;Rg=(Rg<<1)|0x01;R->AF.B.l|=PZSTable[Rg]
#define M_SRL(Rg)      \
  R->AF.B.l=Rg&0x01;Rg>>=1;R->AF.B.l|=PZSTable[Rg]

#define M_BIT(Bit,Rg)  \
  R->AF.B.l=(R->AF.B.l&C_FLAG)|H_FLAG|PZSTable[Rg&(1<<Bit)]

#define M_SET(Bit,Rg) Rg|=1<<Bit
#define M_RES(Bit,Rg) Rg&=~(1<<Bit)

#define M_POP(Rg)      \
  R->Rg.B.l=OpZ80(R->SP.W++);R->Rg.B.h=OpZ80(R->SP.W++)
#define M_PUSH(Rg)     \
  WrZ80(--R->SP.W,R->Rg.B.h);WrZ80(--R->SP.W,R->Rg.B.l)

#define M_CALL         \
  J.B.l=OpZ80(R->PC.W++);J.B.h=OpZ80(R->PC.W++);         \
  WrZ80(--R->SP.W,R->PC.B.h);WrZ80(--R->SP.W,R->PC.B.l); \
  R->PC.W=J.W; \
  JumpZ80(J.W)

#define M_JP  J.B.l=OpZ80(R->PC.W++);J.B.h=OpZ80(R->PC.W);R->PC.W=J.W;JumpZ80(J.W)
#define M_JR  R->PC.W+=(offset)OpZ80(R->PC.W)+1;JumpZ80(R->PC.W)
#define M_RET R->PC.B.l=OpZ80(R->SP.W++);R->PC.B.h=OpZ80(R->SP.W++);JumpZ80(R->PC.W)

#define M_RST(Ad)      \
  WrZ80(--R->SP.W,R->PC.B.h);WrZ80(--R->SP.W,R->PC.B.l);R->PC.W=Ad;JumpZ80(Ad)

#define M_LDWORD(Rg)   \
  R->Rg.B.l=OpZ80(R->PC.W++);R->Rg.B.h=OpZ80(R->PC.W++)

#define M_ADD(Rg)      \
  J.W=R->AF.B.h+Rg;    \
  R->AF.B.l=           \
                       (~(R->AF.B.h^Rg)&(Rg^J.B.l)&0x80? V_FLAG:0)| \
                       J.B.h|ZSTable[J.B.l]|                        \
                       ((R->AF.B.h^Rg^J.B.l)&H_FLAG);               \
  R->AF.B.h=J.B.l

#define M_SUB(Rg)      \
  J.W=R->AF.B.h-Rg;    \
  R->AF.B.l=           \
                       ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
                       N_FLAG|-J.B.h|ZSTable[J.B.l]|                      \
                       ((R->AF.B.h^Rg^J.B.l)&H_FLAG);                     \
  R->AF.B.h=J.B.l

#define M_ADC(Rg)      \
  J.W=R->AF.B.h+Rg+(R->AF.B.l&C_FLAG); \
  R->AF.B.l=                           \
                                       (~(R->AF.B.h^Rg)&(Rg^J.B.l)&0x80? V_FLAG:0)| \
                                       J.B.h|ZSTable[J.B.l]|              \
                                       ((R->AF.B.h^Rg^J.B.l)&H_FLAG);     \
  R->AF.B.h=J.B.l

#define M_SBC(Rg)      \
  J.W=R->AF.B.h-Rg-(R->AF.B.l&C_FLAG); \
  R->AF.B.l=                           \
                                       ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
                                       N_FLAG|-J.B.h|ZSTable[J.B.l]|      \
                                       ((R->AF.B.h^Rg^J.B.l)&H_FLAG);     \
  R->AF.B.h=J.B.l

#define M_CP(Rg)       \
  J.W=R->AF.B.h-Rg;    \
  R->AF.B.l=           \
                       ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
                       N_FLAG|-J.B.h|ZSTable[J.B.l]|                      \
                       ((R->AF.B.h^Rg^J.B.l)&H_FLAG)

#define M_AND(Rg) R->AF.B.h&=Rg;R->AF.B.l=H_FLAG|PZSTable[R->AF.B.h]
#define M_OR(Rg)  R->AF.B.h|=Rg;R->AF.B.l=PZSTable[R->AF.B.h]
#define M_XOR(Rg) R->AF.B.h^=Rg;R->AF.B.l=PZSTable[R->AF.B.h]

#define M_IN(Rg)        \
  Rg=InZ80(R->BC.W);  \
  R->AF.B.l=PZSTable[Rg]|(R->AF.B.l&C_FLAG)

#define M_INC(Rg)       \
  Rg++;                 \
  R->AF.B.l=            \
                        (R->AF.B.l&C_FLAG)|ZSTable[Rg]|           \
                        (Rg==0x80? V_FLAG:0)|(Rg&0x0F? 0:H_FLAG)

#define M_DEC(Rg)       \
  Rg--;                 \
  R->AF.B.l=            \
                        N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[Rg]| \
                        (Rg==0x7F? V_FLAG:0)|((Rg&0x0F)==0x0F? H_FLAG:0)

#define M_ADDW(Rg1,Rg2) \
  J.W=(R->Rg1.W+R->Rg2.W)&0xFFFF;                        \
  R->AF.B.l=                                             \
      (R->AF.B.l&~(H_FLAG|N_FLAG|C_FLAG))|                 \
      ((R->Rg1.W^R->Rg2.W^J.W)&0x1000? H_FLAG:0)|          \
      (((long)R->Rg1.W+(long)R->Rg2.W)&0x10000? C_FLAG:0); \
  R->Rg1.W=J.W

#define M_ADCW(Rg)      \
  I=R->AF.B.l&C_FLAG;J.W=(R->HL.W+R->Rg.W+I)&0xFFFF;           \
  R->AF.B.l=                                                   \
      (((long)R->HL.W+(long)R->Rg.W+(long)I)&0x10000? C_FLAG:0)| \
      (~(R->HL.W^R->Rg.W)&(R->Rg.W^J.W)&0x8000? V_FLAG:0)|       \
      ((R->HL.W^R->Rg.W^J.W)&0x1000? H_FLAG:0)|                  \
      (J.W? 0:Z_FLAG)|(J.B.h&S_FLAG);                            \
  R->HL.W=J.W

#define M_SBCW(Rg)      \
  I=R->AF.B.l&C_FLAG;J.W=(R->HL.W-R->Rg.W-I)&0xFFFF;           \
  R->AF.B.l=                                                   \
      N_FLAG|                                                    \
      (((long)R->HL.W-(long)R->Rg.W-(long)I)&0x10000? C_FLAG:0)| \
      ((R->HL.W^R->Rg.W)&(R->HL.W^J.W)&0x8000? V_FLAG:0)|        \
      ((R->HL.W^R->Rg.W^J.W)&0x1000? H_FLAG:0)|                  \
      (J.W? 0:Z_FLAG)|(J.B.h&S_FLAG);                            \
  R->HL.W=J.W

enum Codes
{
  NOP, LD_BC_WORD, LD_xBC_A, INC_BC, INC_B, DEC_B, LD_B_BYTE, RLCA,
  EX_AF_AF, ADD_HL_BC, LD_A_xBC, DEC_BC, INC_C, DEC_C, LD_C_BYTE, RRCA,
  DJNZ, LD_DE_WORD, LD_xDE_A, INC_DE, INC_D, DEC_D, LD_D_BYTE, RLA,
  JR, ADD_HL_DE, LD_A_xDE, DEC_DE, INC_E, DEC_E, LD_E_BYTE, RRA,
  JR_NZ, LD_HL_WORD, LD_xWORD_HL, INC_HL, INC_H, DEC_H, LD_H_BYTE, DAA,
  JR_Z, ADD_HL_HL, LD_HL_xWORD, DEC_HL, INC_L, DEC_L, LD_L_BYTE, CPL,
  JR_NC, LD_SP_WORD, LD_xWORD_A, INC_SP, INC_xHL, DEC_xHL, LD_xHL_BYTE, SCF,
  JR_C, ADD_HL_SP, LD_A_xWORD, DEC_SP, INC_A, DEC_A, LD_A_BYTE, CCF,
  LD_B_B, LD_B_C, LD_B_D, LD_B_E, LD_B_H, LD_B_L, LD_B_xHL, LD_B_A,
  LD_C_B, LD_C_C, LD_C_D, LD_C_E, LD_C_H, LD_C_L, LD_C_xHL, LD_C_A,
  LD_D_B, LD_D_C, LD_D_D, LD_D_E, LD_D_H, LD_D_L, LD_D_xHL, LD_D_A,
  LD_E_B, LD_E_C, LD_E_D, LD_E_E, LD_E_H, LD_E_L, LD_E_xHL, LD_E_A,
  LD_H_B, LD_H_C, LD_H_D, LD_H_E, LD_H_H, LD_H_L, LD_H_xHL, LD_H_A,
  LD_L_B, LD_L_C, LD_L_D, LD_L_E, LD_L_H, LD_L_L, LD_L_xHL, LD_L_A,
  LD_xHL_B, LD_xHL_C, LD_xHL_D, LD_xHL_E, LD_xHL_H, LD_xHL_L, HALT, LD_xHL_A,
  LD_A_B, LD_A_C, LD_A_D, LD_A_E, LD_A_H, LD_A_L, LD_A_xHL, LD_A_A,
  ADD_B, ADD_C, ADD_D, ADD_E, ADD_H, ADD_L, ADD_xHL, ADD_A,
  ADC_B, ADC_C, ADC_D, ADC_E, ADC_H, ADC_L, ADC_xHL, ADC_A,
  SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_xHL, SUB_A,
  SBC_B, SBC_C, SBC_D, SBC_E, SBC_H, SBC_L, SBC_xHL, SBC_A,
  AND_B, AND_C, AND_D, AND_E, AND_H, AND_L, AND_xHL, AND_A,
  XOR_B, XOR_C, XOR_D, XOR_E, XOR_H, XOR_L, XOR_xHL, XOR_A,
  OR_B, OR_C, OR_D, OR_E, OR_H, OR_L, OR_xHL, OR_A,
  CP_B, CP_C, CP_D, CP_E, CP_H, CP_L, CP_xHL, CP_A,
  RET_NZ, POP_BC, JP_NZ, JP, CALL_NZ, PUSH_BC, ADD_BYTE, RST00,
  RET_Z, RET, JP_Z, PFX_CB, CALL_Z, CALL, ADC_BYTE, RST08,
  RET_NC, POP_DE, JP_NC, OUTA, CALL_NC, PUSH_DE, SUB_BYTE, RST10,
  RET_C, EXX, JP_C, INA, CALL_C, PFX_DD, SBC_BYTE, RST18,
  RET_PO, POP_HL, JP_PO, EX_HL_xSP, CALL_PO, PUSH_HL, AND_BYTE, RST20,
  RET_PE, LD_PC_HL, JP_PE, EX_DE_HL, CALL_PE, PFX_ED, XOR_BYTE, RST28,
  RET_P, POP_AF, JP_P, DI, CALL_P, PUSH_AF, OR_BYTE, RST30,
  RET_M, LD_SP_HL, JP_M, EI, CALL_M, PFX_FD, CP_BYTE, RST38
};

enum CodesCB
{
  RLC_B, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_xHL, RLC_A,
  RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_xHL, RRC_A,
  RL_B, RL_C, RL_D, RL_E, RL_H, RL_L, RL_xHL, RL_A,
  RR_B, RR_C, RR_D, RR_E, RR_H, RR_L, RR_xHL, RR_A,
  SLA_B, SLA_C, SLA_D, SLA_E, SLA_H, SLA_L, SLA_xHL, SLA_A,
  SRA_B, SRA_C, SRA_D, SRA_E, SRA_H, SRA_L, SRA_xHL, SRA_A,
  SLL_B, SLL_C, SLL_D, SLL_E, SLL_H, SLL_L, SLL_xHL, SLL_A,
  SRL_B, SRL_C, SRL_D, SRL_E, SRL_H, SRL_L, SRL_xHL, SRL_A,
  BIT0_B, BIT0_C, BIT0_D, BIT0_E, BIT0_H, BIT0_L, BIT0_xHL, BIT0_A,
  BIT1_B, BIT1_C, BIT1_D, BIT1_E, BIT1_H, BIT1_L, BIT1_xHL, BIT1_A,
  BIT2_B, BIT2_C, BIT2_D, BIT2_E, BIT2_H, BIT2_L, BIT2_xHL, BIT2_A,
  BIT3_B, BIT3_C, BIT3_D, BIT3_E, BIT3_H, BIT3_L, BIT3_xHL, BIT3_A,
  BIT4_B, BIT4_C, BIT4_D, BIT4_E, BIT4_H, BIT4_L, BIT4_xHL, BIT4_A,
  BIT5_B, BIT5_C, BIT5_D, BIT5_E, BIT5_H, BIT5_L, BIT5_xHL, BIT5_A,
  BIT6_B, BIT6_C, BIT6_D, BIT6_E, BIT6_H, BIT6_L, BIT6_xHL, BIT6_A,
  BIT7_B, BIT7_C, BIT7_D, BIT7_E, BIT7_H, BIT7_L, BIT7_xHL, BIT7_A,
  RES0_B, RES0_C, RES0_D, RES0_E, RES0_H, RES0_L, RES0_xHL, RES0_A,
  RES1_B, RES1_C, RES1_D, RES1_E, RES1_H, RES1_L, RES1_xHL, RES1_A,
  RES2_B, RES2_C, RES2_D, RES2_E, RES2_H, RES2_L, RES2_xHL, RES2_A,
  RES3_B, RES3_C, RES3_D, RES3_E, RES3_H, RES3_L, RES3_xHL, RES3_A,
  RES4_B, RES4_C, RES4_D, RES4_E, RES4_H, RES4_L, RES4_xHL, RES4_A,
  RES5_B, RES5_C, RES5_D, RES5_E, RES5_H, RES5_L, RES5_xHL, RES5_A,
  RES6_B, RES6_C, RES6_D, RES6_E, RES6_H, RES6_L, RES6_xHL, RES6_A,
  RES7_B, RES7_C, RES7_D, RES7_E, RES7_H, RES7_L, RES7_xHL, RES7_A,
  SET0_B, SET0_C, SET0_D, SET0_E, SET0_H, SET0_L, SET0_xHL, SET0_A,
  SET1_B, SET1_C, SET1_D, SET1_E, SET1_H, SET1_L, SET1_xHL, SET1_A,
  SET2_B, SET2_C, SET2_D, SET2_E, SET2_H, SET2_L, SET2_xHL, SET2_A,
  SET3_B, SET3_C, SET3_D, SET3_E, SET3_H, SET3_L, SET3_xHL, SET3_A,
  SET4_B, SET4_C, SET4_D, SET4_E, SET4_H, SET4_L, SET4_xHL, SET4_A,
  SET5_B, SET5_C, SET5_D, SET5_E, SET5_H, SET5_L, SET5_xHL, SET5_A,
  SET6_B, SET6_C, SET6_D, SET6_E, SET6_H, SET6_L, SET6_xHL, SET6_A,
  SET7_B, SET7_C, SET7_D, SET7_E, SET7_H, SET7_L, SET7_xHL, SET7_A
};

enum CodesED
{
  DB_00, DB_01, DB_02, DB_03, DB_04, DB_05, DB_06, DB_07,
  DB_08, DB_09, DB_0A, DB_0B, DB_0C, DB_0D, DB_0E, DB_0F,
  DB_10, DB_11, DB_12, DB_13, DB_14, DB_15, DB_16, DB_17,
  DB_18, DB_19, DB_1A, DB_1B, DB_1C, DB_1D, DB_1E, DB_1F,
  DB_20, DB_21, DB_22, DB_23, DB_24, DB_25, DB_26, DB_27,
  DB_28, DB_29, DB_2A, DB_2B, DB_2C, DB_2D, DB_2E, DB_2F,
  DB_30, DB_31, DB_32, DB_33, DB_34, DB_35, DB_36, DB_37,
  DB_38, DB_39, DB_3A, DB_3B, DB_3C, DB_3D, DB_3E, DB_3F,
  IN_B_xC, OUT_xC_B, SBC_HL_BC, LD_xWORDe_BC, NEG, RETN, IM_0, LD_I_A,
  IN_C_xC, OUT_xC_C, ADC_HL_BC, LD_BC_xWORDe, DB_4C, RETI, DB_, LD_R_A,
  IN_D_xC, OUT_xC_D, SBC_HL_DE, LD_xWORDe_DE, DB_54, DB_55, IM_1, LD_A_I,
  IN_E_xC, OUT_xC_E, ADC_HL_DE, LD_DE_xWORDe, DB_5C, DB_5D, IM_2, LD_A_R,
  IN_H_xC, OUT_xC_H, SBC_HL_HL, LD_xWORDe_HL, DB_64, DB_65, DB_66, RRD,
  IN_L_xC, OUT_xC_L, ADC_HL_HL, LD_HL_xWORDe, DB_6C, DB_6D, DB_6E, RLD,
  IN_F_xC, DB_71, SBC_HL_SP, LD_xWORDe_SP, DB_74, DB_75, DB_76, DB_77,
  IN_A_xC, OUT_xC_A, ADC_HL_SP, LD_SP_xWORDe, DB_7C, DB_7D, DB_7E, DB_7F,
  DB_80, DB_81, DB_82, DB_83, DB_84, DB_85, DB_86, DB_87,
  DB_88, DB_89, DB_8A, DB_8B, DB_8C, DB_8D, DB_8E, DB_8F,
  DB_90, DB_91, DB_92, DB_93, DB_94, DB_95, DB_96, DB_97,
  DB_98, DB_99, DB_9A, DB_9B, DB_9C, DB_9D, DB_9E, DB_9F,
  LDI, CPI, INI, OUTI, DB_A4, DB_A5, DB_A6, DB_A7,
  LDD, CPD, IND, OUTD, DB_AC, DB_AD, DB_AE, DB_AF,
  LDIR, CPIR, INIR, OTIR, DB_B4, DB_B5, DB_B6, DB_B7,
  LDDR, CPDR, INDR, OTDR, DB_BC, DB_BD, DB_BE, DB_BF,
  DB_C0, DB_C1, DB_C2, DB_C3, DB_C4, DB_C5, DB_C6, DB_C7,
  DB_C8, DB_C9, DB_CA, DB_CB, DB_CC, DB_CD, DB_CE, DB_CF,
  DB_D0, DB_D1, DB_D2, DB_D3, DB_D4, DB_D5, DB_D6, DB_D7,
  DB_D8, DB_D9, DB_DA, DB_DB, DB_DC, DB_DD, DB_DE, DB_DF,
  DB_E0, DB_E1, DB_E2, DB_E3, DB_E4, DB_E5, DB_E6, DB_E7,
  DB_E8, DB_E9, DB_EA, DB_EB, DB_EC, DB_ED, DB_EE, DB_EF,
  DB_F0, DB_F1, DB_F2, DB_F3, DB_F4, DB_F5, DB_F6, DB_F7,
  DB_F8, DB_F9, DB_FA, DB_FB, DB_FC, DB_FD, DB_FE, DB_FF
};

static void CodesCB(register Z80 *R)
{
  register byte I;

  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesCB[I];
  switch (I)
  {
#include "CodesCB.h"
    default:
      if (R->TrapBadOps)
        printf
        (
          "[Z80 %lX] Unrecognized instruction: CB %02X at PC=%04X\n",
          (long)(R->User), OpZ80(R->PC.W - 1), R->PC.W - 2
        );
  }
}

static void CodesDDCB(register Z80 *R)
{
  register pair J;
  register byte I;

#define XX IX
  J.W = R->XX.W + (offset)OpZ80(R->PC.W++);
  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesXXCB[I];
  switch (I)
  {
#include "CodesXCB.h"
    default:
      if (R->TrapBadOps)
        printf
        (
          "[Z80 %lX] Unrecognized instruction: DD CB %02X %02X at PC=%04X\n",
          (long)(R->User), OpZ80(R->PC.W - 2), OpZ80(R->PC.W - 1), R->PC.W - 4
        );
  }
#undef XX
}

static void CodesFDCB(register Z80 *R)
{
  register pair J;
  register byte I;

#define XX IY
  J.W = R->XX.W + (offset)OpZ80(R->PC.W++);
  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesXXCB[I];
  switch (I)
  {
#include "CodesXCB.h"
    default:
      if (R->TrapBadOps)
        printf
        (
          "[Z80 %lX] Unrecognized instruction: FD CB %02X %02X at PC=%04X\n",
          (long)R->User, OpZ80(R->PC.W - 2), OpZ80(R->PC.W - 1), R->PC.W - 4
        );
  }
#undef XX
}

static void CodesED(register Z80 *R)
{
  register byte I;
  register pair J;

  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesED[I];
  switch (I)
  {
#include "CodesED.h"
    case PFX_ED:
      R->PC.W--; break;
    default:
      if (R->TrapBadOps)
        printf
        (
          "[Z80 %lX] Unrecognized instruction: ED %02X at PC=%04X\n",
          (long)R->User, OpZ80(R->PC.W - 1), R->PC.W - 2
        );
  }
}

static void CodesDD(register Z80 *R)
{
  register byte I;
  register pair J;

#define XX IX
  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesXX[I];
  switch (I)
  {
#include "CodesXX.h"
    case PFX_FD:
    case PFX_DD:
      R->PC.W--; break;
    case PFX_CB:
      CodesDDCB(R); break;
    default:
      if (R->TrapBadOps)
        printf
        (
          "[Z80 %lX] Unrecognized instruction: DD %02X at PC=%04X\n",
          (long)R->User, OpZ80(R->PC.W - 1), R->PC.W - 2
        );
  }
#undef XX
}

static void CodesFD(register Z80 *R)
{
  register byte I;
  register pair J;

#define XX IY
  I = OpZ80(R->PC.W++);
  R->ICount -= CyclesXX[I];
  switch (I)
  {
#include "CodesXX.h"
    case PFX_FD:
    case PFX_DD:
      R->PC.W--; break;
    case PFX_CB:
      CodesFDCB(R); break;
    default:
      printf
      (
        "Unrecognized instruction: FD %02X at PC=%04X\n",
        OpZ80(R->PC.W - 1), R->PC.W - 2
      );
  }
#undef XX
}

/** ResetZ80() ***********************************************/
/** This function can be used to reset the register struct  **/
/** before starting execution with Z80(). It sets the       **/
/** registers to their supposed initial values.             **/
/*************************************************************/
void ResetZ80(Z80 *R)
{
  R->PC.W     = 0x0000;
  R->SP.W     = 0xF000;
  R->AF.W     = 0x0000;
  R->BC.W     = 0x0000;
  R->DE.W     = 0x0000;
  R->HL.W     = 0x0000;
  R->AF1.W    = 0x0000;
  R->BC1.W    = 0x0000;
  R->DE1.W    = 0x0000;
  R->HL1.W    = 0x0000;
  R->IX.W     = 0x0000;
  R->IY.W     = 0x0000;
  R->I        = 0x00;
  R->R        = 0x00;
  R->IFF      = 0x00;
  R->ICount   = R->IPeriod;
  R->IRequest = INT_NONE;
  R->IBackup  = 0;

  JumpZ80(R->PC.W);
}





//typedef struct z80fileheader {
//    uint8_t _A;1
//    uint8_t _F;2
//    uint16_t _BC;34//(LSB, i.e. C, first)
//    uint16_t _HL;56
//    uint16_t _PC;78
//    uint16_t _SP;910
//    uint8_t _InterruptRegister;11
//    uint8_t _RefreshRegister;12
//    uint8_t _Flags1;13
//    uint16_t _DE;1415
//    uint16_t _BC_Dash;1617
//    uint16_t _DE_Dash;1819
//    uint16_t _HL_Dash;2021
//    uint8_t _A_Dash;22
//    uint8_t _F_Dash;23
//    uint16_t _IY;2425
//    uint16_t _IX;2627
//    uint8_t _InterruptFlipFlop;28
//    uint8_t _IFF2;
//    uint8_t _Flags2;
//  29      1       Bit 0-1: Interrupt mode (0, 1 or 2)
//                        Bit 2  : 1=Issue 2 emulation
//                        Bit 3  : 1=Double interrupt frequency
//                        Bit 4-5: 1=High video synchronisation
//                                 3=Low video synchronisation
//                                 0,2=Normal
//                        Bit 6-7: 0=Cursor/Protek/AGF joystick
//                                 1=Kempston joystick
//                                 2=Sinclair 2 Left joystick (or user
//                                   defined, for version 3 .z80 files)
//                                 3=Sinclair 2 Right joystick
//0x00, 0x50, 0x00, 0x00, 0xFF, 0x5C, 0x3E, 0x1F, 0xFA, 0x5F, 0x00, 0x00, 0x20, 0x04, 0x5D, 0x21,
//                                                                              EI          IM1

//jetpack
//0x7E, 0x00, 0x00, 0x04, 0xED, 0x55, 0x62, 0x71, 0xE6, 0x5C, 0x3F, 0x21, 0x21, 0x0D, 0x3E, 0xE2,
//0x00, 0x93, 0x62, 0xED, 0x5A, 0x47, 0xA8, 0x3A, 0x5C, 0x7A, 0x5C, 0x00, 0x00, 0x01,

//decathlon
//0xEF, 0xA8, 0xE9, 0x07, 0x60, 0x58, 0x38, 0x00, 0xF9, 0xFF, 0x3F, 0x66, 0x20, 0x00, 0x00, 0x00,
//0x40, 0x9B, 0x36, 0xFF, 0xFF, 0xD5, 0x09, 0x3A, 0x5C, 0x00, 0x69, 0x00, 0x00, 0x01,,


boolean SetZ80(Z80 *R, struct z80fileheader * header)
{

ResetZ80(R);

  const uint8_t *ptr;
  uint8_t wert1,wert2;
  uint8_t flag_version=0;
   
  uint16_t header_len;
  uint16_t akt_adr;
 
  // pointer auf Datenanfang setzen
  ptr=header;

  R->AF.B.h=*(ptr++); // A [0]
  R->AF.B.l=*(ptr++); // F [1]
  R->BC.B.l=*(ptr++); // C [2]
  R->BC.B.h=*(ptr++); // B [3]
  R->HL.B.l=*(ptr++); // L [4]
  R->HL.B.h=*(ptr++); // H [5]

  // PC [6+7]
  wert1=*(ptr++); 
  wert2=*(ptr++);
  R->PC.W=(wert2<<8)|wert1;
  if(R->PC.W==0x0000) {
    return false;

  } 

  // SP [8+9]
  wert1=*(ptr++);
  wert2=*(ptr++);
  R->SP.W=(wert2<<8)|wert1;

  R->I=*(ptr++); // I [10]
  R->R=*(ptr++); // R [11]
  
  // Comressed-Flag und Border [12]
  wert1=*(ptr++); 
  
  wert2=((wert1&0x0E)>>1);

  zxDisplayBorderSet(wert2);

  
  R->DE.B.l=*(ptr++); // E [13]
  R->DE.B.h=*(ptr++); // D [14]
  R->BC1.B.l=*(ptr++); // C1 [15]
  R->BC1.B.h=*(ptr++); // B1 [16]
  R->DE1.B.l=*(ptr++); // E1 [17]
  R->DE1.B.h=*(ptr++); // D1 [18]
  R->HL1.B.l=*(ptr++); // L1 [19]
  R->HL1.B.h=*(ptr++); // H1 [20]
  R->AF1.B.h=*(ptr++); // A1 [21]
  R->AF1.B.l=*(ptr++); // F1 [22]
  R->IY.B.l=*(ptr++); // Y [23]
  R->IY.B.h=*(ptr++); // I [24]
  R->IX.B.l=*(ptr++); // X [25]
  R->IX.B.h=*(ptr++); // I [26]

  // Interrupt-Flag [27]
  wert1=*(ptr++); 
  if(wert1!=0) {
    // EI
    R->IFF|=IFF_2|IFF_EI;
  }
  else {
    // DI
    R->IFF&=~(IFF_1|IFF_2|IFF_EI);
  }
  wert1=*(ptr++); // nc [28]
  // Interrupt-Mode [29]
  wert1=*(ptr++);  
  if((wert1&0x01)!=0) {
    R->IFF|=IFF_IM1;
  }
  else {
    R->IFF&=~IFF_IM1;
  }
  if((wert1&0x02)!=0) {
    R->IFF|=IFF_IM2;
  }
  else {
    R->IFF&=~IFF_IM2;
  }


 

  R->ICount   = R->IPeriod;
  R->IRequest = INT_NONE;
  R->IBackup  = 0;


return true;
}

/** ExecZ80() ************************************************/
/** This function will execute given number of Z80 cycles.  **/
/** It will then return the number of cycles left, possibly **/
/** negative, and current register values in R.             **/
/*************************************************************/
#ifdef EXECZ80
int ExecZ80(register Z80 *R, register int RunCycles)
{
  register byte I;
  register pair J;

  for (R->ICount = RunCycles;;)
  {

//z80 engine too fast at 160mhz. let's burn some cycle
//to slowdown
volatile int i=z80DelayCycle;
for(;i;) i--;



    
    while (R->ICount > 0)
    {
#ifdef DEBUG
      /* Turn tracing on when reached trap address */
      if (R->PC.W == R->Trap) R->Trace = 1;
      /* Call single-step debugger, exit if requested */
      if (R->Trace)
        if (!DebugZ80(R)) return (R->ICount);
#endif

      /* Read opcode and count cycles */
      I = OpZ80(R->PC.W++);
      /* Count cycles */
      R->ICount -= Cycles[I];

      /* Interpret opcode */
      switch (I)
      {
#include "Codes.h"
        case PFX_CB: CodesCB(R); break;
        case PFX_ED: CodesED(R); break;
        case PFX_FD: CodesFD(R); break;
        case PFX_DD: CodesDD(R); break;
      }
    }

    /* Unless we have come here after EI, exit */
    if (!(R->IFF & IFF_EI)) return (R->ICount);
    else
    {
      /* Done with AfterEI state */
      R->IFF = (R->IFF & ~IFF_EI) | IFF_1;
      /* Restore the ICount */
      R->ICount += R->IBackup - 1;
      /* Interrupt CPU if needed */
      if ((R->IRequest != INT_NONE) && (R->IRequest != INT_QUIT)) IntZ80(R, R->IRequest);
    }
  }
}
#endif /* EXECZ80 */

/** IntZ80() *************************************************/
/** This function will generate interrupt of given vector.  **/
/*************************************************************/
void IntZ80(Z80 *R, word16 Vector)
{
  /* If HALTed, take CPU off HALT instruction */
  if (R->IFF & IFF_HALT) {
    R->PC.W++;
    R->IFF &= ~IFF_HALT;
  }

  if ((R->IFF & IFF_1) || (Vector == INT_NMI))
  {
    /* Save PC on stack */
    M_PUSH(PC);

    /* Automatically reset IRequest if needed */
    if (R->IAutoReset && (Vector == R->IRequest)) R->IRequest = INT_NONE;

    /* If it is NMI... */
    if (Vector == INT_NMI)
    {
      /* Clear IFF1 */
      R->IFF &= ~(IFF_1 | IFF_EI);
      /* Jump to hardwired NMI vector */
      R->PC.W = 0x0066;
      JumpZ80(0x0066);
      /* Done */
      return;
    }

    /* Further interrupts off */
    R->IFF &= ~(IFF_1 | IFF_2 | IFF_EI);

    /* If in IM2 mode... */
    if (R->IFF & IFF_IM2)
    {
      /* Make up the vector address */
      Vector = (Vector & 0xFF) | ((word16)(R->I) << 8);
      /* Read the vector */
      R->PC.B.l = RdZ80(Vector++);
      R->PC.B.h = RdZ80(Vector);
      JumpZ80(R->PC.W);
      /* Done */
      return;
    }

    /* If in IM1 mode, just jump to hardwired IRQ vector */
    if (R->IFF & IFF_IM1) {
      R->PC.W = 0x0038;
      JumpZ80(0x0038);
      return;
    }

    /* If in IM0 mode... */

    /* Jump to a vector */
    switch (Vector)
    {
      case INT_RST00: R->PC.W = 0x0000; JumpZ80(0x0000); break;
      case INT_RST08: R->PC.W = 0x0008; JumpZ80(0x0008); break;
      case INT_RST10: R->PC.W = 0x0010; JumpZ80(0x0010); break;
      case INT_RST18: R->PC.W = 0x0018; JumpZ80(0x0018); break;
      case INT_RST20: R->PC.W = 0x0020; JumpZ80(0x0020); break;
      case INT_RST28: R->PC.W = 0x0028; JumpZ80(0x0028); break;
      case INT_RST30: R->PC.W = 0x0030; JumpZ80(0x0030); break;
      case INT_RST38: R->PC.W = 0x0038; JumpZ80(0x0038); break;
    }
  }
}

/** RunZ80() *************************************************/
/** This function will run Z80 code until an LoopZ80() call **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
#ifndef EXECZ80
word16 RunZ80(Z80 *R)
{
  register byte I;
  register pair J;

  for (;;)
  {
#ifdef DEBUG
    /* Turn tracing on when reached trap address */
    if (R->PC.W == R->Trap) R->Trace = 1;
    /* Call single-step debugger, exit if requested */
    if (R->Trace)
      if (!DebugZ80(R)) return (R->PC.W);
#endif

    I = OpZ80(R->PC.W++);
    R->ICount -= Cycles[I];

    switch (I)
    {
#include "Codes.h"
      case PFX_CB: CodesCB(R); break;
      case PFX_ED: CodesED(R); break;
      case PFX_FD: CodesFD(R); break;
      case PFX_DD: CodesDD(R); break;
    }

    /* If cycle counter expired... */
    if (R->ICount <= 0)
    {
      /* If we have come after EI, get address from IRequest */
      /* Otherwise, get it from the loop handler             */
      if (R->IFF & IFF_EI)
      {
        R->IFF = (R->IFF & ~IFF_EI) | IFF_1; /* Done with AfterEI state */
        R->ICount += R->IBackup - 1;   /* Restore the ICount      */

        /* Call periodic handler or set pending IRQ */
        if (R->ICount > 0) J.W = R->IRequest;
        else
        {
          J.W = LoopZ80(R);      /* Call periodic handler    */
          R->ICount += R->IPeriod; /* Reset the cycle counter  */
          if (J.W == INT_NONE) J.W = R->IRequest; /* Pending IRQ */
        }
      }
      else
      {
        J.W = LoopZ80(R);        /* Call periodic handler    */
        R->ICount += R->IPeriod; /* Reset the cycle counter  */
        if (J.W == INT_NONE) J.W = R->IRequest; /* Pending IRQ */
      }

      if (J.W == INT_QUIT) return (R->PC.W); /* Exit if INT_QUIT */
      if (J.W != INT_NONE) IntZ80(R, J.W); /* Int-pt if needed */
    }
  }

  /* Execution stopped */
  return (R->PC.W);
}
#endif /* !EXECZ80 */
