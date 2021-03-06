#ifndef _MMAPFILE_H
#define _MMAPFILE_H
#include "halAlignmentInstance.h"
#include "halDefs.h"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace hal {
    /* Current API major and minor versions */
    static const unsigned MMAP_API_MAJOR_VERSION = 1;
    static const unsigned MMAP_API_MINOR_VERSION = 1;

    /* get current mmap version as a string */
    const std::string& getMmapCurentVersion();
    
    /* Null offset constant.  So happens the header is at location zero, but we never have
     * a offset to it */
    static const size_t MMAP_NULL_OFFSET = 0;

    /* header for the file */
    struct MMapHeader {
        char format[32];
        char mmapVersion[32];
        char halVersion[32];
        size_t nextOffset;
        size_t rootOffset;
        bool dirty;
        char _reserved[256];   // 256 bytes of reserved added in mmap API 1.1
    };
    typedef struct MMapHeader MMapHeader;

    /**
     * An mmapped HAL file.  This handles creation and opening of mapped
     * file.
     * WARNING: When writing, close() must be explicitly called or file will
     * be left marked as dirty.
     */
    class MMapFile {
        friend class MMapAlignment;

      public:
        /* check if first bit of file has MMAP header */
        static bool isMmapFile(const std::string &initialBytes);

        /* get the mmap version of this file */
        const std::string getVersion() const {
            return _version;
        }
        /* get the mmap major version of this file */
        unsigned getMajorVersion() const {
            return _majorVersion;
        }
        
        /* get the mmap minor version of this file */
        unsigned getMinorVersion() const {
            return _minorVersion;
        }
        
        std::string getStorageFormat() const {
            return STORAGE_FORMAT_MMAP;
        }

        virtual bool isUdcProtocol() const = 0;

        inline size_t getRootOffset() const;
        inline void *toPtr(size_t offset, size_t accessSize);
        inline const void *toPtr(size_t offset, size_t accessSize) const;
        inline size_t allocMem(size_t size, bool isRoot = false);
        bool isReadOnly() const {
            return !(_mode & WRITE_ACCESS);
        };
        std::string getVersion() {
            return _header->halVersion;
        };
        virtual ~MMapFile() {
        }
        /* round up to alignment size */
        static size_t alignRound(size_t size) {
            return ((size + (sizeof(size_t) - 1)) / sizeof(size_t)) * sizeof(size_t);
        }

      protected:
        MMapFile(const std::string alignmentPath, unsigned mode, bool mustFetch);
        /** close marks as clean, don't call on error, just delete */
        virtual void close() = 0;
        virtual void fetch(size_t offset, size_t accessSize) const {
            // no-op by default
        }

        void setHeaderPtr();
        void createHeader();
        void loadHeader(bool markDirty);
        void validateWriteAccess() const;
        inline void fetchIfNeeded(size_t offset, size_t accessSize) const;

        const std::string _alignmentPath; // name of file for errors
        unsigned _mode;                   // access mode
        void *_basePtr;                   // location file is mapped
        MMapHeader *_header;              // pointer to header
        size_t _fileSize;                 // size of file
        bool _mustFetch;                  // fetch must be called on each access.

      private:
        MMapFile() {
            // no copying
        }
        void parseCheckVersion();

        static MMapFile *factory(const std::string &alignmentPath, unsigned mode = READ_ACCESS,
                                 size_t fileSize = MMAP_DEFAULT_FILE_SIZE);

        std::string _version;
        unsigned _majorVersion;
        unsigned _minorVersion;
        
    };
}

/** Get the offset of the root object */
size_t hal::MMapFile::getRootOffset() const {
    assert(_header->rootOffset > 0);
    return _header->rootOffset;
}

/* fetch the range if required, else inline no-op */
void hal::MMapFile::fetchIfNeeded(size_t offset, size_t accessSize) const {
    if (_mustFetch) {
        fetch(offset, accessSize);
    }
}

/** Get pointer to the root a pointer.  Where accessSize is the
 * number of bytes that will be accessed, which is used when
 * pre-fetching is needed. If accessing an array, accessSize is size
 * of element, not the entire array.*/
void *hal::MMapFile::toPtr(size_t offset, size_t accessSize) {
    fetchIfNeeded(offset, accessSize);
    return static_cast<char *>(_basePtr) + offset;
}

const void *hal::MMapFile::toPtr(size_t offset, size_t accessSize) const {
    fetchIfNeeded(offset, accessSize);
    return static_cast<const char *>(_basePtr) + offset;
}

/** Allocate new memory, resize file if necessary. If isRoot is specified, it
 * is stored as the root used to find all object.  */
size_t hal::MMapFile::allocMem(size_t size, bool isRoot) {
    validateWriteAccess();
    if (_header->nextOffset + size > _fileSize) {
        throw hal_exception("mmap file is full, specify file size larger than " + std::to_string(_fileSize));
    }
    size_t offset = _header->nextOffset;
    _header->nextOffset += alignRound(size);
    if (isRoot) {
        _header->rootOffset = offset;
    }
    return offset;
}

#endif

// Local Variables:
// mode: C ++
// End
// Local Variables:
// mode: c++
// End:
