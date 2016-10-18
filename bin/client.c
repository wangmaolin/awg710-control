/* 
 * This is a client program to control the AWG series by LAN.
 * This program can be compiled and executed on generic unix system and 
 * WIN32 (Windows 95/98/NT/2000/Me)
 * environment.
 * This program is tested both with Windows2000 with MSVC++6.0 SP5 and 
 * SunOS Release 4.1.3 with gcc 2.3.3.
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"

void main(int argc, char *argv[])
{
	char hostname[256];
	int fd, ret;
	
	if (argc > 1)
	{
		strcpy(hostname, argv[1]);
	}
	else
	{
		printf("input server name or ip-address: ");
		gets(hostname);
	}

	if ((fd = AwgConnect(hostname)) < 0)
	{
		printf("AwgConnect: failed\n");
		exit(1);
	}
	
	printf("AwgConnect succeeded\n");

	while (1)
	{
		char line[256], arg1[256], arg2[256];
		int nArgs;
		
		printf("command: ");
		if (gets(line) == NULL)
		{
			break;
		}
		if (*line == '?' || *line == 'h')
		{
			printf("s command: send command (example: s *idn?)\n"
				"r: receive\n"
				"l: receive until LF\n"
				"S local_file [remote_file]: send file "
						"(example: S test.wfm)\n"
				"R remote_file [local_file]: receive file "
						"(example: R test.wfm)\n"
				"q: quit\n"
				"? or h: show this help\n");
		}
		else if (*line == 's')
		{
			int len;
			char *arg;

			if (line[1] == 0)
			{
				printf("need an argument\n");
				continue;
			}
			arg = line + 2;
			len = strlen(arg);
			if ('\\' == arg[len - 1])
			{
				/* do not append LF */
				arg[--len] = 0;
			}
			else
			{
				arg[len++] = '\n';
				arg[len] = 0;
			}
			AwgSendString(fd, arg);
		}
		else if (*line ==  'r')
		{
			ret = AwgReceive(fd, line, sizeof(line) - 1);
			if (ret <= 0)
			{
				/* error */
				continue;
			}
			line[ret] = 0;
			printf("%s", line);
			fflush(stdout);
		}
		else if (*line ==  'l')
		{
			ret = AwgReceiveMessage(fd, line, sizeof(line) - 1);
			if (ret <= 0)
			{
				/* error */
				continue;
			}
			line[ret] = 0;
			printf("%s", line);
			fflush(stdout);
		}
		else if (*line == 'S')
		{
			nArgs = sscanf(line + 1, "%s %s", arg1, arg2);
			if (nArgs == 1)
			{
				strcpy(arg2, arg1);
			}
			else if (nArgs != 2)
			{
				printf("need at least one argument\n");
				continue;
			}
			AwgSendFile(fd, arg1, arg2);
		}
		else if (*line == 'R')
		{
			nArgs = sscanf(line + 1, "%s %s", arg1, arg2);
			if (nArgs == 1)
			{
				strcpy(arg2, arg1);
			}
			else if (nArgs != 2)
			{
				printf("need at least one argument\n");
				continue;
			}
			AwgReceiveFile(fd, arg1, arg2);
		}
		else if (*line == 'q')
		{
			break;
		}
		else
		{
			printf("unknown command %c\n", *line);
		}
	}

	AwgDisconnect(fd);
}

