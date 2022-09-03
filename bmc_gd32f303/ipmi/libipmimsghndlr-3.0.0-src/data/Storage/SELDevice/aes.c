/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2012, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: aes.c
*
* Descriptions: Code that connects to AES engine and sends any 
* new SEL event generated 
*   
*
* Author: Manish Tomar (manisht@ami.com)
*
******************************************************************/

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>

#include "aes.h"
#include "featuredef.h"

#define AES_PATH "/tmp/aesevents"
#define MAX_TRIES 3

static int _g_tries = 0;

static int _connect()
{
	int sock = -1, ret;
	int flags;
	int optval = -1;
	socklen_t optlen = sizeof(optval);
	char *errstr, errbuf[128];
	struct sockaddr_un addr;
	struct pollfd pfd = { 0 };
	
	//errno = 0;
	
	// create socket
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, AES_PATH, sizeof(addr.sun_path));
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		errstr = "error creating Unix Socket";
		goto error;
	}
	
	// set non-block
	flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	
	// connect it
	if (0 == connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		return sock;
	}
	
	// connection failed. wait if in progress
	if (errno != EINPROGRESS) {
		errstr = "Connect failed";
		goto error;
	}
	
	// Wait for max of 5 milliseconds
	pfd.fd = sock;
	pfd.events = POLLOUT;
	if (-1 == (ret = poll(&pfd, 1, 5))) {
		errstr = "Error polling";
		goto error;
	}
	if (ret == 0) {
		errstr = "poll timed out";
		goto error;
	}
	if (ret != 1) {
		errstr = "unexpected poll return";
		goto error;
	}
	if (!(pfd.revents & POLLOUT)) {
		errstr = "unexected event on fd";
		goto error;
	}
	if (-1 == getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
		errstr = "error from getsockopt";
		goto error;
	}
	if (optval != 0) {
		errstr = "error connecting to aes";
		goto error;
	}
	
	return sock;
	
error:
	memset(errbuf, 0, sizeof(errbuf));
	strerror_r(errno, errbuf, sizeof(errbuf));
	fprintf(stderr, "%s: %s\n", errstr, errbuf);
	if (sock != -1)
		close(sock);
	return -1;
}


int AES_SendEvent(SELEventRecord_T *event)
{
	static int sock = -1;	// UDS connected socket
    static int enabled = -1;
	ssize_t sent;
	
    // check if AES is enabled in stack
    if (enabled == -1) {
        enabled = IsFeatureEnabled("CONFIG_SPX_FEATURE_AUTOMATION_ENGINE");
    }
    if (!enabled)
        return 0;

resend:
	if (sock == -1) {
		// First time called. Make UDS connection with small timeout
		if (_g_tries > MAX_TRIES) {
			fprintf(stderr, "AES connection attempts exceeded max tries\n");
			return -1;
		}
		sock = _connect();
		if (sock == -1) {
			_g_tries++;
			return -1;
		}
	}
	
	// Send the event using existing socket connection
	if (sizeof(*event) != (sent = send(sock, event, sizeof(*event), MSG_NOSIGNAL))) {
		if (sent == -1) {
			if (errno == ECONNRESET || errno == EPIPE) {
				if(sock != -1)
					close(sock);
				// Could happen if aes engine is restarted
				sock = -1;
				_g_tries = 0;
				goto resend;
			}
			char buf[128];
			strerror_r(errno, buf, sizeof(buf));
			fprintf(stderr, "AES: Error sending data: %s\n", buf);
		}
		else {
			fprintf(stderr, "AES: Partial data sent. UNHANDLED AND UNEXPECTED\n");
		}
		if(sock != -1)
			close(sock);
		// setting sock to -1 to reconnect next time
		// TODO: But this event will not be sent. May need to change to reconnect and send this event
		return (sock = -1);
	}
	
	return 0;
}
