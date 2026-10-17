/* Internal HLE timing counters. Original source filename is unknown. */
#include <string.h>

typedef unsigned __int64 AudioClock;

typedef struct AudioProfileCounter {
    AudioClock start;
    AudioClock end;
    AudioClock total;
    AudioClock count;
    AudioClock average;
    AudioClock maximum;
    int enabled;
} AudioProfileCounter;

AudioProfileCounter AudioProfileCounters[4096];

#define READ_AUDIO_CLOCK(destination) \
    { AudioClock *clock = (destination); \
      __asm mov edi, clock \
      __asm pushad \
      __asm rdtsc \
      __asm mov [edi+4], edx \
      __asm mov [edi], eax \
      __asm popad }

void ResetAudioProfile(void)
{
    memset(AudioProfileCounters, 0, sizeof(AudioProfileCounters));
}

void AverageAudioProfile(unsigned int index)
{
    AudioProfileCounters[index].average = AudioProfileCounters[index].total /
        AudioProfileCounters[index].count;
}

void AdjustAudioProfileMaximum(unsigned int index)
{
    AudioProfileCounters[index].maximum += AudioProfileCounters[index].maximum /
        AudioProfileCounters[index].count;
}

void StartAudioProfile(unsigned int index)
{
    if (AudioProfileCounters[index].count <= 16) {
        AudioProfileCounters[index].enabled = 1;
        READ_AUDIO_CLOCK(&AudioProfileCounters[index].start);
    }
}

void StopAudioProfile(unsigned int index)
{
    if (AudioProfileCounters[index].count <= 16 &&
        AudioProfileCounters[index].start != 0) {
        READ_AUDIO_CLOCK(&AudioProfileCounters[index].end);
        if (AudioProfileCounters[index].end >= AudioProfileCounters[index].start) {
            AudioProfileCounters[index].total += AudioProfileCounters[index].end -
                AudioProfileCounters[index].start;
            ++AudioProfileCounters[index].count;
            if (AudioProfileCounters[index].maximum < AudioProfileCounters[index].end -
                AudioProfileCounters[index].start)
                AudioProfileCounters[index].maximum = AudioProfileCounters[index].end -
                    AudioProfileCounters[index].start;
        }
    }
}
