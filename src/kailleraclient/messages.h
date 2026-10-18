// Packet-specific inline classes inferred from parsed-field offsets, object
// sizes, base constructors and exception-unwind lifecycle order in the DLL.
#ifndef KAILLERA_MESSAGES_H
#define KAILLERA_MESSAGES_H
#include "message.h"

class SimpleMessage : public Message {
public:
    SimpleMessage(int kind) { type=kind; }
};

class LoginMessage : public Message {
public:
    LoginMessage(char *application,char *name,int connection) {
        type=3;
        appendString(application);
        setNick(name);
        appendByte((char)connection);
    }
    char application[128];
    int connection;
};

class PongMessage : public Message {
public:
    PongMessage() { type=6; for (int i=0;i<4;i++) appendInt(i); }
};

class UserJoinedMessage : public Message {
public:
    UserJoinedMessage(const Message &message):Message(message) {
        id=readWord(); ping=readInt(); connection=readByte();
    }
    int id,ping,connection;
};

class UserLeftMessage : public Message {
public:
    UserLeftMessage(int value,char *text) { type=1; appendWord((short)value); appendString(text); }
    UserLeftMessage(const Message &message):Message(message) {
        id=readWord(); readString(reason,128);
    }
    char reason[128];
    int id;
};

class ServerStatusMessage : public Message {
public:
    ServerStatusMessage(const Message &message):Message(message) {
        users=readInt(); games=readInt();
    }
    int users,games;
};

class ChatMessage : public Message {
public:
    ChatMessage(const Message &message):Message(message) { readString(text,128); }
    ChatMessage(int kind,char *value) { type=kind; appendString(value); }
    char text[128];
};

class GameCreatedMessage : public Message {
public:
    GameCreatedMessage(char *name) {
        type=10; appendString(name); appendString(""); appendInt(-1);
    }
    GameCreatedMessage(const Message &message):Message(message) {
        readString(game,128); readString(application,128); id=readInt();
    }
    char game[128],application[128];
    int id;
};

class GameUserLeftMessage : public Message {
public:
    GameUserLeftMessage(const Message &message):Message(message) { id=readWord(); }
    GameUserLeftMessage(int value) { type=11; appendWord((short)value); }
    int id;
};

class GameUserJoinedMessage : public Message {
public:
    GameUserJoinedMessage(int value) {
        type=12; appendInt(value); appendString(""); appendInt(0); appendWord(-1); appendByte(1);
    }
    GameUserJoinedMessage(const Message &message):Message(message) {
        game=readInt(); readString(name,32); ping=readInt();
        id=readWord(); connection=readByte();
    }
    char name[32];
    int game,ping,id,connection;
};

class GameUsersMessage : public Message {
public:
    GameUsersMessage(const Message &message):Message(message) { users=readInt(); }
    int users;
};

class GameStatusMessage : public Message {
public:
    GameStatusMessage(const Message &message):Message(message) {
        id=readInt(); status=readByte(); users=readByte(); maximum=readByte();
    }
    int id,status,users,maximum;
};

class GameDeletedMessage : public Message {
public:
    GameDeletedMessage(const Message &message):Message(message) { id=readInt(); }
    int id;
};

class GameStartedMessage : public Message {
public:
    GameStartedMessage() { type=17; appendWord(-1); appendByte(-1); appendByte(-1); }
    GameStartedMessage(const Message &message):Message(message) {
        delay=readWord(); player=readByte(); players=readByte();
    }
    int delay,player,players;
};

class KickMessage : public Message {
public:
    KickMessage(int value) { type=15; appendWord((short)value); }
    int id;
};

class GameDataMessage : public Message {
public:
    GameDataMessage(void *data,int bytes) { type=18; appendWord((short)bytes); append(data,bytes); }
    GameDataMessage(const Message &message):Message(message) { size=readWord(); }
    unsigned int size;
};

class CachedGameDataMessage : public Message {
public:
    CachedGameDataMessage(int value) { type=19; appendByte((char)value); }
    CachedGameDataMessage(const Message &message):Message(message) { index=readByte(); }
    int index;
};

class PlayerDroppedMessage : public Message {
public:
    PlayerDroppedMessage(const Message &message):Message(message) { player=readByte(); }
    PlayerDroppedMessage(int number) { type=20; appendByte((char)number); }
    int player;
};
#endif
