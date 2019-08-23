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

#include <string>
#include <cctype>

#include "io.hpp"
#include "dl.hpp"
#include "io-raw.hpp"
#include "io-tad.hpp"
#include "io-gta.hpp"
#include "io-pnm.hpp"
#include "io-exr.hpp"
#include "io-pfs.hpp"
#include "io-png.hpp"
#include "io-jpeg.hpp"


namespace TAD {

const char* strerror(Error e)
{
    const char* str = "";
    switch (e) {
    case ErrorNone:
        str = "Success";
        break;
    case ErrorFormatUnsupported:
        str = "Unsupported file format";
        break;
    case ErrorFeaturesUnsupported:
        str = "Unsupported file features";
        break;
    case ErrorMissingHints:
        str = "Required hints are missing";
        break;
    case ErrorInvalidData:
        str = "Invalid data";
        break;
    case ErrorSeekingNotSupported:
        str = "Seeking not supported";
        break;
    case ErrorAppendingNotSupported:
        str = "Appending not supported";
        break;
    case ErrorLibrary:
        str = "A library function failed";
        break;
    case ErrorSysErrno:
        str = std::strerror(errno);
        break;
    }
    return str;
}

static std::string getExtension(const std::string& fileName)
{
    std::string extension;
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot != std::string::npos) {
        extension = fileName.substr(lastDot + 1);
        for (size_t i = 0; i < extension.size(); i++)
            extension[i] = std::tolower(extension[i]);
    }
    return extension;
}

static FormatImportExport* openFormatImportExport(const std::string& format)
{
    std::string fieName;
    if (format == "csv") {
        fieName = "csv";
    } else if (format == "exr") {
        fieName = "exr";
    } else if (format == "gta") {
        fieName = "gta";
    } else if (format == "h5" || format == "he5" || format == "hdf5") {
        fieName = "hdf5";
    } else if (format == "jpg" || format == "jpeg") {
        fieName = "jpeg";
    } else if (format == "mat") {
        fieName = "mat";
    } else if (format == "pbm"
            || format == "pgm"
            || format == "ppm"
            || format == "pnm"
            || format == "pam") {
        fieName = "pnm";
    } else if (format == "pfs") {
        fieName = "pfs";
    } else if (format == "png") {
        fieName = "png";
    } else if (format == "raw") {
        fieName = "raw";
    } else if (format == "tad") {
        fieName = "tad";
    }

    // first builtin formats...
    if (fieName == "csv") {
        return nullptr;// TODO: new FormatImportExportCSV;
    } else if (fieName == "raw") {
        return new FormatImportExportRAW;
    } else if (fieName == "tad") {
        return new FormatImportExportTAD;
#ifdef TAD_STATIC
#  ifdef TAD_WITH_GTA
    } else if (fieName == "gta") {
        return new FormatImportExportGTA;
#  endif
#  ifdef TAD_WITH_NETPBM
    } else if (fieName == "pnm") {
        return new FormatImportExportPNM;
#  endif
#  ifdef TAD_WITH_OPENEXR
    } else if (fieName == "exr") {
        return new FormatImportExportEXR;
#  endif
#  ifdef TAD_WITH_PFS
    } else if (fieName == "pfs") {
        return new FormatImportExportPFS;
#  endif
#  ifdef TAD_WITH_PNG
    } else if (fieName == "png") {
        return new FormatImportExportPNG;
#  endif
#  ifdef TAD_WITH_JPEG
    } else if (fieName == "jpeg") {
        return new FormatImportExportJPEG;
#  endif
#endif
    } else {
#ifndef TAD_STATIC
        // ... then plugin formats
        std::string pluginName = std::string("libtadio-") + fieName + DOT_SO_STR;
        void* plugin = dlopen(pluginName.c_str(), RTLD_NOW);
        if (!plugin)
            return nullptr;
        std::string factoryName = std::string("FormatImportExportFactory_") + fieName;
        void* factory = dlsym(plugin, factoryName.c_str());
        if (!factory)
            return nullptr;
        FormatImportExport* (*fieFactory)() = reinterpret_cast<FormatImportExport* (*)()>(factory);
        return fieFactory();
#else
        return nullptr;
#endif
    }
}

Importer::Importer()
{
}

Importer::Importer(const std::string& fileName, const TagList& hints)
{
    initialize(fileName, hints);
}

void Importer::initialize(const std::string& fileName, const TagList& hints)
{
    _fileName = fileName;
    _hints = hints;
    _format = (hints.contains("FORMAT") ? hints.value("FORMAT") : getExtension(_fileName));
    _fie = std::shared_ptr<FormatImportExport>(openFormatImportExport(_format));
    _fileIsOpened = false;
}

Importer::~Importer()
{
}

int Importer::arrayCount()
{
    if (!_fie)
        return -1;
    if (!_fileIsOpened && _fie->openForReading(_fileName, _hints) != ErrorNone)
        return -1;
    _fileIsOpened = true;
    return _fie->arrayCount();
}

ArrayContainer Importer::readArray(Error* error, int arrayIndex)
{
    if (!_fie) {
        if (error)
            *error = ErrorFormatUnsupported;
        return ArrayContainer();
    }
    Error e = ErrorNone;
    if (!_fileIsOpened) {
        e = _fie->openForReading(_fileName, _hints);
        if (e != ErrorNone) {
            if (error)
                *error = e;
            return ArrayContainer();
        }
        _fileIsOpened = true;
    }
    ArrayContainer r = _fie->readArray(&e, arrayIndex);
    if (e != ErrorNone) {
        if (error)
            *error = e;
        return ArrayContainer();
    }
    if (error)
        *error = ErrorNone;
    return r;
}

bool Importer::hasMore(Error* error)
{
    if (!_fie) {
        if (error)
            *error = ErrorFormatUnsupported;
        return false;
    }
    Error e;
    if (!_fileIsOpened && (e = _fie->openForReading(_fileName, _hints)) != ErrorNone) {
        if (error)
            *error = e;
        return false;
    }
    _fileIsOpened = true;
    bool ret = _fie->hasMore();
    if (!ret) {
        if (error)
            *error = ErrorNone;
    }
    return ret;
}

Exporter::Exporter()
{
}

Exporter::Exporter(const std::string& fileName, bool append, const TagList& hints)
{
    initialize(fileName, append, hints);
}

void Exporter::initialize(const std::string& fileName, bool append, const TagList& hints)
{
    _fileName = fileName;
    _append = append;
    _hints = hints;
    _format = (hints.contains("FORMAT") ? hints.value("FORMAT") : getExtension(_fileName));
    _fie = std::shared_ptr<FormatImportExport>(openFormatImportExport(_format));
    _fileIsOpened = false;
}

Exporter::~Exporter()
{
}

Error Exporter::writeArray(const ArrayContainer& array)
{
    if (!_fie) {
        return ErrorFormatUnsupported;
    }
    Error e;
    if (!_fileIsOpened) {
        e = _fie->openForWriting(_fileName, _append, _hints);
        if (e != ErrorNone) {
            return e;
        }
        _fileIsOpened = true;
    }
    e = _fie->writeArray(array);
    if (e != ErrorNone) {
        return e;
    }

    return ErrorNone;
}

}
