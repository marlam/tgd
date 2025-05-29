/*
 * Copyright (C) 2025 Martin Lambers <marlam@marlam.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if __has_include (<sys/mman.h>)
# include <sys/mman.h>
# include <fcntl.h>
# include <unistd.h>
# include <filesystem>
#endif

/**
 * \file alloc.hpp
 * \brief Custom allocators for array data.
 */

namespace TGD {

/*! \brief The default allocator based on new[] and delete[]. */
class Allocator
{
public:

    /*! \brief Constructor. */
    Allocator()
    {
    }

    /*! \brief Destructor. */
    virtual ~Allocator()
    {
    }

    /*! \brief Allocates \a n bytes via new[]. */
    virtual unsigned char* allocate(std::size_t n) const
    {
        return new unsigned char[n];
    }

    /*! \brief Returns a function that deallocates memory via delete[]. */
    virtual std::function<void (unsigned char* p)> getDeallocator(std::size_t /* n */) const
    {
        return [](unsigned char* p) { delete[] p; };
    }

    /*! \brief Returns whether this allocator clears allocated memory. */
    virtual bool clearsMemory() const
    {
        return false;
    }
};

/*! \brief An mmap-based allocator that allows to work with arrays that do not fit into main memory. */
class MmapAllocator final : public Allocator
{
public:
    enum Type {
        Private,                /**< Allocation in a specific directory, without a visible file name. */
        NewFile,                /**< Shared allocation in a newly created file with a given name. */
        ExistingFileReadOnly,   /**< Shared allocation in an existing file, read only. */
        ExistingFileReadWrite   /**< Shared allocation in an existing file, read and write. */
    };

private:
    const std::string _name;
    const Type _type;

public:
    /*! \brief Constructor for a private mmap allocator.
     *
     * \param dirName   Directory in which the private file is created.
     *
     * Creates a private temporary file in the given directory.
     * Typical values for \a dirName are "." for the current directory,
     * or a directory taken from the TMPDIR environment variable, if
     * available.
     */
    MmapAllocator(const std::string& dirName = ".") :
        _name(dirName), _type(Private)
    {
    }

    /*! \brief Constructor for a named-file mmap allocator.
     *
     * \param fileName  Name of the file.
     * \param type      Type of file usage.
     *
     * The \a type can be NewFile, which will create a new file (or possibly
     * overwrites an existing file with the same name),
     * or ExistingFileReadOnly, which will open the specified file
     * in read-only mode, or ExistingFileReadWrite, which will
     * open the specified file in read-write mode.
     */
    MmapAllocator(const std::string& fileName, Type type) :
        _name(fileName), _type(type)
    {
    }

    /*! \brief Destructor. */
    virtual ~MmapAllocator()
    {
    }

    /*! \brief Allocates n bytes. */
    virtual unsigned char *allocate(std::size_t n) const override
    {
#if __has_include (<sys/mman.h>)
        int fd = -1;
        switch (_type) {
        case Private:
            {
                bool oTmpfileSupported = true;
# ifdef O_TMPFILE
                fd = open(_name.c_str(), O_TMPFILE | O_RDWR, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
                if (fd < 0 && errno == EOPNOTSUPP)
                    oTmpfileSupported = false;
# else
                oTmpfileSupported = false;
# endif
                if (!oTmpfileSupported) {
                    std::vector<char> tmpl(_name.length() + 11 + 1);
                    strcpy(tmpl.data(), _name.c_str());
                    strcat(tmpl.data(), "/TGD-XXXXXX");
                    fd = mkstemp(tmpl.data());
                    if (fd != -1)
                        unlink(tmpl.data());
                }
            }
            break;
        case NewFile:
            fd = open(_name.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
            break;
        case ExistingFileReadOnly:
            fd = open(_name.c_str(), O_RDONLY);
            break;
        case ExistingFileReadWrite:
            fd = open(_name.c_str(), O_RDWR);
            break;
        }
        if (fd < 0) {
            throw std::filesystem::filesystem_error(
                    (  _type == Private ? "Cannot create temporary tgd data file in directory"
                     : _type == NewFile ? "Cannot create tgd data file"
                     : "Cannot open tgd data file"), _name.c_str(),
                    std::error_code(errno, std::system_category()));
        }
        if (_type == Private || _type == NewFile) {
            if (ftruncate(fd, n) != 0) {
                (void)close(fd);
                throw std::filesystem::filesystem_error(
                        (_type == Private ? "Cannot set size of temporary tgd data file in directory"
                         : "Cannot set size of tgd data file"), _name.c_str(),
                        std::error_code(errno, std::system_category()));
            }
        }
        int mmapProt = PROT_READ;
        if (_type != ExistingFileReadOnly)
            mmapProt |= PROT_WRITE;
        // For type Private, the obvious idea would be to mmap() with MAP_PRIVATE.
        // However, this does not seem to support mapping regions that are larger
        // than main memory, which defeats the purpose of MmapAllocator. So we
        // always use MAP_SHARED.
        void* ptr = mmap(nullptr, n, mmapProt, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            (void)close(fd);
            throw std::filesystem::filesystem_error(
                    (_type == Private ? "Cannot mmap temporary tgd data file in directory"
                     : "Cannot mmap tgd data file"), _name.c_str(),
                    std::error_code(errno, std::system_category()));
        }
        if (close(fd) != 0) {
            (void)munmap(ptr, n);
            throw std::filesystem::filesystem_error(
                    (_type == Private ? "Cannot close temporary tgd data file in directory"
                     : "Cannot close tgd data file"), _name.c_str(),
                    std::error_code(errno, std::system_category()));
        }
        return static_cast<unsigned char*>(ptr);
#else
        // fall back to new[]
        unsigned char* ptr = new unsigned char[n];
        if (_type == Private || _type == NewFile) {
            // clear memory, as mmap would do on a new file
            std::memset(ptr, 0, n);
        } else {
            // read the file into the array
            FILE* f = fopen(_name.c_str(), "rb");
            if (!f || fread(ptr, n, 1, f) != 1) {
                delete[] ptr;
                throw std::filesystem::filesystem_error(
                        "Cannot read tgd data file", _name.c_str(),
                        std::error_code((!f || ferror(f) ? errno : EINVAL), std::system_category()));
            }
            fclose(f);
        }
        return ptr;
#endif
    }

    /*! \brief Returns a function that deallocates n bytes at the given pointer. */
    virtual std::function<void (unsigned char* p)> getDeallocator(std::size_t n) const override
    {
#if __has_include (<sys/mman.h>)
        return [=](unsigned char* p) { (void)munmap(p, n); };
#else
        // fall back to delete[]
        return [](unsigned char* p) { delete[] p; };
#endif
    }

    /*! \brief Returns whether this allocator clears allocated memory. */
    virtual bool clearsMemory() const override
    {
        return (_type == Private || _type == NewFile);
    }
};

}
