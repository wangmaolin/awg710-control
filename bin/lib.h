#ifndef _LIB_H_
#define _LIB_H_

#define APIENTRY

int APIENTRY AwgConnect(char *hostname);
void APIENTRY AwgDisconnect(int fd);
int APIENTRY AwgSend(int fd, char *s, int len);
int APIENTRY AwgSendString(int fd, char *s);
int APIENTRY AwgReceive(int fd, char *buf, int bufSize);
int APIENTRY AwgReceiveMessage(int fd, char *buf, int bufSize);
int APIENTRY AwgReceiveBinary(int fd, char *buf, int bufSize);
int APIENTRY AwgSendFile(int fd, char *localFilename, char *remoteFilename);
int APIENTRY AwgReceiveFile(int fd, char *remoteFilename, char *localFilename);

#endif /* _LIB_H_ */
