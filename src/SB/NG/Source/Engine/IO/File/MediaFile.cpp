// MediaFile.cpp -- eight functions, read from the image with
// tools/disasm.py. A media file is a MediaObject with file data, a
// callback list and the handler for its target device. Reset clears the
// callbacks, the handler and the error; OpenAsync finds the handler for
// the target (an error message and false when there is none), acquires
// an open command from the handler's queue, fully qualifies the name
// into it, sets the flags and priority and commits; CloseAsync,
// ReadAsync and SeekAsync acquire, fill and commit their own commands
// the same way, each returning true. GetSize reads the file data's size
// and GetReadSize rounds a size up to 32 bytes. MediaError::SetMsg is a
// varargs function whose formatting was compiled out: the register
// save area the compiler emits for any `...` function is all of it.
//
// Layouts from the DWARF (tools/dwarf_types.py): MediaFile 0x68 on
// MediaObject 0x58 (the error code first), file data at +0x58,
// callbacks at +0x60, handler at +0x64; MediaHandler's queue at +0x4;
// MediaCmd's eight-byte header then a union of payloads, the open
// payload a 256-byte name with the flags and priority after it. The
// error message is the unity unit's pool, hence the generated header.

#include "SB/NG/Source/Engine/IO/File/MediaFile.pool.h"

namespace IO {

enum enMediaTarget {
    enMediaTarget_ = 0x7FFFFFFF
};

enum enMediaCmdType {
    MEDIA_CMD_FILE_OPEN = 513,
    MEDIA_CMD_FILE_CLOSE = 514,
    MEDIA_CMD_FILE_SEEK = 515,
    MEDIA_CMD_FILE_READDATA = 516
};

class MediaError {
public:
    void SetMsg(const char* fmt, ...);

    int errCode;
};

class MediaObject {
public:
    enum enPriority {
        enPriority_ = 0x7FFFFFFF
    };

    MediaError error;
    unsigned char _pad0[0x54];
};

class MediaCmdOpen {
public:
    void FullyQualify(char* dst, int size, const char* name);

    char filename[256];
    int openFlags;
    MediaObject::enPriority priority;
};

class MediaCmdRead {
public:
    char* tgtBuffer;
    int numBytesToRead;
};

class MediaCmd;

class CmdQueue {
public:
    MediaCmd* CmdAcquire(enMediaCmdType type, MediaObject* object);
    void CmdCommit(MediaCmd* cmd);
};

class MediaHandler {
public:
    enMediaTarget tgtDevice;
    CmdQueue* cmdQueue;
};

class MediaMogul {
public:
    static MediaHandler* FindHandler(enMediaTarget target);
};

class MediaCallback;

class FileData {
public:
    int size;
    int _pad0;
};

class MediaFile : public MediaObject {
public:
    enum enSeekOrigin {
        enSeekOrigin_ = 0x7FFFFFFF
    };

    void Reset();
    bool OpenAsync(const char* name, int flags, enMediaTarget target,
                   MediaObject::enPriority priority);
    bool CloseAsync();
    bool ReadAsync(char* buffer, int size);
    bool SeekAsync(enSeekOrigin origin, int offset);
    int GetSize();
    int GetReadSize(int size);

    FileData fileData;
    MediaCallback* callbacks;
    MediaHandler* handler;
};

class MediaCmdSeek {
public:
    MediaFile::enSeekOrigin seekType;
    int offset;
};

class MediaCmd {
public:
    unsigned char header[8];
    union {
        MediaCmdOpen openInfo;
        MediaCmdRead readInfo;
        MediaCmdSeek seekInfo;
    };
};

}  // namespace IO

void IO::MediaFile::Reset() {
    callbacks = 0;
    handler = 0;
    error.errCode = 0;
}

bool IO::MediaFile::OpenAsync(const char* name, int flags, enMediaTarget target,
                              MediaObject::enPriority priority) {
    error.errCode = 0;

    handler = MediaMogul::FindHandler(target);

    if (!handler) {
        error.SetMsg("Unhandled target device: %d(file: %s)", target, name);
        return false;
    }

    CmdQueue* queue = handler->cmdQueue;
    MediaCmd* cmd = queue->CmdAcquire(MEDIA_CMD_FILE_OPEN, this);

    cmd->openInfo.FullyQualify(cmd->openInfo.filename, 256, name);
    cmd->openInfo.openFlags = flags;
    cmd->openInfo.priority = priority;

    queue->CmdCommit(cmd);

    return true;
}

// An empty varargs body: the compiler saves the argument registers for
// any `...` function, and a va_start would add the list's three stores
// and 16 bytes of frame that retail does not have (measured).
void IO::MediaError::SetMsg(const char*, ...) {
}

bool IO::MediaFile::CloseAsync() {
    error.errCode = 0;

    CmdQueue* queue = handler->cmdQueue;
    MediaCmd* cmd = queue->CmdAcquire(MEDIA_CMD_FILE_CLOSE, this);

    queue->CmdCommit(cmd);

    return true;
}

bool IO::MediaFile::ReadAsync(char* buffer, int size) {
    error.errCode = 0;

    CmdQueue* queue = handler->cmdQueue;
    MediaCmd* cmd = queue->CmdAcquire(MEDIA_CMD_FILE_READDATA, this);

    cmd->readInfo.tgtBuffer = buffer;
    cmd->readInfo.numBytesToRead = size;

    queue->CmdCommit(cmd);

    return true;
}

bool IO::MediaFile::SeekAsync(enSeekOrigin origin, int offset) {
    error.errCode = 0;

    CmdQueue* queue = handler->cmdQueue;
    MediaCmd* cmd = queue->CmdAcquire(MEDIA_CMD_FILE_SEEK, this);

    cmd->seekInfo.seekType = origin;
    cmd->seekInfo.offset = offset;

    queue->CmdCommit(cmd);

    return true;
}

int IO::MediaFile::GetSize() {
    return fileData.size;
}

int IO::MediaFile::GetReadSize(int size) {
    return (size + 31) & ~31;
}
