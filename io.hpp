/*
 * Copyright (C) 2018, 2019 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
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

#ifndef TAD_IO_HPP
#define TAD_IO_HPP

/**
 * \file io.hpp
 * \brief Import and export of arrays to/from files and streams.
 */

#include <cerrno>
#include <cstring>
#include <memory>

#include "array.hpp"

namespace TAD {

/*! \brief Input/output errors. */
enum Error {
    /*! \brief No error */
    ErrorNone = 0,
    /*! \brief Unsupported file format */
    ErrorFormatUnsupported = 1,
    /*! \brief Unsupported features used by the file */
    ErrorFeaturesUnsupported = 2,
    /*! \brief File format requires hints that are missing */
    ErrorMissingHints = 3,
    /*! \brief Invalid data */
    ErrorInvalidData = 4,
    /*! \brief Seeking not supported */
    ErrorSeekingNotSupported = 5,
    /*! \brief Appending not supported */
    ErrorAppendingNotSupported = 6,
    /*! \brief Failure of external library */
    ErrorLibrary = 7,
    /*! \brief System error, consult errno */
    ErrorSysErrno = 8,
};

/*! \brief Convert an input/output error to human-readable string */
const char* strerror(Error e);

/*! \cond */
// This is the interface that file format converters must implement
class FormatImportExport {
public:
    FormatImportExport() {}
    virtual ~FormatImportExport() {}

    virtual Error openForReading(const std::string& fileName, const TagList& hints) = 0;
    virtual Error openForWriting(const std::string& fileName, bool append, const TagList& hints) = 0;
    virtual Error close() = 0;

    // for reading: (it is guaranteed that the file is opened for reading when one of these is called)
    virtual int arrayCount() = 0;
    virtual ArrayContainer readArray(Error* error, int arrayIndex = -1 /* -1 means next */) = 0;
    virtual bool hasMore() = 0;

    // for writing / appending: (it is guaranteed that the file is opened for writing when one of these is called)
    virtual Error writeArray(const ArrayContainer& array) = 0;
};
#if 0
extern "C" FormatImportExport* FormatImportExportFactory();
#endif
/*! \endcond */

/*! \brief The importer class imports arrays from files or streams. */
class Importer {
private:
    std::string _fileName;
    TagList _hints;
    std::string _format;
    std::shared_ptr<FormatImportExport> _fie;
    bool _fileIsOpened;

public:
    /*! \brief Constructor. This must be initialized with \a initialize(). */
    Importer();

    /*! \brief Constructor.
     *
     * The file name is required.
     * The special file name "-" is interpreted as standard input.
     * The optional \a hints may be useful depending on the file format.
     * For example, raw files contain no information about array dimension or type,
     * so the hints must contain the tags COMPONENTS and TYPE as well as SIZE (for 1D arrays),
     * WIDTH and HEIGHT (for 2D arrays), WIDTH, HEIGHT and DEPTH (for 3D arrays) or DIMENSIONS,
     * DIMENSION0, DIMENSION1, ... (for arrays of arbitrary dimension).
     *
     * Note that this initialization does not try to open the file yet, it merely
     * sets up the necessary information (and thus cannot fail). The functions that
     * access the data will report any errors that might occur. If you want to
     * perform some lightweight checks without accessing the file contents, use the
     * checkAccess() function.
     */
    Importer(const std::string& fileName, const TagList& hints = TagList());

    /*!\brief Destructor. */
    ~Importer();

    /*! \brief Initialize. See the constructor documentation. */
    void initialize(const std::string& fileName, const TagList& hints = TagList());

    /*! \brief Returns the file name. */
    const std::string& fileName() const
    {
        return _fileName;
    }

    /*! \brief Checks if the file is accessible and the format is supported. This function
     * does not access the file contents yet, and does not result in an open file descriptor. */
    Error checkAccess() const;

    /*! \brief Returns the number of arrays in this file. Returns -1 if that information is not
     * available, e.g. if the file is actually a stream or if the file format does not provide
     * that information. */
    int arrayCount();

    /*! \brief Read an array from the file and return it. On error, the error code will be set (if \a error is not
     * nullptr) and a null array will be returned.
     *
     * By default, the next array in the file will be read, but you can
     * give an index to chose the array you want (this only works if \a arrayCount() returns something
     * other than -1).
     */
    ArrayContainer readArray(Error* error = nullptr, int arrayIndex = -1 /* -1 means next */);

    /*! \brief Returns whether there are more arrays in the file, i.e. whether you can read the next array
     * with \a readArray(). This information is always available, for all file formats and also for streams.
     * If (and only if) this function returns false, then it sets the error code. */
    bool hasMore(Error* error = nullptr);
};

/*! \brief Flag to be used for the append parameter of TAD::save() */
const bool Append = true;
/*! \brief Flag to be used for the append parameter of TAD::save() */
const bool Overwrite = false;

/*! \brief The exporter class exports arrays to files or streams. */
class Exporter {
private:
    std::string _fileName;
    bool _append;
    TagList _hints;
    std::string _format;
    std::shared_ptr<FormatImportExport> _fie;
    bool _fileIsOpened;

public:
    /*! \brief Constructor. This must be initialized with \a initialize(). */
    Exporter();

    /*! \brief Constructor. The file name is required.
     * The special file name "-" is interpreted as standard output.
     * If the \a append flag is set, new arrays
     * will be appended to the file (if the file format supports it) instead of overwriting the
     * old file contents. The optional \a hints may include parameters for the file format,
     * e.g. about compression level.
     *
     * Note that this initialization does not try to open the file yet, it merely
     * sets up the necessary information (and thus cannot fail). The functions that
     * access the data will report any errors that might occur. If you want to
     * perform some lightweight checks without accessing the file contents, use the
     * checkAccess() function.
     */
    Exporter(const std::string& fileName, bool append = Overwrite, const TagList& hints = TagList());

    /*!\brief Destructor. */
    ~Exporter();

    /*! \brief Initialize. See the constructor documentation. */
    void initialize(const std::string& fileName, bool append = Overwrite, const TagList& hints = TagList());

    /*! \brief Returns the file name. */
    std::string fileName() const
    {
        return _fileName;
    }

    /*! \brief Checks if the file is accessible and the format is supported. This function
     * does not access the file contents yet, and does not result in an open file descriptor. */
    Error checkAccess() const;

    /*! \brief Writes the \a array to the file. */
    Error writeArray(const ArrayContainer& array);
};

/*! \brief Shortcut to read a single array from a file in a single line of code. */
inline ArrayContainer load(const std::string& fileName, const TagList& hints = TagList(), Error* error = nullptr)
{
    return Importer(fileName, hints).readArray(error, -1);
}

/*! \brief Shortcut to read a single typed array from a file in a single line of code. The component type
 * is converted to the target type if necessary. */
template<typename T> Array<T> load(const std::string& fileName, const TagList& hints = TagList(), Error* error = nullptr)
{
    return load(fileName, hints, error).convert(typeFromTemplate<T>());
}

/*! \brief Shortcut to write a single array to a file in a single line of code. */
inline bool save(const ArrayContainer& A, const std::string& fileName, bool append = Overwrite, Error* error = nullptr, const TagList& hints = TagList())
{
    Error e = Exporter(fileName, append, hints).writeArray(A);
    if (error)
        *error = e;
    return (e == ErrorNone);
}

}

#endif
