/* 
 * This is a source of the library to control AWG series from the network.
 * This program can be compiled and executed on generic unix system and 
 * WIN32 (Windows 95/98/NT/2000/Me) environment.
 * This program is tested both with Windows2000 with MSVC++6.0 SP5 and 
 * SunOS Release 4.1.3 with gcc 2.3.3.
 * In WIN32 environment, this source file can be used to build DLL or
 * simply linked with other program to build a standalone EXE.
 */

#include <stdio.h> /* for fopen() etc. */
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#define closesocket(a)	close(a)
#define FALSE 0
#define TRUE 1

#include "lib.h"

int debug = FALSE;
#define dprintf	if (debug) printf

#define PORT_NO	4000

#define BUFSIZE	8192
char buffer[BUFSIZE];
char savedBuffer[BUFSIZE];	/* used by AwgReceiveMessage() and AwgReceiveBinary(); */
char *savedBufferPtr = NULL;
char savedBufferRemains = 0;

typedef struct sockaddr SockAddr;
typedef struct sockaddr_in SockAddr_In;
typedef struct hostent HostEnt;

/* declaration of static functions */

static void showSocketError(char *s);
static int sendFromFile(int fd, char *fname);
static int getFileSize(char *fname);
static int receiveIntoFile(int fd, char *fname, int numBytes);
#define cleanup()

/* AwgConnect() returns
 * 	0  : succeed
 * 	-1 : error
 */

int APIENTRY AwgConnect(char *hostname)
{
	int fd;
	SockAddr_In clientName, serverName;
	HostEnt *hostEntry;
	int ret;
	int b1, b2, b3, b4;
	
 
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		showSocketError("socket() failed");
		cleanup();
		return -1;
	}
	
	memset(&clientName, 0, sizeof(clientName));
	clientName.sin_family = AF_INET;
	clientName.sin_port = 0;
	clientName.sin_addr.s_addr = INADDR_ANY;

	ret = bind(fd, (SockAddr *)(&clientName), sizeof(clientName));
	if (ret)
	{
		showSocketError("bind failed");
		closesocket(fd);
		cleanup();
		return -1;
	}
	
	memset(&serverName, 0, sizeof(serverName));
	serverName.sin_family = AF_INET;

	if ('0' <= hostname[0] && hostname[0] <= '9')
	{
		inet_aton(hostname,&serverName.sin_addr.s_addr);
	}
	else
	{
		hostEntry = gethostbyname(hostname);
		if (hostEntry == NULL)
		{
			showSocketError("gethostbyname failed\n");
			closesocket(fd);
			cleanup();
			return -1;
		}
		serverName.sin_addr.s_addr = *(int *)(hostEntry->h_addr);
	}	

	serverName.sin_port = htons((u_short)PORT_NO);
	
	ret = connect(fd, (SockAddr *)(&serverName), sizeof(serverName));
	if (ret)
	{
		showSocketError("conect failed");
		closesocket(fd);
		cleanup();
		return -1;
	}
	
	return fd;
}

void APIENTRY AwgDisconnect(int fd)
{
	closesocket(fd);
	cleanup();
}

/* AwgSend() returns
 * 	0  : succeed
 * 	-1 : error
 */
int APIENTRY AwgSend(int fd, char *s, int len)
{
	int ret;
	
	ret = send(fd, s, len, 0);

	if (ret != len)
	{
		printf("send error ret = %d\n", ret);
		if (ret < 0)
		{
			showSocketError("write to socket");
		}
		return -1;
	}
	return 0;
}

/* AwgSendString() returns
 * 	0  : succeed
 * 	-1 : error
 */

int APIENTRY AwgSendString(int fd, char *s)
{
	int len;
	
	len = strlen(s);
	
	dprintf("AwgSendString: send \042%s\042 (%d bytes)\n", s, len);

	return AwgSend(fd, s, len);
}


/* AwgReceive() will receive the data from AWG.
 * It returns
 * 	-1   : error
 *	else : num bytes received
 */

int APIENTRY AwgReceive(int fd, char *buf, int bufSize)
{
	int n, ret;
	fd_set	readFds;
	struct timeval	timeout;
	
	/* use select() to get timeout
	 */
	timeout.tv_sec = 2;	/* set timeout to 2 seconds */
	timeout.tv_usec = 0;

	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	n = select(FD_SETSIZE, &readFds, NULL, NULL, &timeout);
	if (0 > n)
	{
		showSocketError("select failed");
		return -1;
	}
	else if (0 == n)
	{	printf("select timeouted\n");
		return -1;
	}

	ret = recv(fd, buf, bufSize, 0);
	if (ret < 0)
	{
		showSocketError("recv failed");
		return -1;
	}
	else if (ret == 0)
	{
		printf("connection closed by server\n");
		return -1;
	}
	return ret;
}

/* AwgReceiveMessage() will receive the data until LF char is received or buffer is filled.
 * It returns
 * 	-1   : error
 *	else : num bytes received
 */

int APIENTRY AwgReceiveMessage(int fd, char *buf, int bufSize)
{
	char *bp = buf;
	char c;
	int count = 0;

	/* check savedBuffer first */

	while (savedBufferRemains > 0)
	{

		c = *savedBufferPtr++;
		*bp++ = c;
		count++;
		savedBufferRemains--;
		if (0xa == c || count >= bufSize)
		{
			return count;
		}				
	}
	/* need to loop until LF is received or buffer is filled */
	while (1)
	{
		char *p;
		int ret;

		ret = AwgReceive(fd, savedBuffer, BUFSIZE);
		if (ret < 0)
		{
			return ret;
		}
		p = savedBuffer;
		while (ret-- > 0)
		{
			c = *p++;
			*bp++ = c;
			count++;
			if (0xa == c || count >= bufSize)
			{
				savedBufferPtr = p;
				savedBufferRemains = ret;
				return count;
			}
		}
	}
}

/* AwgReceiveBinary() will receive the data until buffer is filled 
 * It returns
 *	 0   : succeed
 * 	-1   : error
 */

int APIENTRY AwgReceiveBinary(int fd, char *buf, int bufSize)
{
	char *bp = buf;
	char c;
	int count = 0;

	/* check savedBuffer first */

	while (savedBufferRemains > 0)
	{
		c = *savedBufferPtr++;
		*bp++ = c;
		count++;
		savedBufferRemains--;
		if (count >= bufSize)
		{
			return 0;
		}				
	}
	/* need to loop until buffer is filled */
	while (1)
	{
		char *p;
		int ret;

		ret = AwgReceive(fd, savedBuffer, BUFSIZE);
		if (ret < 0)
		{
			return ret;
		}
		p = savedBuffer;
		while (ret-- > 0)
		{
			c = *p++;
			*bp++ = c;
			count++;
			if (count >= bufSize)
			{
				savedBufferPtr = p;
				savedBufferRemains = ret;
				return 0;
			}
		}
	}
}

/* AwgSendFile() returns
 * 	0  : succeed
 * 	-1 : error
 */

int APIENTRY AwgSendFile(int fd, char *localFilename, char *remoteFilename)
{
	char digits[11];
	char command[100];
	int size, ret;
	
	size = getFileSize(localFilename);
	if (size <= 0)
	{
		return -1;
	}
	sprintf(digits, "%d", size);
	sprintf(command, "mmem:data \042%s\042,#%zu%s", 
			remoteFilename, strlen(digits), digits);
	ret = AwgSendString(fd, command);
	if (ret < 0)
	{
		return -1;
	}
	if (sendFromFile(fd, localFilename) < 0)
	{
		return -1;
	}
	return AwgSendString(fd, "\n");
}

/* sendFromFile() returns
 * 	0  : succeed
 * 	-1 : error
 */

static int sendFromFile(int fd, char *fname)
{
	int ret, ret2;
	FILE *fp;

	fp = fopen(fname, "r");

	if (fp == NULL)
	{
		printf("can not open %s for read\n", fname);
		perror("sendFromFile");
		return -1;
	}
	while ((ret = fread(buffer, 1, BUFSIZE, fp)) > 0)
	{
		dprintf("sendFromFile send %d bytes\n", ret);
		ret2 = AwgSend(fd, buffer, ret);
		if (ret2 < 0)
		{
			printf("AwgSend failed\n");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

/* AwgReceiveFile() returns
 * 	0  : succeed
 * 	-1 : error
 */

int APIENTRY AwgReceiveFile(int fd, char *remoteFilename, char *localFilename)
{
	char command[100], buf[20];
	int numDigits, numBytes, ret;
	
	sprintf(command, "mmem:data? \042%s\042\n", remoteFilename);
	AwgSendString(fd, command);
	
	ret = AwgReceive(fd, buf, 2);
	if (ret != 2)
	{
		printf("failed to get block header\n");
		return -1;
	}
	if (buf[0] != '#')
	{
		printf("error: first char is not '#'\n");
		return -1;
	}
	if (buf[1] < '1' || buf[1] > '9')
	{
		printf("error: invalid numDigits char %c\n",
			buf[1]);
	}
	numDigits = buf[1] - '0';
	ret = AwgReceive(fd, buf, numDigits);
	if (ret != numDigits)
	{
		printf("failed to get numDigits\n");
		return -1;
	}
	buf[numDigits] = 0;
	numBytes = atoi(buf);
	receiveIntoFile(fd, localFilename, numBytes);
	/* receive and dispose terminator char (LF) */
	ret = AwgReceive(fd, buf, 1);
	if (ret != 1)
	{
		printf("failed to get terminator char (LF)\n");
		return -1;
	}
	return 0;
}

/* receiveIntoFile() returns
 * 	0  : succeed
 * 	-1 : error
 */
 
static int receiveIntoFile(int fd, char *fname, int numBytes)
{
	FILE *fp;
	int size, ret;
	
	fp = fopen(fname, "w");

	if (fp == NULL)
	{
		printf("can not open %s for write\n", fname);
		perror("receiveIntoFile");
	}

	while (numBytes > 0)
	{
		if (numBytes < BUFSIZE)
		{
			size = numBytes;
		}
		else
		{
			size = BUFSIZE;
		}
		ret = AwgReceive(fd, buffer, size);
		if (ret <= 0)
		{
			fclose(fp);
			return -1;
		}
		/* NOTE: recv() does not fill entire buffer always */
		if (fwrite(buffer, 1, ret, fp) != (unsigned int)ret)
		{
			perror("fwrite failed\n");
			fclose(fp);
			return -1;
		}
		numBytes -= ret;
	}
	fclose(fp);
	return 0;
}

/* getFileSize() returns
 * 	-1   : error
 * 	else : size of the file in bytes
 */

static int getFileSize(char *fname)
{
	struct stat statBuf;
	int ret = stat(fname, &statBuf);
	if (ret != 0)
	{
		printf("stat %s failed\n", fname);
		perror("stat");
		return -1;
	}
	return statBuf.st_size;
}


static void showSocketError(char *s)
{
	perror(s);
}

