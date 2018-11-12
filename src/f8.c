/*
	This file is part of FreeChaF.

	FreeChaF is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FreeChaF is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeChaF.  If not, see http://www.gnu.org/licenses/
*/

// http://www.nyx.net/~lturner/public_html/F8_ins.html
// http://www.nyx.net/~lturner/public_html/Fairchild_F8.html
// http://channelf.se/veswiki/index.php?title=Main_Page
// http://seanriddle.com/chanfinfo.html

#include "f8.h"
#include "memory.h"
#include "ports.h"

int R[64]; // 64 byte Scratchpad

int A   = 0; // Accumulator
int PC0 = 0; // Program Counter
int PC1 = 0; // Program Counter alternate
int DC0 = 0; // Data Counter
int DC1 = 0; // Data Counter alternate
int ISAR= 0; // Indirect Scratchpad Address Register (6-bit)
int W   = 0; // Status Register (flags)

int (*OpCodes[0x100])(int);

// Flags
#define flag_Sign 0
#define flag_Zero 2
#define flag_Overflow 3
#define flag_Carry 1
#define flag_Interupt 4

/* *****************************
   *
   *  Helper functions
   *
   ***************************** */


// Read 1-byte instruction operand
int readOperand8(void)
{
	PC0++;
	return Memory[PC0-1];
}
// Read 2-byte instruction operand
int readOperand16(void)
{
	int val = (Memory[PC0]<<8) | Memory[PC0+1];
	PC0+=2;
	return val;
}

// Set specific flag
void setFlag(int flag, int val)
{
	W = W | 1<<flag;
	W = W ^ 1<<flag;
	W = W | (val>0)<<flag;
	W ^= (flag==0); // Compliment sign flag (1-positive, 0-negative)
}

// Set zero and sign flags based on val, clear overflow and carry 
void setFlags_0z0s(int val) // O Z C S
{
	setFlag(flag_Overflow, 0);
	setFlag(flag_Zero, (val==0));
	setFlag(flag_Carry, 0);
	setFlag(flag_Sign, (val & 0x80)>0);
}

// Clear all flags
void clearFlags_ozcs(void)
{
	setFlag(flag_Overflow, 0);
	setFlag(flag_Sign, 0);
	setFlag(flag_Carry, 0);
	setFlag(flag_Zero, 1);
}

// Increment Indirect Scratchpad Address Register
// affects just lower three bits, which roll-over 
void incISAR(void)
{
	ISAR = (ISAR&0x38) | ((ISAR+1)&0x7);
}

// Decrement Indirect Scratchpad Address Register
// affects just lower three bits, which roll-over
void decISAR(void)
{
	ISAR = (ISAR&0x38) | ((ISAR-1)&0x7);
}

// Increment Primary Data Counter
void incDC0(void)
{
	DC0 = (DC0+1) & 0xFFFF;
}

// Read Scratchpad Byte
int Read8(int reg)
{
	return (R[reg & 0x3F]) & 0xFF;
}
// Read 16-bit int from Scratchpad
int Read16(int reg)
{
	return ((R[reg&0x3F]<<8) | R[(reg+1)&0x3F]) & 0xFFFF;
}
// Write 16-bit int to Scratchpad
void Store16(int reg, int val)
{
	R[reg&0x3F] = (val>>8) & 0xFF;
	R[(reg+1)&0x3F]=val & 0xFF;
}

// Add two 8-bit signed ints
int Add8(int a, int b)
{
	int signa = a & 0x80;
	int signb = b & 0x80;
	int result = a + b;
	int signr = result & 0x80;

	setFlag(flag_Sign, (result & 0x80)>0);
	setFlag(flag_Zero, ((result & 0xFF)==0));
	setFlag(flag_Overflow, (signa==signb && signa!=signr)); 
	setFlag(flag_Carry, (result & 0x100)>0);
	return result & 0xFF;
}

// Add two 8-bit BCD numbers
int AddBCD(int a, int b)
{
	// Method from 6-3 of the Fairchild F8 Guide to Programming
	// assume a = (a + 0x66) & 0xFF
	int sum = a + b;

	int ci = ((a&0xF)+(b&0xF))>0xF; // carry intermediate
	int cu = sum>=0x100; // carry upper

	Add8(a, b);

	if(ci==0) { sum = (sum&0xF0) | ((sum+0xA)&0xF); }
	if(cu==0) { sum = (sum+0xA0); }

	return sum & 0xFF;
}

// Subtract two 8-bit signed ints
int Sub8(int a, int b)
{
	b = ((b ^ 0xFF) + 1);
	return Add8(a, b); 
}

// Bitwise And, sets flags
int And8(int a, int b)
{
	a = a & b;
	setFlags_0z0s(a);
	return a & 0xFF;
}
// Bitwise Or, sets flags
int Or8(int a, int b)
{
	a = a | b;
	setFlags_0z0s(a);
	return a & 0xFF;
}
// Bitwise Xor, sets flags
int Xor8(int a, int b)
{
	a = (a ^ b) & 0xFF;
	setFlags_0z0s(a);
	return a & 0xFF;
}
// Logical Shift Right, sets flags
int ShiftRight(int val, int dist)
{
	val = (val >> dist)  & 0xFF;
	setFlags_0z0s(val);
	return val;
}
// Logical Shift Left, sets flags
int ShiftLeft(int val, int dist)
{
	val = (val << dist) & 0xFF;
	setFlags_0z0s(val);
	return val & 0xFF;
}
// Computes relative branch offset
int calcBranch(int n)
{
	if((n&0x80)==0) { return(n-1); } // forward
	return -(((n-1)^0xFF)+1); // backward
	
}

/* *****************************
   *
   *  Opcode functions
   *
   ***************************** */

int LR_A_Ku(int v) // 00 LR A, Ku : A <- R12
{
	A = R[12];
	return 2;
}

int LR_A_Kl(int v) // 01 LR A, Kl : A <- R13
{
	A = R[13];
	return 2;
}

int LR_A_Qu(int v) // 02 LR A, Qu : A <- R14
{
	A = R[14];
	return 2;
}

int LR_A_Ql(int v) // 03 LR A, Ql : A <- R15 
{
	A = R[15];
	return 2;
} 

int LR_Ku_A(int v) // 04 LR Ku, A : R12 <- A 
{
	R[12] = A;
	return 2;
}

int LR_Kl_A(int v) // 05 LR Kl, A : R13 <- A 
{
	R[13] = A;
	return 2;
} 

int LR_Qu_A(int v) // 06 LR Qu, A : R14 <- A
{
	R[14] = A;
	return 2;
} 

int LR_Ql_A(int v) // 07 LR Ql, A : R15 <- A 
{
	R[15] = A;
	return 2;
} 

int LR_K_P(int v) // 08 LR K, P  : R12 <- PC1U, R13 <- PC1L
{
	Store16(12, PC1);
	return 8;
}

int LR_P_K(int v)  // 09 LR P, K  : PC1U <- R12, PC1L <- R13
{
	PC1 = Read16(12);
	return 8;
} 

int LR_A_IS(int v) // 0A LR A, IS : A <- ISAR 
{
	A = ISAR;
	return 2;
}

int LR_IS_A(int v) // 0B LR IS, A : ISAR <- A
{
	ISAR = A & 0x3F;
	return 2;
}

int PK(int v) // 0C PK PC1 <- PC0, PC0U <- R12, PC0L <- R13
{
	PC1 = PC0;
	PC0 = Read16(12);
	return 5;
}

int LR_P0_Q(int v) // 0D LR P0, Q : PC0L <- R15, PC0U <- R14
{
	PC0 = Read16(14);
	return 8;
}

int LR_Q_DC(int v) // 0E LR Q, DC : R14 <- DC0U, R15 <- DC0L 
{
	Store16(14, DC0);
	return 8;
}

int LR_DC_Q(int v) // 0F LR DC, Q : DC0U <- R14, DC0L <- R15
{
	DC0 = Read16(14);
	return 8;
}

int LR_DC_H(int v) // 10 LR DC, H : DC0U <- R10, DC0L <- R11 
{
	DC0 = Read16(10);
	return 8; 
}

int LR_H_DC(int v) // 11 LR H, DC : R10 <- DC0U, R11 <- DC0L
{
	Store16(10, DC0);
	return 8;
} 

int SR_1(int v) // 12 SR 1 : A >> 1
{
	A = ShiftRight(A, 1);
	return 2;
} 

int SL_1(int v) // 13 SL 1 : A << 1
{
	A = ShiftLeft(A, 1);
	return 2;
} 

int SR_4(int v) // 14 SR 4 : A >> 4
{
	A = ShiftRight(A, 4);
	return 2;
}

int SL_4(int v) // 15 SL 4 : A << 4
{ 
	A = ShiftLeft(A, 4);
	return 2;
} 

int LM(int v) // 16 LM A <- (DC0), DC0 <- DC0 + 1
{
	A = Memory[DC0];
	DC0 = (DC0+1) & 0xFFFF;
	return 5;
} 

int ST(int v) // 17 ST (DC0) <- A, DC0 <- DC0 + 1 
{
	Memory[DC0] = A;
	DC0 = (DC0+1) & 0xFFFF;
	return 5;
}        

int COM(int v) // 18 COM A : A <- A XOR 0xFF                
{
	A = A ^ 0xFF;
	setFlags_0z0s(A);
	return 2;
}

int LNK(int v) // 19 LNK : A <- A + C                       
{
	A = Add8(A, (W>>flag_Carry)&1);
	return 2;
}

int DI(int v) // 1A DI : Disable Interupts                 
{
	setFlag(flag_Interupt, 0);
	return 2; 
}

int EI(int v) // 1B EI : Enable Interupts                  
{
	setFlag(flag_Interupt, 1);
	return 2;
}

int POP(int v) // 1C POP : PC0 <- PC1                       
{
	PC0 = PC1;
	return 4;
}

int LR_W_J(int v) // 1D LR W, J : W <- R9                      
{ 
	W = R[9];
	return 2;
}

int LR_J_W(int v) // 1E LR J, W : R9 <- W                      
{
	R[9] = W;
	return 4;
}

int INC(int v) // 1F INC : A <- A + 1                       
{
	A = Add8(A, 1);
	return 2;
} 

int LI_n(int v) // 20 LI n : A <- n                          
{
	A = readOperand8();
	return 5; 
} 

int NI_n(int v) // 21 NI n : A <- A AND n                    
{
	A = And8(A, readOperand8());
	return 5;
} 

int OI_n(int v) // 22 OI n : A <- A OR n                     
{
	A = Or8(A, readOperand8());
	return 5;
} 

int XI_n(int v)   // 23 XI n : A <- A XOR n                    
{
	A = Xor8(A, readOperand8());
	return 5;
} 

int AI_n(int v)   // 24 AI n : A <- A + n                      
{
	A = Add8(A, readOperand8());
	return 5;
}

int CI_n(int v)   // 25 CI n : n+!(A)+1 (n-A), Only set status 
{
	Sub8(readOperand8(), A);
	return 5;
} 

int IN_n(int v) // 26 IN n : Data Bus <- Port n, A <- Port n
{ 
	A = PORTS_read(readOperand8());
	setFlags_0z0s(A);
	return 8;
} 

int OUT_n(int v) // 27 OUT n : Data Bus <- Port n, Port n <- A
{
	PORTS_notify(readOperand8(), A);
	return 8;
} 

int PI_mn(int v) // 28 PI mn : A <- m, PC1 <- PC0+1, PC0L <- n, PC0U <- A 
{ 
	A = readOperand8();   // A <- m
	PC1 = PC0+1;          // PC1 <- PC0+1
	PC0 = readOperand8(); // PC0L <- n
	PC0 = PC0 | (A<<8);   // PC0U <- A
	return 13;
} 

int JMP_mn(int v) // 29 JMP mn : A <- m, PC0L <- n, PC0U <- A 
{
	A = readOperand8(); // A <- m
	PC0=readOperand8(); // PC0L <- n
	PC0 |= (A<<8);      // PC0U <- A
	return 11;
}

int DCI_mn(int v) // 2A DCI mn : DC0U <- m, PC0++, DC0L <- n, PC0++ 
{
	DC0=readOperand16();
	return 12;
} 

int NOP(int v) // 2B NOP
{
	return 2;
} 

int XDC(int v) // 2C DC0,DC1 <- DC1,DC0 
{
	DC0^=DC1;
	DC1^=DC0;
	DC0^=DC1;
	return 4;
}

int DS_r(int v) // 3x DS r <- (r)+0xFF, [decrease scratchpad byte]
{
	R[v&0xF] = Sub8(R[v&0xF], 1);
	return 3;
} 
int DS_r_S(int v) // DS r Indirect
{
	R[ISAR] = Sub8(R[ISAR], 1);
	return 3;
}
int DS_r_I(int v) // DS r Increment
{
	R[ISAR] = Sub8(R[ISAR], 1);
	incISAR();
	return 3;
}
int DS_r_D(int v) // DS r Decrement
{
	R[ISAR] = Sub8(R[ISAR], 1);
	decISAR();
	return 3;
}

int LR_A_r(int v) // 4x LR A, r : A <- r 
{
	A = R[v&0xF];
	return 2;
} 
int LR_A_r_S(int v) // LR A, r Indirect
{
	A = R[ISAR];
	return 2;
}
int LR_A_r_I(int v) // LR A, r Increment
{
	A = R[ISAR];
	incISAR();
	return 2;
}
int LR_A_r_D(int v) // LR A, r Decrement
{
	A = R[ISAR];
	decISAR();
	return 2;
}
	
int LR_r_A(int v)   // 5x LR r, A : r <- A
{
	R[v&0xF] = A;
	return 2;
} 
int LR_r_A_S(int v) // LR r, A Indirect
{
	R[ISAR] = A;
	return 2;
}
int LR_r_A_I(int v) // LR r, A Increment
{
	R[ISAR] = A;
	incISAR();
	return 2;
}
int LR_r_A_D(int v) // LR r, A Decrement
{
	R[ISAR] = A;
	decISAR();
	return 2;
}

int LISU_i(int v) // 6x LISU i : ISARU <- i
{
	ISAR = (ISAR & 0x07) | ((v&0x7)<<3);
	return 2;
} 

int LISL_i(int v) // 6x LISL i : ISARL <- i
{
	ISAR = (ISAR & 0x38) | (v&0x7);
	return 2;
}

int CLR(int v) // 70 CLR : A <- 0
{
	A = 0;
	return 2;
}

int LIS_i(int v) // 7x LIS i : A <- i 
{ 
	A = v&0xF;
	return 2;
} 

int BT_t_n(int v) // 8x BT t, n : 1000 0ttt nnnn nnnn : 
{
	// AND bitmask t with W, if result is not 0: PC0<-PC0+n+1
	int t = ((W) & (v&0x7))!=0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BP_n(int v)   // 81 BP n : branch if POSITIVE: PC0<-PC0+n+1 
{
	int t = ((W>>flag_Sign)&1)==1;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BC_n(int v)   // 82 BC n : branch if CARRY: PC0<-PC0+n+1
{
	int t = ((W>>flag_Carry)&1)==1;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BZ_n(int v)   // 84 BZ n : branch if ZERO: PC0<-PC0+n+1
{
	int t = ((W>>flag_Zero)&1)==1;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int AM(int v)  // 88 AM  : A <- A+(DC0), DC0++
{
	A = Add8(A, Memory[DC0]);
	incDC0();
	return 5;
} 

int AMD(int v) // 89 AMD : A <- A+(DC0) decimal adjusted, DC0++
{
	A = AddBCD(A, Memory[DC0]);
	incDC0();
	return 5;
} 

int NM(int v) // 8A NM  : A <- A AND (DC0), DC0+1
{
	A = And8(A, Memory[DC0]);
	incDC0();
	return 5;
}

int OM(int v) // 8B OM  : A <-  A OR (DC0), DC0+1
{
	A = Or8(A, Memory[DC0]);
	incDC0();
	return 5;
} 

int XM(int v) // 8C XM  : A <-  A OR (DC0), DC0+1
{
	A = Xor8(A, Memory[DC0]);
	incDC0();
	return 5;
}

int CM(int v) // 8D CM  : (DC0) - A, only set status, DC0+1
{
	Sub8(Memory[DC0], A);
	incDC0();
	return 5;
} 

int ADC(int v) // 8E ADC : DC0 <- DC0 + A
{ 
	DC0 += A+(0xFF00*(A>0x7F)); // A is signed
	DC0 &= 0xFFFF;
	return 5;
} 

int BR7_n(int v)   // 8F BR7 n : if ISARL != 7: PC0<-PC0+n+1 
{
	int t = (ISAR&0x7)!=7;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BR_n(int v) // 90 BR n : PC0 <- PC0+n+1
{ 
	int n = readOperand8();
	PC0 = PC0+calcBranch(n);
	return 7;
}

int BN_n(int v)   // 91 BN n : branch if NEGATIVE PC0<-PC0+n+1
{
	int t = ((W>>flag_Sign)&1)==0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BNC_n(int v) // 92 BNC n : branch if NO CARRY: PC0 <- PC0+n+1 
{
	int t = ((W>>flag_Carry)&1)==0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BF_i_n(int v) // 9x BF i, n : 1001 iiii nnnn nnnn 
{
	// AND bitmask i with W, if result is 0: PC0 <- PC0+n+1
	int t = ((W) & (v&0xF))==0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BNZ_n(int v)  // 94 BNZ n : branch if NOT ZERO: PC0 <- PC0+n+1 
{
	int t = ((W>>flag_Zero)&1)==0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int BNO_n(int v) // 98 BNO n : branch if NO OVERFLOW: PC0 <- PC0+n+1 
{
	int t = ((W>>flag_Overflow)&1)==0;
	int n = readOperand8();
	PC0 = PC0+calcBranch(n)*(t);
	return 6 + (t); // 3 - no jump,  3.5 - jump
}

int INS_i(int v)  // Ax INS_i : 1010 iiii : A <- (Port i)
{
	//  if i=2..15: Data Bus <- Port Address, A <- (Port i)
	A = PORTS_read(v&0xF);
	setFlags_0z0s(A);
	return 4 + 4*((v&0xF)>1); // 2 i=0..1, 4 i=2..15
}

int OUTS_i(int v) // Bx OUTS i : 1011 iiii : Port i <- A
{
	// if i=2..15: Data Bus <- Port Address, Port i <- A
	PORTS_notify(v&0xF, A);
	return 4 + 4*((v&0xF)>1); // 2 i=0..1, 4 i=2..15
}

int AS_r(int v) // Cx AS r : A <- A+(r) 
{
	A = Add8(A, R[v&0xF]);
	return 2;
} 
int AS_r_S(int v) // AS r Indirect
{
	A = Add8(A, R[ISAR]);
	return 2;
}
int AS_r_I(int v) // AS r Increment
{
	A = Add8(A, R[ISAR]);
	incISAR();
	return 2;
}
int AS_r_D(int v) // AS r Decrement
{
	A = Add8(A, R[ISAR]);
	decISAR();
	return 2;
}

int ASD_r(int v)   // Dx ASD r  : A <- A+(r) [BCD]
{
	A = AddBCD(A, R[v&0xF]);
	return 4;
}
int ASD_r_S(int v) // ASD r Indirect
{
	A = AddBCD(A, R[ISAR]); 
	return 4;
}
int ASD_r_I(int v) // ASD r Increment
{
	A = AddBCD(A, R[ISAR]);
	incISAR();
	return 4;
}
int ASD_r_D(int v) // ASD r Decrement
{
	A = AddBCD(A, R[ISAR]);
	decISAR();
	return 4;
}

int XS_r(int v) // Ex XS r   : A <- A XOR (r) 
{
	A = Xor8(A, R[v&0xF]);
	return 2;
} 
int XS_r_S(int v) // XS r Indirect
{
	A = Xor8(A, R[ISAR]);
	return 2;
}
int XS_r_I(int v) // XS r Increment
{
	A = Xor8(A, R[ISAR]);
	incISAR();
	return 2;
}
int XS_r_D(int v) // XS r Decrement
{
	A = Xor8(A, R[ISAR]);
	decISAR();
	return 2;
}

int NS_r(int v)  // Fx NS r   : A <- A AND (r) 
{
	A = And8(A, R[v&0xF]);
	return 2;
}
int NS_r_S(int v) // NS r Indirect
{
	A = And8(A, R[ISAR]);
	return 2;
}
int NS_r_I(int v) // NS r Increment
{
	A = And8(A, R[ISAR]);
	incISAR();
	return 2;
}
int NS_r_D(int v) // NS r Decrement
{
	A = And8(A, R[ISAR]);
	decISAR();
	return 2;
}


/* *****************************
   *
   *  Map Opcodes to functions
   *
   ***************************** */
void F8_init()
{
	int i;

	OpCodes[0x00] = LR_A_Ku; // 00 LR A, Ku : A <- R12
	OpCodes[0x01] = LR_A_Kl; // 01 LR A, Kl : A <- R13
	OpCodes[0x02] = LR_A_Qu; // 02 LR A, Qu : A <- R14
	OpCodes[0x03] = LR_A_Ql; // 03 LR A, Ql : A <- R15
	OpCodes[0x04] = LR_Ku_A; // 04 LR Ku, A : R12 <- A
	OpCodes[0x05] = LR_Kl_A; // 05 LR Kl, A : R13 <- A
	OpCodes[0x06] = LR_Qu_A; // 06 LR Qu, A : R14 <- A
	OpCodes[0x07] = LR_Ql_A; // 07 LR Ql, A : R15 <- A
	OpCodes[0x08] = LR_K_P;  // 08 LR K, P  : R12 <- PC1U, R13 <- PC1L 
	OpCodes[0x09] = LR_P_K;  // 09 LR P, K  : PC1U <- R12, PC1L <- R13
	OpCodes[0x0A] = LR_A_IS; // 0A LR A, IS : A <- ISAR
	OpCodes[0x0B] = LR_IS_A; // 0B LR IS, A : ISAR <- A
	OpCodes[0x0C] = PK;      // 0C PK       : PC1 <- PC0, PC0 <- Q
	OpCodes[0x0D] = LR_P0_Q; // 0D LR P0, Q : PC0L <- R15, PC0U <- R14 
	OpCodes[0x0E] = LR_Q_DC; // 0E LR Q, DC : R14 <- DC0U, R15 <- DC0L
	OpCodes[0x0F] = LR_DC_Q; // 0F LR DC, Q : DC0U <- R14, DC0L <- R15
	OpCodes[0x10] = LR_DC_H; // 10 LR DC, H : DC0U <- R10, DC0L <- R11 
	OpCodes[0x11] = LR_H_DC; // 11 LR H, DC : R10 <- DC0U, R11 <- DC0L
	OpCodes[0x12] = SR_1;    // 12 SR 1     : A >> 1
	OpCodes[0x13] = SL_1;    // 13 SL 1     : A << 1
	OpCodes[0x14] = SR_4;    // 14 SR 4     : A >> 4
	OpCodes[0x15] = SL_4;    // 15 SL 4     : A << 4
	OpCodes[0x16] = LM;      // 16 LM       : A <- (DC0), DC0 <- DC0 + 1
	OpCodes[0x17] = ST;      // 17 ST       : DC0 <- A, DC0 <- DC0 + 1 
	OpCodes[0x18] = COM;     // 18 COM A    : A <- A XOR 0xFF
	OpCodes[0x19] = LNK;     // 19 LNK      : A <- A + C 
	OpCodes[0x1A] = DI;      // 1A DI       : Disable Interupts
	OpCodes[0x1B] = EI;      // 1B EI       : Enable Interupts
	OpCodes[0x1C] = POP;     // 1C POP      : PC0 <- PC1, A destroyed 
	OpCodes[0x1D] = LR_W_J;  // 1D LR W, J  : W <- R9
	OpCodes[0x1E] = LR_J_W;  // 1E LR J, W  : R9 <- W
	OpCodes[0x1F] = INC;     // 1F INC      : A <- A + 1
	OpCodes[0x20] = LI_n;    // 20 LI n     : A <- n 
	OpCodes[0x21] = NI_n;    // 21 NI n     : A <- A AND n 
	OpCodes[0x22] = OI_n;    // 22 OI n     : A <- A OR n 
	OpCodes[0x23] = XI_n;    // 23 XI n     : A <- A XOR n 
	OpCodes[0x24] = AI_n;    // 24 AI n     : A <- A + n 
	OpCodes[0x25] = CI_n;    // 25 CI n     : n+!(A)+1 (n-A), Only set status
	OpCodes[0x26] = IN_n;    // 26 IN n     : Data Bus <- Port n, A <- Port n
	OpCodes[0x27] = OUT_n;   // 27 OUT n    : Data Bus <- Port n, Port n <- A
	OpCodes[0x28] = PI_mn;   // 28 PI mn    : A <- m, PC1 <- PC0+1, PC0 <- mn
	OpCodes[0x29] = JMP_mn;  // 29 JMP mn   : A <- m, PC0L <- n, PC0U <- (A) 
	OpCodes[0x2A] = DCI_mn;  // 2A DCI mn   : DC0U <- m, PC0++, DC0L <- n, PC0++ 
	OpCodes[0x2B] = NOP;     // 2B NOP
	OpCodes[0x2C] = XDC;     // 2C XDC      : DC0,DC1 <- DC1,DC0 
	OpCodes[0x2D] = NOP;     // 2D         ** bad opcode **
	OpCodes[0x2E] = NOP;     // 2E         ** bad opcode **
	OpCodes[0x2F] = NOP;     // 2F         ** bad opcode **
	
	for(i=0; i<12; i++)
	{
		OpCodes[0x30+i] = DS_r;    // 3x DS      : r <- (r)+0xFF,
		OpCodes[0x40+i] = LR_A_r;  // 4x LR A, r : A <- r
		OpCodes[0x50+i] = LR_r_A;  // 5x LR r, A : r <- A
	}

	OpCodes[0x3C] = DS_r_S;
	OpCodes[0x3D] = DS_r_I;
	OpCodes[0x3E] = DS_r_D;
	OpCodes[0x3F] = NOP;

	OpCodes[0x4C] = LR_A_r_S;
	OpCodes[0x4D] = LR_A_r_I;
	OpCodes[0x4E] = LR_A_r_D;
	OpCodes[0x4F] = NOP;

	OpCodes[0x5C] = LR_r_A_S;
	OpCodes[0x5D] = LR_r_A_I;
	OpCodes[0x5E] = LR_r_A_D;
	OpCodes[0x5F] = NOP;

	for(i=0; i<8; i++)
	{
		OpCodes[0x60+i] = LISU_i;  // 6x LISU i : ISARU <- i 
		OpCodes[0x68+i] = LISL_i;  // 6x LISL i : ISARL <- i
	}
	OpCodes[0x70] = CLR;     // 70 CLR    : A <- 0

	for(i=0; i<15; i++)
	{
		OpCodes[0x71+i] = LIS_i;   // 7x LIS i  : A <- i
	}
	OpCodes[0x80] = BT_t_n; // 8x BT t, n : bit test t with W
	OpCodes[0x81] = BP_n;   // 81 BP n    : branch if POSITIVE: PC0<-PC0+n+1
	OpCodes[0x82] = BC_n;   // 82 BC n    : branch if CARRY: PC0<-PC0+n+1
	OpCodes[0x83] = BT_t_n; 
	OpCodes[0x84] = BZ_n;   // 84 BZ n    : branch if ZERO: PC0<-PC0+n+1
	OpCodes[0x85] = BT_t_n;
	OpCodes[0x86] = BT_t_n;
	OpCodes[0x87] = BT_t_n;
	OpCodes[0x88] = AM;     // 88 AM      : A <- A+(DC0), DC0++
	OpCodes[0x89] = AMD;    // 89 AMD     : A <- A+(DC0) decimal adjusted, DC0++
	OpCodes[0x8A] = NM;     // 8A NM      : A <- A AND (DC0), DC0+1
	OpCodes[0x8B] = OM;     // 8B OM      : A <- A OR (DC0), DC0+1
	OpCodes[0x8C] = XM;     // 8C XM      : A <- A XOR (DC0), DC0+1 
	OpCodes[0x8D] = CM;     // 8D CM      : (DC0) - A, only set status, DC0+1
	OpCodes[0x8E] = ADC;    // 8E ADC     : DC0 <- DC0 + A
	OpCodes[0x8F] = BR7_n;  // 8F BR7 n   : if ISARL != 7: PC0 <- PC0+n+1 
	OpCodes[0x90] = BR_n;   // 90 BR n    : PC0 <- PC0+n+1
	OpCodes[0x91] = BN_n;   // 91 BN n    : branch if NEGATIVE: 
	OpCodes[0x92] = BNC_n;  // 92 BNC n   : branch if NO CARRY: 
	OpCodes[0x93] = BF_i_n; // 9x BF i, n : bit test i with W
	OpCodes[0x94] = BNZ_n;  // 94 BNZ n   : branch if NOT ZERO: 
	OpCodes[0x95] = BF_i_n;
	OpCodes[0x96] = BF_i_n;
	OpCodes[0x97] = BF_i_n;
	OpCodes[0x98] = BNO_n;  // 98 BNO n   : branch if NO OVERFLOW:
	OpCodes[0x99] = BF_i_n;
	OpCodes[0x9A] = BF_i_n;
	OpCodes[0x9B] = BF_i_n;
	OpCodes[0x9C] = BF_i_n;
	OpCodes[0x9D] = BF_i_n;
	OpCodes[0x9E] = BF_i_n;
	OpCodes[0x9F] = BF_i_n;

	for(i=0; i<16; i++)
	{
		OpCodes[0xA0+i] = INS_i;  // Ax INS_i  : read port (short)
		OpCodes[0xB0+i] = OUTS_i; // Bx OUTS i : write port (short)
		OpCodes[0xC0+i] = AS_r;   // Cx AS r   : A <- A+(r) 
		OpCodes[0xD0+i] = ASD_r;  // Dx ASD r  : A <- A+(r) [BCD]
		OpCodes[0xE0+i] = XS_r;   // Ex XS r   : A <- A XOR (r)  
		OpCodes[0xF0+i] = NS_r;   // Fx NS r   : A <- A AND (r)
	}
	OpCodes[0xCC] = AS_r_S;
	OpCodes[0xCD] = AS_r_I;
	OpCodes[0xCE] = AS_r_D;
	OpCodes[0xCF] = NOP;

	OpCodes[0xDC] = ASD_r_S;
	OpCodes[0xDD] = ASD_r_I;
	OpCodes[0xDE] = ASD_r_D;
	OpCodes[0xDF] = NOP;

	OpCodes[0xEC] = XS_r_S;
	OpCodes[0xED] = XS_r_I;
	OpCodes[0xEE] = XS_r_D;
	OpCodes[0xEF] = NOP;

	OpCodes[0xFC] = NS_r_S;
	OpCodes[0xFD] = NS_r_I;
	OpCodes[0xFE] = NS_r_D;
	OpCodes[0xFF] = NOP;
}

int F8_exec() // execute a single instruction
{
	PC0++;
	return OpCodes[Memory[PC0-1]](Memory[PC0-1]);
}

void F8_reset()
{
	int i;
	// clear registers, flags
	A=0;
	W=0;
	ISAR = 0;
	PC0=0; PC1=0;
	DC0=0; DC1=0;

	// clear scratchpad
	for(i=0; i<64; i++)
	{
		R[i] = 0; 
	}
}


