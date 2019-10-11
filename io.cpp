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

#include <cstdio>
#include <cctype>
#include <string>

#include "io.hpp"
#include "dl.hpp"
#include "io-tad.hpp"
#include "io-pnm.hpp"
#include "io-raw.hpp"
#include "io-rgbe.hpp"
#include "io-exr.hpp"
#include "io-gta.hpp"
#include "io-hdf5.hpp"
#include "io-jpeg.hpp"
#include "io-pfs.hpp"
#include "io-png.hpp"
#include "io-tiff.hpp"
#include "io-ffmpeg.hpp"
#include "io-dcmtk.hpp"
#include "io-gdal.hpp"
#include "io-fits.hpp"


namespace TAD {

const char* strerror(Error e)
{
    const char* str = "";
    switch (e) {
    case ErrorNone:
        str = "success";
        break;
    case ErrorFormatUnsupported:
        str = "unsupported file format";
        break;
    case ErrorFeaturesUnsupported:
        str = "unsupported file features";
        break;
    case ErrorMissingHints:
        str = "required hints are missing";
        break;
    case ErrorInvalidData:
        str = "invalid data";
        break;
    case ErrorSeekingNotSupported:
        str = "seeking not supported";
        break;
    case ErrorAppendingNotSupported:
        str = "appending not supported";
        break;
    case ErrorLibrary:
        str = "a library function failed";
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
    std::string fieName(format);
    // set fieName for importers that handle multiple file formats
    if (format == "pbm" || format == "pgm" || format == "ppm" || format == "pnm" || format == "pam" || format == "pfm") {
        fieName = "pnm";
    } else if (format == "hdr" || format == "pic") {
        fieName = "rgbe";
    } else if (format == "dcm" || format == "dicom") {
        fieName = "dcmtk";
    } else if (format == "fit") {
        fieName = "fits";
    } else if (format == "mp4" || format == "m4v"
            || format == "mkv" || format == "ogv"
            || format == "mpeg" || format == "mpg"
            || format == "mov" || format == "avi" || format == "wmv"
            || format == "gif" || format == "dds" || format == "bmp" || format == "tga") {
        // TODO: this list can be much longer; add other extensions when needed
        fieName = "ffmpeg";
    } else if (format == "vrt" || format == "tsx") {
        // TODO: this list can be much longer; add other extensions when needed
        fieName = "gdal";
    } else if (format == "h5" || format == "he5" || format == "hdf5") {
        fieName = "hdf5";
    } else if (format == "jpg") {
        fieName = "jpeg";
    } else if (format == "tif") {
        fieName = "tiff";
    }

    // first builtin formats...
    if (fieName == "tad") {
        return new FormatImportExportTAD;
    } else if (fieName == "pnm") {
        return new FormatImportExportPNM;
    } else if (fieName == "raw") {
        return new FormatImportExportRAW;
    } else if (fieName == "rgbe") {
        return new FormatImportExportRGBE;
#ifdef TAD_STATIC
#  ifdef TAD_WITH_DCMTK
    } else if (fieName == "dcmtk") {
        return new FormatImportExportDCMTK;
#  endif
#  ifdef TAD_WITH_OPENEXR
    } else if (fieName == "exr") {
        return new FormatImportExportEXR;
#  endif
#  ifdef TAD_WITH_CFITSIO
    } else if (fieName == "fits") {
        return new FormatImportExportFITS;
#  endif
#  ifdef TAD_WITH_FFMPEG
    } else if (fieName == "ffmpeg") {
        return new FormatImportExportFFMPEG;
#  endif
#  ifdef TAD_WITH_GDAL
    } else if (fieName == "gdal") {
        return new FormatImportExportGDAL;
#  endif
#  ifdef TAD_WITH_GTA
    } else if (fieName == "gta") {
        return new FormatImportExportGTA;
#  endif
#  ifdef TAD_WITH_HDF5
    } else if (fieName == "hdf5") {
        return new FormatImportExportHDF5;
#  endif
#  ifdef TAD_WITH_JPEG
    } else if (fieName == "jpeg") {
        return new FormatImportExportJPEG;
#  endif
#  ifdef TAD_WITH_MATIO
    } else if (fieName == "mat") {
        return new FormatImportExportMAT;
#  endif
#  ifdef TAD_WITH_PFS
    } else if (fieName == "pfs") {
        return new FormatImportExportPFS;
#  endif
#  ifdef TAD_WITH_PNG
    } else if (fieName == "png") {
        return new FormatImportExportPNG;
#  endif
#  ifdef TAD_WITH_TIFF
    } else if (fieName == "tiff") {
        return new FormatImportExportTIFF;
#  endif
#endif
    } else {
#ifndef TAD_STATIC
        // ... then plugin formats
        std::string pluginName = std::string("libtadio-") + fieName + DOT_SO_STR;
        void* plugin = dlopen(pluginName.c_str(), RTLD_NOW);
        if (!plugin) {
            return nullptr;
        }
        std::string factoryName = std::string("FormatImportExportFactory_") + fieName;
        void* factory = dlsym(plugin, factoryName.c_str());
        if (!factory) {
            return nullptr;
        }
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

Importer::~Importer()
{
    if (_fie && _fileIsOpened)
        _fie->close();
}

Importer::Importer(const std::string& fileName, const TagList& hints)
{
    initialize(fileName, hints);
}

void Importer::initialize(const std::string& fileName, const TagList& hints)
{
    _fileName = fileName;
    _hints = hints;
    _format = (hints.contains("FORMAT") ? hints.value("FORMAT")
            : fileName == "-" ? "tad"
            : getExtension(_fileName));
    _fie = std::shared_ptr<FormatImportExport>(openFormatImportExport(_format));
    _fileIsOpened = false;
}

Error Importer::checkAccess() const
{
    if (!_fie) {
        return ErrorFormatUnsupported;
    }
    if (fileName() != "-") {
        FILE* f = std::fopen(fileName().c_str(), "rb");
        if (!f) {
            return ErrorSysErrno;
        }
        std::fclose(f);
    }
    return ErrorNone;
}

Error Importer::ensureFileIsOpenedForReading()
{
    if (!_fie) {
        return ErrorFormatUnsupported;
    }
    Error e = ErrorNone;
    if (!_fileIsOpened) {
        e = _fie->openForReading(_fileName, _hints);
        if (e == ErrorNone)
            _fileIsOpened = true;
    }
    return e;
}

int Importer::arrayCount()
{
    Error e = ensureFileIsOpenedForReading();
    return (e == ErrorNone ? _fie->arrayCount() : -1);
}

ArrayContainer Importer::readArray(Error* error, int arrayIndex)
{
    Error e = ensureFileIsOpenedForReading();
    if (e != ErrorNone) {
        if (error)
            *error = e;
        return ArrayContainer();
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
    Error e = ensureFileIsOpenedForReading();
    if (e != ErrorNone) {
        if (error)
            *error = e;
        return false;
    }
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

Exporter::~Exporter()
{
    if (_fie && _fileIsOpened)
        _fie->close();
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
    _format = (hints.contains("FORMAT") ? hints.value("FORMAT")
            : fileName == "-" ? "tad"
            : getExtension(_fileName));
    _fie = std::shared_ptr<FormatImportExport>(openFormatImportExport(_format));
    _fileIsOpened = false;
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
