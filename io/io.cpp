/*
 * Copyright (C) 2018, 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
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
#include "io-csv.hpp"
#include "io-pnm.hpp"
#include "io-raw.hpp"
#include "io-rgbe.hpp"
#include "io-stb.hpp"
#include "io-dcmtk.hpp"
#include "io-exr.hpp"
#include "io-ffmpeg.hpp"
#include "io-fits.hpp"
#include "io-gdal.hpp"
#include "io-gta.hpp"
#include "io-hdf5.hpp"
#include "io-jpeg.hpp"
#include "io-mat.hpp"
#include "io-pdf.hpp"
#include "io-pfs.hpp"
#include "io-png.hpp"
#include "io-tiff.hpp"
#include "io-magick.hpp"


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
    /* Create vector of FormatImportExport names in order of preference for the given format */
    std::vector<std::string> fieNames;
    if (format == "pbm" || format == "pgm" || format == "ppm" || format == "pnm" || format == "pam" || format == "pfm") {
        fieNames.push_back("pnm");
    } else if (format == "hdr" || format == "pic") {
        fieNames.push_back("rgbe");
    } else if (format == "dcm" || format == "dicom") {
        fieNames.push_back("dcmtk");
    } else if (format == "fit") {
        fieNames.push_back("fits");
    } else if (format == "mp4" || format == "m4v" || format == "mkv" || format == "ogv"
            || format == "mpeg" || format == "mpg" || format == "webm"
            || format == "mov" || format == "avi" || format == "wmv") {
        // TODO: this list can be much longer; add other extensions when needed
        fieNames.push_back("ffmpeg");
    } else if (format == "vrt" || format == "tsx") {
        // TODO: this list can be much longer; add other extensions when needed
        fieNames.push_back("gdal");
    } else if (format == "h5" || format == "he5" || format == "hdf5") {
        fieNames.push_back("hdf5");
    } else if (format == "jpg" || format == "jpeg") {
        fieNames.push_back("jpeg");
        fieNames.push_back("stb");
    } else if (format == "png") {
        fieNames.push_back("png");
        fieNames.push_back("stb");
    } else if (format == "tif" || format == "tiff") {
        fieNames.push_back("tiff");
        fieNames.push_back("magick");
    } else if (format == "bmp" || format == "tga" || format == "psd") {
        fieNames.push_back("stb");
    } else if (format == "gif" || format == "dds" || format == "xpm"
            || format == "xwd" || format == "ico" || format == "webp") {
        // TODO: this list can be much longer; add other extensions when needed
        fieNames.push_back("magick");
    } else {
        // fallback: assume there is an importer/exporter with the extension name
        fieNames.push_back(format);
    }

    FormatImportExport* fie = nullptr;
    for (size_t i = 0; !fie && i < fieNames.size(); i++) {
        const std::string& fieName = fieNames[i];
        // first builtin formats...
        if (fieName == "tad") {
            fie = new FormatImportExportTAD;
        } else if (fieName == "csv") {
            fie = new FormatImportExportCSV;
        } else if (fieName == "pnm") {
            fie = new FormatImportExportPNM;
        } else if (fieName == "raw") {
            fie = new FormatImportExportRAW;
        } else if (fieName == "rgbe") {
            fie = new FormatImportExportRGBE;
        } else if (fieName == "stb") {
            fie = new FormatImportExportSTB;
#ifdef TAD_STATIC
#  ifdef TAD_WITH_DCMTK
        } else if (fieName == "dcmtk") {
            fie = new FormatImportExportDCMTK;
#  endif
#  ifdef TAD_WITH_OPENEXR
        } else if (fieName == "exr") {
            fie = new FormatImportExportEXR;
#  endif
#  ifdef TAD_WITH_CFITSIO
        } else if (fieName == "fits") {
            fie = new FormatImportExportFITS;
#  endif
#  ifdef TAD_WITH_FFMPEG
        } else if (fieName == "ffmpeg") {
            fie = new FormatImportExportFFMPEG;
#  endif
#  ifdef TAD_WITH_GDAL
        } else if (fieName == "gdal") {
            fie = new FormatImportExportGDAL;
#  endif
#  ifdef TAD_WITH_GTA
        } else if (fieName == "gta") {
            fie = new FormatImportExportGTA;
#  endif
#  ifdef TAD_WITH_HDF5
        } else if (fieName == "hdf5") {
            fie = new FormatImportExportHDF5;
#  endif
#  ifdef TAD_WITH_JPEG
        } else if (fieName == "jpeg") {
            fie = new FormatImportExportJPEG;
#  endif
#  ifdef TAD_WITH_MATIO
        } else if (fieName == "mat") {
            fie = new FormatImportExportMAT;
#  endif
#  ifdef TAD_WITH_POPPLER
        } else if (fieName == "pdf") {
            fie = new FormatImportExportPDF;
#  endif
#  ifdef TAD_WITH_PFS
        } else if (fieName == "pfs") {
            fie = new FormatImportExportPFS;
#  endif
#  ifdef TAD_WITH_PNG
        } else if (fieName == "png") {
            fie = new FormatImportExportPNG;
#  endif
#  ifdef TAD_WITH_TIFF
        } else if (fieName == "tiff") {
            fie = new FormatImportExportTIFF;
#  endif
#  ifdef TAD_WITH_MAGICK
        } else if (fieName == "magick") {
            fie = new FormatImportExportMagick;
#  endif
#endif
        } else {
#ifndef TAD_STATIC
            // ... then plugin formats
            std::string pluginName = std::string("libtadio-") + fieName + DOT_SO_STR;
            void* plugin = dlopen(pluginName.c_str(), RTLD_NOW);
            if (plugin) {
                std::string factoryName = std::string("FormatImportExportFactory_") + fieName;
                void* factory = dlsym(plugin, factoryName.c_str());
                if (factory) {
                    FormatImportExport* (*fieFactory)() = reinterpret_cast<FormatImportExport* (*)()>(factory);
                    fie = fieFactory();
                }
            }
#endif
        }
    }
    return fie;
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
