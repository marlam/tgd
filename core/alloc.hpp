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

#if __has_include (<sys/mman.h>)
# include <sys/mman.h>
# include <fcntl.h>
# include <unistd.h>
# include <system_error>
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

    /*! \brief Returns whether this allocator works on this system (it does when mmap is available). */
    constexpr static bool isAvailableOnThisSystem()
    {
#if __has_include (<sys/mman.h>)
        return true;
#else
        return false;
#endif
    }

    /*! \brief Allocates n bytes. */
    virtual unsigned char *allocate(std::size_t n) const override
    {
#if __has_include (<sys/mman.h>)
        int fd = -1;
        switch (_type) {
        case Private:
            fd = open(_name.c_str(), O_TMPFILE | O_RDWR, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
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
            throw std::system_error(std::error_code(errno, std::system_category()), "cannot open tgd data file");
        }
        if (_type == Private || _type == NewFile) {
            if (ftruncate(fd, n) != 0) {
                (void)close(fd);
                throw std::system_error(std::error_code(errno, std::system_category()), "cannot set size of tgd data file");
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
            throw std::system_error(std::error_code(errno, std::system_category()), "cannot mmap tgd data file");
        }
        if (close(fd) != 0) {
            (void)munmap(ptr, n);
            throw std::system_error(std::error_code(errno, std::system_category()), "cannot close tgd data file");
        }
        return static_cast<unsigned char*>(ptr);
#else
        throw std::system_error(std::error_code(EINVAL, std::system_category()), "mmap is not available on this system");
#endif
    }

    /*! \brief Returns a function that deallocates n bytes at the given pointer. */
    virtual std::function<void (unsigned char* p)> getDeallocator(std::size_t n) const override
    {
#if __has_include (<sys/mman.h>)
        return [=](unsigned char* p) { (void)munmap(p, n); };
#else
        return [](unsigned char*) { };
#endif
    }

    /*! \brief Returns whether this allocator clears allocated memory. */
    virtual bool clearsMemory() const override
    {
        return (_type == Private || _type == NewFile);
    }
};

}
