/* Legacy 32-bit ADPCM decoder. Private filename and entry name are inferred.
 *
 * Adapted from the Azimer ADPCM formulation retained in Wii64 ucode1.cpp.
 * The historical source relationship is inferred; the Wii64 file itself is
 * newer. See analysis/audio-adpcm-legacy-provenance.json and the accompanying
 * AudioADPCM-LICENSE.txt. The reconstruction retains the original DLL's
 * unusual short-buffer indexing and signed count behavior.
 */
#include "AudioADPCMLegacy.h"
#include <string.h>

void DecodeAudioADPCMLegacy(AudioCommand *command, AudioContext *context) {
    BYTE Flags=command->bytes[2];

    DWORD Address=command->words[1];
    WORD inPtr=0;

    short *out=context->buffer+(context->output >> 1);

    short count=(short)context->count;
    BYTE icode;
    BYTE code;
    int vscale;
    WORD index;
    WORD j;
    int a[8];
    short *book1,*book2;
    int l1,l2;
    int inp1[8];
    int inp2[8];
    count >>= 1;

    memset(out,0,32);

    if(!(Flags&0x1))
    {
        if(Flags&0x2) {
            for(j=0;j<16;j++)
                out[j]=*(short *)&RDRAM[(context->loopAddress+j*2)^2];
        } else {
            memcpy(out,&RDRAM[Address],32);
        }
    }

    l1=out[14];
    l2=out[15];
    out+=16;
    while(count>0)
    {

        code=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
        index=code&0xf;
        index<<=4;
        book1=context->buffer+0x800+index;
        book2=book1+8;
        code>>=4;
        vscale=(0xffff8000>>((12-code)-1));

        inPtr++;
        j=0;
        while(j<8)

        {
            icode=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
            inPtr++;

            inp1[j]=(short)((icode&0xf0)<<8);
            if(code<12)
                inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);

            j++;

            inp1[j]=(short)((icode&0xf)<<12);
            if(code<12)
                inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);

            j++;
        }
        j=0;
        while(j<8)
        {
            icode=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
            inPtr++;

            inp2[j]=(short)((icode&0xf0)<<8);
            if(code<12)
                inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);

            j++;

            inp2[j]=(short)((icode&0xf)<<12);
            if(code<12)
                inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);

            j++;
        }

        a[0]= (int)book1[0]*(int)l1;
        a[0]+=(int)book2[0]*(int)l2;
        a[0]+=(int)inp1[0]*(int)2048;

        a[1] =(int)book1[1]*(int)l1;
        a[1]+=(int)book2[1]*(int)l2;
        a[1]+=(int)book2[0]*inp1[0];
        a[1]+=(int)inp1[1]*(int)2048;

        a[2] =(int)book1[2]*(int)l1;
        a[2]+=(int)book2[2]*(int)l2;
        a[2]+=(int)book2[1]*inp1[0];
        a[2]+=(int)book2[0]*inp1[1];
        a[2]+=(int)inp1[2]*(int)2048;

        a[3] =(int)book1[3]*(int)l1;
        a[3]+=(int)book2[3]*(int)l2;
        a[3]+=(int)book2[2]*inp1[0];
        a[3]+=(int)book2[1]*inp1[1];
        a[3]+=(int)book2[0]*inp1[2];
        a[3]+=(int)inp1[3]*(int)2048;

        a[4] =(int)book1[4]*(int)l1;
        a[4]+=(int)book2[4]*(int)l2;
        a[4]+=(int)book2[3]*inp1[0];
        a[4]+=(int)book2[2]*inp1[1];
        a[4]+=(int)book2[1]*inp1[2];
        a[4]+=(int)book2[0]*inp1[3];
        a[4]+=(int)inp1[4]*(int)2048;

        a[5] =(int)book1[5]*(int)l1;
        a[5]+=(int)book2[5]*(int)l2;
        a[5]+=(int)book2[4]*inp1[0];
        a[5]+=(int)book2[3]*inp1[1];
        a[5]+=(int)book2[2]*inp1[2];
        a[5]+=(int)book2[1]*inp1[3];
        a[5]+=(int)book2[0]*inp1[4];
        a[5]+=(int)inp1[5]*(int)2048;

        a[6] =(int)book1[6]*(int)l1;
        a[6]+=(int)book2[6]*(int)l2;
        a[6]+=(int)book2[5]*inp1[0];
        a[6]+=(int)book2[4]*inp1[1];
        a[6]+=(int)book2[3]*inp1[2];
        a[6]+=(int)book2[2]*inp1[3];
        a[6]+=(int)book2[1]*inp1[4];
        a[6]+=(int)book2[0]*inp1[5];
        a[6]+=(int)inp1[6]*(int)2048;

        a[7] =(int)book1[7]*(int)l1;
        a[7]+=(int)book2[7]*(int)l2;
        a[7]+=(int)book2[6]*inp1[0];
        a[7]+=(int)book2[5]*inp1[1];
        a[7]+=(int)book2[4]*inp1[2];
        a[7]+=(int)book2[3]*inp1[3];
        a[7]+=(int)book2[2]*inp1[4];
        a[7]+=(int)book2[1]*inp1[5];
        a[7]+=(int)book2[0]*inp1[6];
        a[7]+=(int)inp1[7]*(int)2048;

        for(j=0;j<8;j++)
        {
            a[j]>>=11;
            if(a[j]>32767) a[j]=32767;
            else if(a[j]<-32768) a[j]=-32768;
            *(out++)=a[j];
        }
        l1=a[6];
        l2=a[7];

        a[0]= (int)book1[0]*(int)l1;
        a[0]+=(int)book2[0]*(int)l2;
        a[0]+=(int)inp2[0]*(int)2048;

        a[1] =(int)book1[1]*(int)l1;
        a[1]+=(int)book2[1]*(int)l2;
        a[1]+=(int)book2[0]*inp2[0];
        a[1]+=(int)inp2[1]*(int)2048;

        a[2] =(int)book1[2]*(int)l1;
        a[2]+=(int)book2[2]*(int)l2;
        a[2]+=(int)book2[1]*inp2[0];
        a[2]+=(int)book2[0]*inp2[1];
        a[2]+=(int)inp2[2]*(int)2048;

        a[3] =(int)book1[3]*(int)l1;
        a[3]+=(int)book2[3]*(int)l2;
        a[3]+=(int)book2[2]*inp2[0];
        a[3]+=(int)book2[1]*inp2[1];
        a[3]+=(int)book2[0]*inp2[2];
        a[3]+=(int)inp2[3]*(int)2048;

        a[4] =(int)book1[4]*(int)l1;
        a[4]+=(int)book2[4]*(int)l2;
        a[4]+=(int)book2[3]*inp2[0];
        a[4]+=(int)book2[2]*inp2[1];
        a[4]+=(int)book2[1]*inp2[2];
        a[4]+=(int)book2[0]*inp2[3];
        a[4]+=(int)inp2[4]*(int)2048;

        a[5] =(int)book1[5]*(int)l1;
        a[5]+=(int)book2[5]*(int)l2;
        a[5]+=(int)book2[4]*inp2[0];
        a[5]+=(int)book2[3]*inp2[1];
        a[5]+=(int)book2[2]*inp2[2];
        a[5]+=(int)book2[1]*inp2[3];
        a[5]+=(int)book2[0]*inp2[4];
        a[5]+=(int)inp2[5]*(int)2048;

        a[6] =(int)book1[6]*(int)l1;
        a[6]+=(int)book2[6]*(int)l2;
        a[6]+=(int)book2[5]*inp2[0];
        a[6]+=(int)book2[4]*inp2[1];
        a[6]+=(int)book2[3]*inp2[2];
        a[6]+=(int)book2[2]*inp2[3];
        a[6]+=(int)book2[1]*inp2[4];
        a[6]+=(int)book2[0]*inp2[5];
        a[6]+=(int)inp2[6]*(int)2048;

        a[7] =(int)book1[7]*(int)l1;
        a[7]+=(int)book2[7]*(int)l2;
        a[7]+=(int)book2[6]*inp2[0];
        a[7]+=(int)book2[5]*inp2[1];
        a[7]+=(int)book2[4]*inp2[2];
        a[7]+=(int)book2[3]*inp2[3];
        a[7]+=(int)book2[2]*inp2[4];
        a[7]+=(int)book2[1]*inp2[5];
        a[7]+=(int)book2[0]*inp2[6];
        a[7]+=(int)inp2[7]*(int)2048;

        for(j=0;j<8;j++)
        {
            a[j]>>=11;
            if(a[j]>32767) a[j]=32767;
            else if(a[j]<-32768) a[j]=-32768;
            *(out++)=a[j];
        }
        l1=a[6];
        l2=a[7];

        count-=32;
    }
    out-=16;
    memcpy(&RDRAM[Address],out,32);
}

