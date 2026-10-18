#ifndef KAILLERA_GAME_H
#define KAILLERA_GAME_H
#include "protocol.h"
#include "bytequeue.h"
extern HWND gameInput;
extern char currentGameName[128];
extern int initialSynced,syncCancelled,frameDelayCount,frameWithinPacket;
extern ByteQueue sendByteQueue,receiveByteQueue;
extern ByteQueueList receivedDataCache,sentDataCache;
void showHostGamePanel();
void showJoinedGamePanel();
BOOL CALLBACK hostGameDialog(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK joinedGameDialog(HWND,UINT,WPARAM,LPARAM);
void showGameContextMenu(int x,int y);
void pumpGameMessages();
void showSyncDialog();
void closeSyncDialog();
BOOL CALLBACK syncDialog(HWND,UINT,WPARAM,LPARAM);
#endif
