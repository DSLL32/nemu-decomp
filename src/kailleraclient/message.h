// Reconstructed Kaillera 0.84 protocol message. Names are inferred.
#ifndef KAILLERA_MESSAGE_H
#define KAILLERA_MESSAGE_H

class Message {
public:
    Message();
    ~Message();
    Message(const Message &other);
    void setType(int value) { type=value; }
    void reserve(int size);
    void setNick(char *nick);
    void append(void *data, int size);
    void read(void *data, int size);
    void appendString(char *text);
    void readString(char *text, int size);
    void appendInt(int value);
    int readInt();
    void appendByte(unsigned char value);
    unsigned char readByte();
    void appendWord(unsigned short value);
    unsigned short readWord();
    int pack(char *data, int size);
    void unpack(char *data, int size);

    int type;
    char nick[32];
    char *buffer;
    int capacity;
    int length;
};
#endif
