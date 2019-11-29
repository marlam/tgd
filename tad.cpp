/*
 * Copyright (C) 2019 Computer Graphics Group, University of Siegen
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

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>
#ifdef _WIN32
# include <fcntl.h>
#endif

#include "array.hpp"
#include "io.hpp"
#include "foreach.hpp"
#include "operators.hpp"
#include "tools.hpp"

#include "cmdline.hpp"


bool parseType(const std::string& value)
{
    TAD::Type t;
    return TAD::typeFromString(value, &t);
}

TAD::Type getType(const std::string& value)
{
    TAD::Type t;
    TAD::typeFromString(value, &t);
    return t;
}

void removeValueRelatedTags(TAD::ArrayContainer& array)
{
    for (size_t i = 0; i < array.componentCount(); i++) {
        array.componentTagList(i).unset("MINVAL");
        array.componentTagList(i).unset("MAXVAL");
    }
}


int tad_help(void)
{
    fprintf(stderr,
            "Usage: tad <command> [options...] [arguments...]\n"
            "Available commands:\n"
            "  convert\n"
            "  diff\n"
            "  info\n"
            "Use the --help option to get command-specific help.\n");
    return 0;
}

int tad_version(void)
{
    fprintf(stderr, "tad version %s\n", TAD_VERSION);
    return 0;
}

template<typename T>
void tad_convert_normalize_helper_to_float(TAD::Array<T>& array, TAD::Type oldType)
{
    if (oldType == TAD::int8) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return (v < T(0)
                    ? v / -std::numeric_limits<int8_t>::min()
                    : v / std::numeric_limits<int8_t>::max()); });
        removeValueRelatedTags(array);
    } else if (oldType == TAD::uint8) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return v / std::numeric_limits<uint8_t>::max(); });
        removeValueRelatedTags(array);
    } else if (oldType == TAD::int16) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return (v < T(0)
                    ? v / -std::numeric_limits<int16_t>::min()
                    : v / std::numeric_limits<int16_t>::max()); });
        removeValueRelatedTags(array);
    } else if (oldType == TAD::uint16) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return v / std::numeric_limits<uint16_t>::max(); });
        removeValueRelatedTags(array);
    }
}

template<typename T>
void tad_convert_normalize_helper_from_float(TAD::Array<T>& array, TAD::Type newType)
{
    if (newType == TAD::int8) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return (v < T(0)
                    ? v * -std::numeric_limits<int8_t>::min()
                    : v * std::numeric_limits<int8_t>::max()); });
        removeValueRelatedTags(array);
    } else if (newType == TAD::uint8) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return v * std::numeric_limits<uint8_t>::max(); });
        removeValueRelatedTags(array);
    } else if (newType == TAD::int16) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return (v < T(0)
                    ? v * -std::numeric_limits<int16_t>::min()
                    : v * std::numeric_limits<int16_t>::max()); });
        removeValueRelatedTags(array);
    } else if (newType == TAD::uint16) {
        TAD::forEachComponentInplace(array,
                [] (T v) -> T { return v * std::numeric_limits<uint16_t>::max(); });
        removeValueRelatedTags(array);
    }
}

int tad_convert(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithArg("type", 't', parseType);
    cmdLine.addOptionWithoutArg("normalize", 'n');
    cmdLine.addOptionWithArg("input-format", 'i');
    cmdLine.addOptionWithArg("output-format", 'o');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 2, 2, errMsg)) {
        fprintf(stderr, "tad convert: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad convert [option]... <infile|-> <outfile|->\n"
                "Convert to a new type and/or format.\n"
                "Options:\n"
                "  -n|--normalize: create/assume floating point values in [-1,1]/[0,1]\n"
                "    when converting to/from signed/unsigned integer values\n"
                "  -t|--type: convert to new type (int8, uint8, int16, uint16, int32, uint32,\n"
                "    int64, uint64, float32, float64)\n"
                "  -i|--input-format: set format of input file (overriding file name extension)\n"
                "  -o|--output-format: set format of output file (overriding file name extension)\n");
        return 0;
    }

    const std::string& inFileName = cmdLine.arguments()[0];
    const std::string& outFileName = cmdLine.arguments()[1];
    TAD::TagList importerHints;
    if (cmdLine.isSet("input-format"))
        importerHints.set("FORMAT", cmdLine.value("input-format"));
    TAD::TagList exporterHints;
    if (cmdLine.isSet("output-format"))
        exporterHints.set("FORMAT", cmdLine.value("output-format"));
    TAD::Importer importer(inFileName, importerHints);
    TAD::Exporter exporter(outFileName, TAD::Overwrite, exporterHints);
    TAD::Error err = TAD::ErrorNone;
    TAD::Type type = getType(cmdLine.value("type"));
    for (;;) {
        if (!importer.hasMore(&err)) {
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad convert: %s: %s\n", inFileName.c_str(), TAD::strerror(err));
            }
            break;
        }
        TAD::ArrayContainer array = importer.readArray(&err);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad convert: %s: %s\n", inFileName.c_str(), TAD::strerror(err));
            break;
        }
        if (cmdLine.isSet("type")) {
            TAD::Type oldType = array.componentType();
            if (cmdLine.isSet("normalize")) {
                if (type == TAD::float32) {
                    array = convert(array, type);
                    TAD::Array<float> floatArray(array);
                    tad_convert_normalize_helper_to_float<float>(floatArray, oldType);
                } else if (type == TAD::float64) {
                    array = convert(array, type);
                    TAD::Array<double> doubleArray(array);
                    tad_convert_normalize_helper_to_float<double>(doubleArray, oldType);
                } else if (oldType == TAD::float32) {
                    if (type == TAD::int8 || type == TAD::uint8 || type == TAD::int16 || type == TAD::uint16) {
                        TAD::Array<float> floatArray(array);
                        tad_convert_normalize_helper_from_float<float>(floatArray, type);
                    }
                    array = convert(array, type);
                } else if (oldType == TAD::float64) {
                    if (type == TAD::int8 || type == TAD::uint8 || type == TAD::int16 || type == TAD::uint16) {
                        TAD::Array<double> doubleArray(array);
                        tad_convert_normalize_helper_from_float<double>(doubleArray, type);
                    }
                    array = convert(array, type);
                }
            } else {
                array = convert(array, type);
            }
        }
        err = exporter.writeArray(array);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad convert: %s: %s\n", outFileName.c_str(), TAD::strerror(err));
            break;
        }
    }

    return (err == TAD::ErrorNone ? 0 : 1);
}

int tad_diff(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithArg("input-format", 'i');
    cmdLine.addOptionWithArg("output-format", 'o');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 3, 3, errMsg)) {
        fprintf(stderr, "tad diff: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad diff [option]... <infile0|-> <infile1|-> <outfile|->\n"
                "Compute the absolute difference.\n"
                "Options:\n"
                "  -i|--input-format: set format of input file (overriding file name extension)\n"
                "  -o|--output-format: set format of output file (overriding file name extension)\n");
        return 0;
    }

    const std::string& inFileName0 = cmdLine.arguments()[0];
    const std::string& inFileName1 = cmdLine.arguments()[1];
    const std::string& outFileName = cmdLine.arguments()[2];
    TAD::TagList importerHints;
    if (cmdLine.isSet("input-format"))
        importerHints.set("FORMAT", cmdLine.value("input-format"));
    TAD::TagList exporterHints;
    if (cmdLine.isSet("output-format"))
        exporterHints.set("FORMAT", cmdLine.value("output-format"));
    TAD::Importer importer0(inFileName0, importerHints);
    TAD::Importer importer1(inFileName1, importerHints);
    TAD::Exporter exporter(outFileName, TAD::Overwrite, exporterHints);
    TAD::Error err = TAD::ErrorNone;
    for (;;) {
        if (!importer0.hasMore(&err)) {
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad diff: %s: %s\n", inFileName0.c_str(), TAD::strerror(err));
            }
            break;
        }
        if (!importer1.hasMore(&err)) {
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad diff: %s: %s\n", inFileName1.c_str(), TAD::strerror(err));
            }
            break;
        }
        TAD::ArrayContainer array0 = importer0.readArray(&err);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad diff: %s: %s\n", inFileName0.c_str(), TAD::strerror(err));
            break;
        }
        TAD::ArrayContainer array1 = importer1.readArray(&err);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad diff: %s: %s\n", inFileName1.c_str(), TAD::strerror(err));
            break;
        }
        if (!array0.isCompatible(array1)) {
            fprintf(stderr, "tad diff: incompatible input arrays\n");
            break;
        }
        TAD::Array<float> floatArray0 = convert(array0, TAD::float32);
        TAD::Array<float> floatArray1 = convert(array1, TAD::float32);
        TAD::Array<float> floatResult = TAD::forEachComponent(floatArray0, floatArray1,
                [] (float v0, float v1) -> float { return std::abs(v0 - v1); });
        TAD::ArrayContainer result = convert(floatResult, array0.componentType());
        removeValueRelatedTags(result);
        err = exporter.writeArray(result);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad diff: %s: %s\n", outFileName.c_str(), TAD::strerror(err));
            break;
        }
    }

    return (err == TAD::ErrorNone ? 0 : 1);
}

void tad_info_print_taglist(const TAD::TagList& tl)
{
    for (auto it = tl.cbegin(); it != tl.cend(); it++) {
        printf("    %s=%s\n", it->first.c_str(), it->second.c_str());
    }
}

std::string tad_info_human_readable_memsize(unsigned long long size)
{
    const double dsize = size;
    const unsigned long long u1024 = 1024;
    char s[32];

    if (size >= u1024 * u1024 * u1024 * u1024) {
        std::snprintf(s, sizeof(s), "%.2f TiB", dsize / (u1024 * u1024 * u1024 * u1024));
    } else if (size >= u1024 * u1024 * u1024) {
        std::snprintf(s, sizeof(s), "%.2f GiB", dsize / (u1024 * u1024 * u1024));
    } else if (size >= u1024 * u1024) {
        std::snprintf(s, sizeof(s), "%.2f MiB", dsize / (u1024 * u1024));
    } else if (size >= u1024) {
        std::snprintf(s, sizeof(s), "%.2f KiB", dsize / u1024);
    } else if (size > 1 || size == 0) {
        std::snprintf(s, sizeof(s), "%d bytes", int(size));
    } else {
        std::strcpy(s, "1 byte");
    }
    return s;
}

int tad_info(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithoutArg("statistics", 's');
    cmdLine.addOptionWithArg("input-format", 'i');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 1, 1, errMsg)) {
        fprintf(stderr, "tad info: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad info [option]... <infile|->\n"
                "Print information.\n"
                "Options:\n"
                "  -s|--statistics: print statistics\n"
                "  -i|--input-format: set format of input file (overriding file name extension)\n"
                "  -o|--output-format: set format of output file (overriding file name extension)\n");
        return 0;
    }

    const std::string& inFileName = cmdLine.arguments()[0];
    TAD::TagList importerHints;
    if (cmdLine.isSet("input-format"))
        importerHints.set("FORMAT", cmdLine.value("input-format"));
    TAD::Importer importer(inFileName, importerHints);
    TAD::Error err = TAD::ErrorNone;
    int arrayCounter = 0;
    for (;;) {
        if (!importer.hasMore(&err)) {
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad info: %s: %s\n", inFileName.c_str(), TAD::strerror(err));
            }
            break;
        }
        TAD::ArrayContainer array = importer.readArray(&err);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad info: %s: %s\n", inFileName.c_str(), TAD::strerror(err));
            break;
        }
        std::string sizeString;
        if (array.dimensions().size() == 0) {
            sizeString = "0";
        } else {
            sizeString = std::to_string(array.dimension(0));
            for (size_t i = 1; i < array.dimensions().size(); i++) {
                sizeString += 'x';
                sizeString += std::to_string(array.dimension(i));
            }
        }
        printf("array %d: %zd x %s, size %s (%s)\n",
                arrayCounter, array.componentCount(),
                TAD::typeToString(array.componentType()),
                sizeString.c_str(), tad_info_human_readable_memsize(array.dataSize()).c_str());
        if (array.globalTagList().size() > 0) {
            printf("  global:\n");
            tad_info_print_taglist(array.globalTagList());
        }
        for (size_t i = 0; i < array.dimensionCount(); i++) {
            if (array.dimensionTagList(i).size() > 0) {
                printf("  dimension %zd:\n", i);
                tad_info_print_taglist(array.dimensionTagList(i));
            }
        }
        for (size_t i = 0; i < array.componentCount(); i++) {
            if (array.componentTagList(i).size() > 0) {
                printf("  component %zd:\n", i);
                tad_info_print_taglist(array.componentTagList(i));
            }
        }
        if (cmdLine.isSet("statistics")) {
            TAD::Array<float> floatArray = convert(array, TAD::float32);
            for (size_t i = 0; i < array.componentCount(); i++) {
                long long int finiteValues = 0;
                float minVal = std::numeric_limits<float>::quiet_NaN();
                float maxVal = std::numeric_limits<float>::quiet_NaN();
                float sampleMean = std::numeric_limits<float>::quiet_NaN();
                float sampleVariance = std::numeric_limits<float>::quiet_NaN();
                float sampleDeviation = std::numeric_limits<float>::quiet_NaN();
                float tmpMinVal = +std::numeric_limits<float>::max();
                float tmpMaxVal = -std::numeric_limits<float>::max();
                double sum = 0.0;
                double sumOfSquares = 0.0;
                for (size_t e = 0; e < array.elementCount(); e++) {
                    float val = floatArray.get<float>(e, i);
                    if (std::isfinite(val)) {
                        finiteValues++;
                        if (val < tmpMinVal)
                            tmpMinVal = val;
                        if (val > tmpMaxVal)
                            tmpMaxVal = val;
                        sum += val;
                        sumOfSquares += val * val;
                    }
                }
                if (finiteValues > 0) {
                    minVal = tmpMinVal;
                    maxVal = tmpMaxVal;
                    sampleMean = sum / finiteValues;
                    if (finiteValues > 1) {
                        sampleVariance = (sumOfSquares - sum / finiteValues * sum) / (finiteValues - 1);
                        if (sampleVariance < 0.0f)
                            sampleVariance = 0.0f;
                        sampleDeviation = std::sqrt(sampleVariance);
                    } else if (finiteValues == 1) {
                        sampleVariance = 0.0f;
                        sampleDeviation = 0.0f;
                    }
                }
                printf("  component %zd: min=%g max=%g mean=%g var=%g dev=%g\n", i,
                        minVal, maxVal, sampleMean, sampleVariance, sampleDeviation);
            }
        }
        arrayCounter++;
    }

    return (err == TAD::ErrorNone ? 0 : 1);
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _fmode = _O_BINARY;
    setbuf(stderr, NULL);
#endif

    int retval = 0;
    if (argc < 2) {
        tad_help();
        retval = 1;
    } else if (std::strcmp(argv[1], "help") == 0 || std::strcmp(argv[1], "--help") == 0) {
        retval = tad_help();
    } else if (std::strcmp(argv[1], "version") == 0 || std::strcmp(argv[1], "--version") == 0) {
        retval = tad_version();
    } else if (std::strcmp(argv[1], "convert") == 0) {
        retval = tad_convert(argc - 1, &(argv[1]));
    } else if (std::strcmp(argv[1], "diff") == 0) {
        retval = tad_diff(argc - 1, &(argv[1]));
    } else if (std::strcmp(argv[1], "info") == 0) {
        retval = tad_info(argc - 1, &(argv[1]));
    } else {
        fprintf(stderr, "tad: invalid command %s\n", argv[1]);
        retval = 1;
    }
    return retval;
}
