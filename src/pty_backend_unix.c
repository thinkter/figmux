#define _POSIX_C_SOURCE 200809L

#include "pty_backend.h"

#if defined(__linux__) || defined(__APPLE__)

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__linux__)
#include <pty.h>
#elif defined(__APPLE__)
#include <util.h>
#endif

static void PtyBackend_Reset(PtyBackend *backend)
{
	backend->masterFd = -1;
	backend->childPid = -1;
	backend->isOpen = false;
}

bool PtyBackend_Spawn(PtyBackend *backend, const char *shellPath, int columns, int rows)
{
	PtyBackend_Reset(backend);

	struct winsize size = {
		.ws_row = (unsigned short)rows,
		.ws_col = (unsigned short)columns,
		.ws_xpixel = 0,
		.ws_ypixel = 0
	};

	pid_t childPid = forkpty(&backend->masterFd, NULL, NULL, &size);
	if (childPid < 0)
	{
		PtyBackend_Reset(backend);
		return false;
	}

	if (childPid == 0)
	{
		const char *shell = shellPath;
		if (shell == NULL || shell[0] == '\0')
		{
			shell = getenv("SHELL");
		}
		if (shell == NULL || shell[0] == '\0')
		{
			shell = "/bin/sh";
		}

		execlp(shell, shell, (char *)NULL);
		_exit(127);
	}

	int flags = fcntl(backend->masterFd, F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(backend->masterFd, F_SETFL, flags | O_NONBLOCK);
	}

	backend->childPid = childPid;
	backend->isOpen = true;
	return true;
}

void PtyBackend_Close(PtyBackend *backend)
{
	if (!backend->isOpen)
	{
		return;
	}

	if (backend->masterFd >= 0)
	{
		close(backend->masterFd);
	}

	if (backend->childPid > 0)
	{
		kill(backend->childPid, SIGTERM);
		waitpid(backend->childPid, NULL, 0);
	}

	PtyBackend_Reset(backend);
}

ssize_t PtyBackend_Read(PtyBackend *backend, char *buffer, size_t bufferSize)
{
	if (!backend->isOpen || backend->masterFd < 0)
	{
		return -1;
	}

	ssize_t bytesRead = read(backend->masterFd, buffer, bufferSize);
	if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
		return 0;
	}

	return bytesRead;
}

ssize_t PtyBackend_Write(PtyBackend *backend, const char *buffer, size_t bufferSize)
{
	if (!backend->isOpen || backend->masterFd < 0)
	{
		return -1;
	}

	size_t totalWritten = 0;
	while (totalWritten < bufferSize)
	{
		ssize_t bytesWritten = write(backend->masterFd, buffer + totalWritten, bufferSize - totalWritten);
		if (bytesWritten > 0)
		{
			totalWritten += (size_t)bytesWritten;
			continue;
		}

		if (bytesWritten < 0 && errno == EINTR)
		{
			continue;
		}

		if (bytesWritten < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			struct pollfd pollFd = {
				.fd = backend->masterFd,
				.events = POLLOUT,
				.revents = 0
			};

			int pollResult = poll(&pollFd, 1, -1);
			if (pollResult > 0)
			{
				continue;
			}

			if (pollResult < 0 && errno == EINTR)
			{
				continue;
			}
		}

		return totalWritten > 0 ? (ssize_t)totalWritten : -1;
	}

	return (ssize_t)totalWritten;
}

bool PtyBackend_Resize(PtyBackend *backend, int columns, int rows)
{
	if (!backend->isOpen || backend->masterFd < 0)
	{
		return false;
	}

	struct winsize size = {
		.ws_row = (unsigned short)rows,
		.ws_col = (unsigned short)columns,
		.ws_xpixel = 0,
		.ws_ypixel = 0
	};

	return ioctl(backend->masterFd, TIOCSWINSZ, &size) == 0;
}

bool PtyBackend_IsChildRunning(PtyBackend *backend)
{
	if (!backend->isOpen || backend->childPid <= 0)
	{
		return false;
	}

	int status = 0;
	pid_t result = waitpid(backend->childPid, &status, WNOHANG);
	if (result == 0)
	{
		return true;
	}

	backend->childPid = -1;
	return false;
}

#else

bool PtyBackend_Spawn(PtyBackend *backend, const char *shellPath, int columns, int rows)
{
	(void)backend;
	(void)shellPath;
	(void)columns;
	(void)rows;
	return false;
}

void PtyBackend_Close(PtyBackend *backend)
{
	(void)backend;
}

ssize_t PtyBackend_Read(PtyBackend *backend, char *buffer, size_t bufferSize)
{
	(void)backend;
	(void)buffer;
	(void)bufferSize;
	return -1;
}

ssize_t PtyBackend_Write(PtyBackend *backend, const char *buffer, size_t bufferSize)
{
	(void)backend;
	(void)buffer;
	(void)bufferSize;
	return -1;
}

bool PtyBackend_Resize(PtyBackend *backend, int columns, int rows)
{
	(void)backend;
	(void)columns;
	(void)rows;
	return false;
}

bool PtyBackend_IsChildRunning(PtyBackend *backend)
{
	(void)backend;
	return false;
}

#endif
