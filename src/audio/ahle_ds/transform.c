/* Derived reconstruction of AMP 0.7.6 transform.c.
 * Copyright (C) Tomislav Uzelac 1996,1997.
 * Karl Anders Oygard optimized the polyphase transform, March 13, 1997.
 * See AMP-LICENSE.txt and analysis/audio-mp3-provenance.json.
 */
#include "transform.h"

float res[32][18];
int nch = 1;

void poly(const int ch, int f, short sample_buffer[18][32][2])
{
static float u[2][2][17][16];
static int u_start[2]={0,0};
static int u_div[2]={0,0};
int start = u_start[ch];
int div = u_div[ch];
float (*u_p)[16];
 {
 float d16,d17,d18,d19,d20,d21,d22,d23,d24,d25,d26,d27,d28,d29,d30,d31;
 float d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15;
 d0 = res[ 0][f]; d16=(d0 - res[31][f]) * b1; d0 += res[31][f];
 d1 = res[ 1][f]; d17=(d1 - res[30][f]) * b3; d1 += res[30][f];
 d3 = res[ 2][f]; d19=(d3 - res[29][f]) * b5; d3 += res[29][f];
 d2 = res[ 3][f]; d18=(d2 - res[28][f]) * b7; d2 += res[28][f];
 d6 = res[ 4][f]; d22=(d6 - res[27][f]) * b9; d6 += res[27][f];
 d7 = res[ 5][f]; d23=(d7 - res[26][f]) * b11; d7 += res[26][f];
 d5 = res[ 6][f]; d21=(d5 - res[25][f]) * b13; d5 += res[25][f];
 d4 = res[ 7][f]; d20=(d4 - res[24][f]) * b15; d4 += res[24][f];
 d12= res[ 8][f]; d28=(d12 - res[23][f]) * b17; d12+= res[23][f];
 d13= res[ 9][f]; d29=(d13 - res[22][f]) * b19; d13+= res[22][f];
 d15= res[10][f]; d31=(d15 - res[21][f]) * b21; d15+= res[21][f];
 d14= res[11][f]; d30=(d14 - res[20][f]) * b23; d14+= res[20][f];
 d10= res[12][f]; d26=(d10 - res[19][f]) * b25; d10+= res[19][f];
 d11= res[13][f]; d27=(d11 - res[18][f]) * b27; d11+= res[18][f];
 d9 = res[14][f]; d25=(d9 - res[17][f]) * b29; d9 += res[17][f];
 d8 = res[15][f]; d24=(d8 - res[16][f]) * b31; d8 += res[16][f];
 {
 float c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15;
 c0 = d0 + d8 ; c8 = ( d0 - d8 ) * b2;
 c1 = d1 + d9 ; c9 = ( d1 - d9 ) * b6;
 c2 = d2 + d10; c10= ( d2 - d10) * b14;
 c3 = d3 + d11; c11= ( d3 - d11) * b10;
 c4 = d4 + d12; c12= ( d4 - d12) * b30;
 c5 = d5 + d13; c13= ( d5 - d13) * b26;
 c6 = d6 + d14; c14= ( d6 - d14) * b18;
 c7 = d7 + d15; c15= ( d7 - d15) * b22;
 d0 = c0 + c4 ; d4 = ( c0 - c4 ) * b4;
 d1 = c1 + c5 ; d5 = ( c1 - c5 ) * b12;
 d2 = c2 + c6 ; d6 = ( c2 - c6 ) * b28;
 d3 = c3 + c7 ; d7 = ( c3 - c7 ) * b20;
 d8 = c8 + c12; d12= ( c8 - c12) * b4;
 d9 = c9 + c13; d13= ( c9 - c13) * b12;
 d10= c10+ c14; d14= (c10 - c14) * b28;
 d11= c11+ c15; d15= (c11 - c15) * b20;
 {
 float rb8 = b8;
 float rb24 = b24;
     c0 = d0 + d2 ; c2 = ( d0 - d2 ) * rb8;
 c1 = d1 + d3 ; c3 = ( d1 - d3 ) * rb24;
     c4 = d4 + d6 ; c6 = ( d4 - d6 ) * rb8;
 c5 = d5 + d7 ; c7 = ( d5 - d7 ) * rb24;
     c8 = d8 + d10; c10= ( d8 - d10) * rb8;
 c9 = d9 + d11; c11= ( d9 - d11) * rb24;
     c12= d12+ d14; c14= (d12 - d14) * rb8;
 c13= d13+ d15; c15= (d13 - d15) * rb24;
 }
 {
 float rb16 = b16;
 d0 = (-c0 -c1) * 2; d1 = ( c0 - c1 ) * rb16;
 d2 = c2 + c3; d3 = ( c2 - c3 ) * rb16;
 d3 -= d2;
 d4 = c4 +c5; d5 = ( c4 - c5 ) * rb16;
 d5 += d4;
 d7 = -d5;
 d7 += ( c6 - c7 ) * rb16; d6 = +c6 +c7;
 d8 = c8 + c9 ; d9 = ( c8 - c9 ) * rb16;
 d11= +d8 +d9;
 d11 +=(c10 - c11) * rb16; d10= c10+ c11;
 d12 = c12+ c13; d13 = (c12 - c13) * rb16;
 d13 += -d8-d9+d12;
 d14 = c14+ c15; d15 = (c14 - c15) * rb16;
 d15-=d11;
 d14 += -d8 -d10;
 }
        u_p = (float (*)[16]) &u[ch][div][0][start];
        u_p[ 0][0] =+d1 ;
        u_p[ 2][0] = +d9 -d14;
        u_p[ 4][0] = +d5 -d6;
        u_p[ 6][0] = -d10 +d13;
        u_p[ 8][0] =d3;
        u_p[10][0] = -d8 -d9 +d11 -d13;
        u_p[12][0] = +d7;
        u_p[14][0] = +d15;
        u_p = (float (*)[16]) &u[ch][!div][0][start];
        u_p[16][0] = d0;
        u_p[14][0] = -(+d8 );
        u_p[12][0] = -(+d4 );
        u_p[10][0] = -(-d8 +d12 );
        u_p[ 8][0] = -(+d2 );
        u_p[ 6][0] = -(+d8 +d10 -d12 );
        u_p[ 4][0] = -(-d4 +d6 );
        u_p[ 2][0] = -d14;
        u_p[ 0][0] = -d1;
 }
 {
 float c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15;
        c0=d16 + d24; c8= (d16 - d24) * b2;
        c1=d17 + d25; c9= (d17 - d25) * b6;
        c2=d18 + d26; c10= (d18 - d26) * b14;
        c3=d19 + d27; c11= (d19 - d27) * b10;
        c4=d20 + d28; c12= (d20 - d28) * b30;
        c5=d21 + d29; c13= (d21 - d29) * b26;
        c6=d22 + d30; c14= (d22 - d30) * b18;
        c7=d23 + d31; c15= (d23 - d31) * b22;
        d16= c0+ c4; d20= (c0 - c4) * b4;
        d17= c1+ c5; d21= (c1 - c5) * b12;
        d18= c2+ c6; d22= (c2 - c6) * b28;
        d19= c3+ c7; d23= (c3 - c7) * b20;
        d24= c8+ c12; d28= (c8 - c12) * b4;
        d25= c9+ c13; d29= (c9 - c13) * b12;
        d26= c10+ c14; d30= (c10 - c14) * b28;
        d27= c11+ c15; d31= (c11 - c15) * b20;
        {
        float rb8 = b8;
        float rb24 = b24;
        c0= d16+ d18; c2= (d16 - d18) * rb8;
        c1= d17+ d19; c3= (d17 - d19) * rb24;
        c4= d20+ d22; c6= (d20 - d22) * rb8;
        c5= d21+ d23; c7= (d21 - d23) * rb24;
        c8= d24+ d26; c10= (d24 - d26) * rb8;
        c9= d25+ d27; c11= (d25 - d27) * rb24;
        c12= d28+ d30; c14= (d28 - d30) * rb8;
        c13= d29+ d31; c15= (d29 - d31) * rb24;
        }
        {
        float rb16 = b16;
        d16= c0+ c1; d17= (c0 - c1) * rb16;
        d18= c2+ c3; d19= (c2 - c3) * rb16;
        d20= c4+ c5; d21= (c4 - c5) * rb16;
        d20+=d16; d21+=d17;
        d22= c6+ c7; d23= (c6 - c7) * rb16;
        d22+=d16; d22+=d18;
        d23+=d16; d23+=d17; d23+=d19;
        d24= c8+ c9; d25= (c8 - c9) * rb16;
        d26= c10+ c11; d27= (c10 - c11) * rb16;
        d26+=d24;
        d27+=d24; d27+=d25;
        d28= c12+ c13; d29= (c12 - c13) * rb16;
        d28-=d20; d29+=d28; d29-=d21;
        d30= c14+ c15; d31= (c14 - c15) * rb16;
        d30-=d22;
        d31-=d23;
 }
 u_p = (float (*)[16]) &u[ch][!div][0][start];
 u_p[ 1][0] = -(+d30 );
 u_p[ 3][0] = -(+d22 -d26 );
 u_p[ 5][0] = -(-d18 -d20 +d26 );
 u_p[ 7][0] = -(+d18 -d28 );
 u_p[ 9][0] = -(+d28 );
 u_p[11][0] = -(+d20 -d24 );
 u_p[13][0] = -(-d16 +d24 );
 u_p[15][0] = -(+d16 );
 u_p = (float (*)[16]) &u[ch][div][0][start];
 u_p[15][0] = +d31;
 u_p[13][0] = +d23 -d27;
 u_p[11][0] = -d19 -d20 -d21 +d27;
 u_p[ 9][0] = +d19 -d29;
 u_p[ 7][0] = -d18 +d29;
 u_p[ 5][0] = +d18 +d20 +d21 -d25 -d26;
 u_p[ 3][0] = -d17 -d22 +d25 +d26;
 u_p[ 1][0] = +d17 -d30;
 }
 }
 {
   short *samples = (&sample_buffer[f>>(2-nch)][nch==2?0:(f&1?16:0)][ch]);
   int out, j;
   const float *dewindow = t_dewindow[0] + 16 - start;
   {
     float (*u_ptr)[16] = u[ch][div];
     for (j = 0; j < 16; ++j) {
       float outf1, outf2, outf3, outf4;
       outf1 = u_ptr[0][ 0] * dewindow[0x0];
       outf2 = u_ptr[0][ 1] * dewindow[0x1];
       outf3 = u_ptr[0][ 2] * dewindow[0x2];
       outf4 = u_ptr[0][ 3] * dewindow[0x3];
       outf1 += u_ptr[0][ 4] * dewindow[0x4];
       outf2 += u_ptr[0][ 5] * dewindow[0x5];
       outf3 += u_ptr[0][ 6] * dewindow[0x6];
       outf4 += u_ptr[0][ 7] * dewindow[0x7];
       outf1 += u_ptr[0][ 8] * dewindow[0x8];
       outf2 += u_ptr[0][ 9] * dewindow[0x9];
       outf3 += u_ptr[0][10] * dewindow[0xa];
       outf4 += u_ptr[0][11] * dewindow[0xb];
       outf1 += u_ptr[0][12] * dewindow[0xc];
       outf2 += u_ptr[0][13] * dewindow[0xd];
       outf3 += u_ptr[0][14] * dewindow[0xe];
       outf4 += u_ptr[0][15] * dewindow[0xf];
       out = outf1 + outf2 + outf3 + outf4;
       dewindow += 32;
       u_ptr++;
       if (out > 32767) *samples = 32767; else if (out < -32768) *samples = -32768; else *samples = out; samples += nch;
     }
     if (div & 0x1) {
       {
  float outf2, outf4;
  outf2 = u_ptr[0][ 0] * dewindow[0x0];
  outf4 = u_ptr[0][ 2] * dewindow[0x2];
  outf2 += u_ptr[0][ 4] * dewindow[0x4];
  outf4 += u_ptr[0][ 6] * dewindow[0x6];
  outf2 += u_ptr[0][ 8] * dewindow[0x8];
  outf4 += u_ptr[0][10] * dewindow[0xa];
  outf2 += u_ptr[0][12] * dewindow[0xc];
  outf4 += u_ptr[0][14] * dewindow[0xe];
  out = outf2 + outf4;
  if (out > 32767) *samples = 32767; else if (out < -32768) *samples = -32768; else *samples = out; samples += nch;
       }
       dewindow -= 48;
       dewindow += start;
       dewindow += start;
       for (; j < 31; ++j) {
  float outf1, outf2, outf3, outf4;
  --u_ptr;
  outf1 = u_ptr[0][ 0] * dewindow[0xf];
  outf2 = u_ptr[0][ 1] * dewindow[0xe];
  outf3 = u_ptr[0][ 2] * dewindow[0xd];
  outf4 = u_ptr[0][ 3] * dewindow[0xc];
  outf1 += u_ptr[0][ 4] * dewindow[0xb];
  outf2 += u_ptr[0][ 5] * dewindow[0xa];
  outf3 += u_ptr[0][ 6] * dewindow[0x9];
  outf4 += u_ptr[0][ 7] * dewindow[0x8];
  outf1 += u_ptr[0][ 8] * dewindow[0x7];
  outf2 += u_ptr[0][ 9] * dewindow[0x6];
  outf3 += u_ptr[0][10] * dewindow[0x5];
  outf4 += u_ptr[0][11] * dewindow[0x4];
  outf1 += u_ptr[0][12] * dewindow[0x3];
  outf2 += u_ptr[0][13] * dewindow[0x2];
  outf3 += u_ptr[0][14] * dewindow[0x1];
  outf4 += u_ptr[0][15] * dewindow[0x0];
  out = -outf1 + outf2 - outf3 + outf4;
  dewindow -= 32;
  if (out > 32767) *samples = 32767; else if (out < -32768) *samples = -32768; else *samples = out; samples += nch;
       }
     } else {
       {
  float outf2, outf4;
  outf2 = u_ptr[0][ 1] * dewindow[0x1];
  outf4 = u_ptr[0][ 3] * dewindow[0x3];
  outf2 += u_ptr[0][ 5] * dewindow[0x5];
  outf4 += u_ptr[0][ 7] * dewindow[0x7];
  outf2 += u_ptr[0][ 9] * dewindow[0x9];
  outf4 += u_ptr[0][11] * dewindow[0xb];
  outf2 += u_ptr[0][13] * dewindow[0xd];
  outf4 += u_ptr[0][15] * dewindow[0xf];
  out = outf2 + outf4;
  if (out > 32767) *samples = 32767; else if (out < -32768) *samples = -32768; else *samples = out; samples += nch;
       }
       dewindow -= 48;
       dewindow += start;
       dewindow += start;
       for (; j < 31; ++j) {
  float outf1, outf2, outf3, outf4;
  --u_ptr;
  outf1 = u_ptr[0][ 0] * dewindow[0xf];
  outf2 = u_ptr[0][ 1] * dewindow[0xe];
  outf3 = u_ptr[0][ 2] * dewindow[0xd];
  outf4 = u_ptr[0][ 3] * dewindow[0xc];
  outf1 += u_ptr[0][ 4] * dewindow[0xb];
  outf2 += u_ptr[0][ 5] * dewindow[0xa];
  outf3 += u_ptr[0][ 6] * dewindow[0x9];
  outf4 += u_ptr[0][ 7] * dewindow[0x8];
  outf1 += u_ptr[0][ 8] * dewindow[0x7];
  outf2 += u_ptr[0][ 9] * dewindow[0x6];
  outf3 += u_ptr[0][10] * dewindow[0x5];
  outf4 += u_ptr[0][11] * dewindow[0x4];
  outf1 += u_ptr[0][12] * dewindow[0x3];
  outf2 += u_ptr[0][13] * dewindow[0x2];
  outf3 += u_ptr[0][14] * dewindow[0x1];
  outf4 += u_ptr[0][15] * dewindow[0x0];
  out = outf1 - outf2 + outf3 - outf4;
  dewindow -= 32;
  if (out > 32767) *samples = 32767; else if (out < -32768) *samples = -32768; else *samples = out; samples += nch;
       }
     }
   }
 }
 --u_start[ch];
 u_start[ch] &= 0xf;
 u_div[ch]=u_div[ch] ? 0 : 1;
}
