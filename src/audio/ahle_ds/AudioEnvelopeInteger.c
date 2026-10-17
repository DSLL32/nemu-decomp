/* Integer envelope mixers. Nemu source grouping and private function names are
 * inferred. MixAudioEnvelope and MixAudioEnvelopeLegacy retain the source
 * structure of UltraHLE's archived SLIST.C envmix_mario/envmix_zelda routines;
 * globals are adapted to the context argument observed in the Nemu binary. */
#include "AudioHLE.h"

void MixAudioEnvelopeClamped(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context)
{
    int left;
    int right;
    int rampLeft;
    int rampRight;
    int limitLeft;
    int limitRight;
    int dry;
    int wet;
    int i, j;
    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    dry = context->dry;
    left = context->targetLeft;
    right = context->targetRight;
    rampLeft = context->rampLeft;
    rampRight = context->rampRight;
    limitLeft = context->rateLeft;
    limitRight = context->rateRight;
    wet = context->wet;
    if (rampLeft == 0xffff) { left = limitLeft; rampLeft = 0; }
    if (rampRight == 0xffff) { right = limitRight; rampRight = 0; }
    if (wet == 0 && left == 0 && rampLeft == 0 && right == 0) return;
    for (i = 0; i < count; i += 8) {
        for (j = i; j < i + 8; ++j) {
            int sample = context->buffer[input + j];
            int l = (sample * left) >> 15;
            int r = (sample * right) >> 15;
            int outLeft = context->buffer[output + j] + l * dry / 32768;
            int outRight = context->buffer[aux0 + j] + r * dry / 32768;
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[output + j] = outLeft;
            context->buffer[aux0 + j] = outRight;
            outLeft = context->buffer[aux1 + j] + ((l * wet) >> 15);
            outRight = context->buffer[aux2 + j] + ((r * wet) >> 15);
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[aux1 + j] = outLeft;
            context->buffer[aux2 + j] = outRight;
        }
        left += rampLeft;
        if (rampLeft > 0) {
            if (left > limitLeft) { left = limitLeft; rampLeft = 0; }
        } else {
            if (left < limitLeft) { left = limitLeft; rampLeft = 0; }
        }
        right += rampRight;
        if (rampRight > 0) {
            if (right > limitRight) { right = limitRight; rampRight = 0; }
        } else {
            if (right < limitRight) { right = limitRight; rampRight = 0; }
        }
    }
    context->rampLeft = rampLeft;
    context->rampRight = rampRight;
    context->volumeLeft = wet;
    context->volumeRight = wet;
    context->targetLeft = left;
    context->targetRight = right;
    context->rateLeft = limitLeft;
    context->rateRight = limitRight;
}

void MixAudioEnvelope(int src,int dst1,int dst2,int eff1,int eff2,int cnt,AudioContext *context)
{
    int i,j,jn;
    int lteff,rteff;
    int ltmul,ltadd,lttar;
    int rtmul,rtadd,rttar;
    int lt,rt;
    cnt>>=1;
    src>>=1;
    dst1>>=1;
    dst2>>=1;
    eff1>>=1;
    eff2>>=1;

    lteff=context->volumeLeft<<1;
    rteff=context->volumeRight<<1;
    ltmul=context->targetLeft<<1;
    rtmul=context->targetRight<<1;
    ltadd=context->rampLeft<<1;
    rtadd=context->rampRight<<1;
    lttar=context->rateLeft<<1;
    rttar=context->rateRight<<1;

    if(ltadd==65535 && ltadd>0)
    {
        ltmul=lttar;
        ltadd=0;
    }
    if(rtadd==65535 && rtadd>0)
    {
        rtmul=rttar;
        rtadd=0;
    }

    if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul)
    {
        // no need to mix, volumes zero
        return;
    }


    if(!lteff && !rteff && !ltadd && !rtadd)
    {
        // no effect mix
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            context->buffer[dst1+j]+=lt;
            context->buffer[dst2+j]+=rt;
        }
    }
    else if(!ltadd && !rtadd)
    {
        // no volume change
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            context->buffer[dst1+j]+=lt;
            context->buffer[dst2+j]+=rt;
            lt=(lt*lteff)>>16;
            rt=(rt*rteff)>>16;
            context->buffer[eff1+j]+=lt;
            context->buffer[eff2+j]+=rt;
        }
    }
    else
    {
        // full mix (no target)
        for(i=0;i<cnt;i+=8)
        {
            jn=i+8;
            for(j=i;j<jn;j++)
            {
                lt=rt=context->buffer[src+j];
                lt=(lt*ltmul)>>16;
                rt=(rt*rtmul)>>16;
                context->buffer[dst1+j]+=lt;
                context->buffer[dst2+j]+=rt;
                lt=(lt*lteff)>>16;
                rt=(rt*rteff)>>16;
                context->buffer[eff1+j]+=lt;
                context->buffer[eff2+j]+=rt;
            }
            ltmul+=ltadd;
            if(ltadd>0)
            {
                if(ltmul>lttar)
                {
                    ltmul=lttar;
                    ltadd=0;
                }
            }
            else
            {
                if(ltmul<lttar)
                {
                    ltmul=lttar;
                    ltadd=0;
                }
            }
            rtmul+=rtadd;
            if(rtadd>0)
            {
                if(rtmul>rttar)
                {
                    rtmul=rttar;
                    rtadd=0;
                }
            }
            else
            {
                if(rtmul<rttar)
                {
                    rtmul=rttar;
                    rtadd=0;
                }
            }
        }
    }

    context->volumeLeft=lteff>>1;
    context->volumeRight=rteff>>1;
    context->rampLeft=ltadd>>1;
    context->rampRight=rtadd>>1;
    context->targetLeft=ltmul>>1;
    context->targetRight=rtmul>>1;
    context->rateLeft=lttar>>1;
    context->rateRight=rttar>>1;
}

void MixAudioEnvelopeLegacy(int src,int dst1,int dst2,int eff1,int eff2,int cnt,AudioContext *context)
{
    int i,j,jn,lx,rx;
    int lteff,rteff;
    int ltmul,ltadd;
    int rtmul,rtadd;
    int lt,rt;
    cnt>>=1;
    src>>=1;
    dst1>>=1;
    dst2>>=1;
    eff1>>=1;
    eff2>>=1;

    lteff=context->volumeLeft;
    rteff=context->volumeRight;
    ltmul=context->targetLeft;
    rtmul=context->targetRight;
    ltadd=context->rampLeft;
    rtadd=context->rampRight;

    if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul)
    {
        // no need to mix, volumes zero
        return;
    }


    if(!lteff && !rteff && !ltadd && !rtadd)
    {
        // no effect mix
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            lx=lt+context->buffer[dst1+j];
            rx=rt+context->buffer[dst2+j];
            /*
            if(lx<-32767) lx=-32767;
            if(lx> 32767) lx= 32767;
            if(rx<-32767) rx=-32767;
            if(rx> 32767) rx= 32767;
            */
            context->buffer[dst1+j]=lx;
            context->buffer[dst2+j]=rx;
        }
    }
    else if(!ltadd && !rtadd)
    {
        // no volume change
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            lx=lt+context->buffer[dst1+j];
            rx=rt+context->buffer[dst2+j];
            /*
            if(lx<-32767) lx=-32767;
            if(lx> 32767) lx= 32767;
            if(rx<-32767) rx=-32767;
            if(rx> 32767) rx= 32767;
            */
            context->buffer[dst1+j]=lx;
            context->buffer[dst2+j]=rx;
            lt=(lt*lteff)>>16;
            rt=(rt*rteff)>>16;
            context->buffer[eff1+j]+=lt;
            context->buffer[eff2+j]+=rt;
        }
    }
    else
    {
        for(i=0;i<cnt;i+=8)
        {
            jn=i+8;
            for(j=i;j<jn;j++)
            {
                lt=rt=context->buffer[src+j];
                lt=(lt*ltmul)>>16;
                rt=(rt*rtmul)>>16;
                lx=lt+context->buffer[dst1+j];
                rx=rt+context->buffer[dst2+j];
                /*
                if(lx<-32767) lx=-32767;
                if(lx> 32767) lx= 32767;
                if(rx<-32767) rx=-32767;
                if(rx> 32767) rx= 32767;
                */
                context->buffer[dst1+j]=lx;
                context->buffer[dst2+j]=rx;
                lt=(lt*lteff)>>16;
                rt=(rt*rteff)>>16;
                context->buffer[eff1+j]+=lt;
                context->buffer[eff2+j]+=rt;
            }
            ltmul+=ltadd;
            rtmul+=rtadd;
            if(ltmul>65535)
            {
                ltmul=65535;
                ltadd=0;
            }
            else if(ltmul<0)
            {
                ltmul=0;
                ltadd=0;
            }
            if(rtmul>65535)
            {
                rtmul=65535;
                rtadd=0;
            }
            else if(rtmul<0)
            {
                rtmul=0;
                rtadd=0;
            }
        }
    }
}
