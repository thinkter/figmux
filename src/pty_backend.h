#ifndef PTY_BACKEND_H
#define PTY_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct PtyBackend {
	int masterFd;
	pid_t childPid;
	bool isOpen;
} PtyBackend;

bool PtyBackend_Spawn(PtyBackend *backend, const char *shellPath, int columns, int rows);
void PtyBackend_Close(PtyBackend *backend);
ssize_t PtyBackend_Read(PtyBackend *backend, char *buffer, size_t bufferSize);
ssize_t PtyBackend_Write(PtyBackend *backend, const char *buffer, size_t bufferSize);
bool PtyBackend_Resize(PtyBackend *backend, int columns, int rows);
bool PtyBackend_IsChildRunning(PtyBackend *backend);

#endif
