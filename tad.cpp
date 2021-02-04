/*
 * Copyright (C) 2019, 2020 Computer Graphics Group, University of Siegen
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

#include <string>
#include <vector>

#include "array.hpp"
#include "io.hpp"
#include "foreach.hpp"
#include "operators.hpp"
#include "tools.hpp"

#include "cmdline.hpp"


/* Helper functions to parse command line options */

bool parseUInt(const std::string& value, bool allowZero)
{
    bool ok = true;
    size_t val = 0;
    size_t idx = 0;
    try {
        val = std::stoull(value, &idx);
    }
    catch (...) {
        ok = false;
    }
    if (ok && (idx != value.length() || (!allowZero && val == 0))) {
        ok = false;
    }
    return ok;
}

bool parseUInt(const std::string& value)
{
    return parseUInt(value, true);
}

bool parseUIntLargerThanZero(const std::string& value)
{
    return parseUInt(value, false);
}

size_t getUInt(const std::string& value)
{
    return std::stoull(value);
}

bool parseUIntList(const std::string& value, bool allowZero)
{
    for (size_t i = 0; i < value.length();) {
        size_t j = value.find_first_of(',', i);
        if (!parseUInt(value.substr(i, (j == std::string::npos ? std::string::npos : j - i)), allowZero))
            return false;
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
    return true;
}

bool parseUIntList(const std::string& value)
{
    return parseUIntList(value, true);
}

bool parseUIntLargerThanZeroList(const std::string& value)
{
    return parseUIntList(value, false);
}

std::vector<size_t> getUIntList(const std::string& value)
{
    std::vector<size_t> values;
    for (size_t i = 0; i < value.length();) {
        size_t j = value.find_first_of(',', i);
        values.push_back(getUInt(value.substr(i, (j == std::string::npos ? std::string::npos : j - i))));
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
    return values;
}

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

bool parseUIntAndName(const std::string& value)
{
    size_t i = value.find_first_of(',');
    return (i != std::string::npos
            && i != 0
            && i != value.length() - 1
            && parseUInt(value.substr(0, i)));
}

void getUIntAndName(const std::string& value, size_t* u, std::string* n)
{
    size_t i = value.find_first_of(',');
    *u = getUInt(value.substr(0, i));
    *n = value.substr(i + 1);
}

/* Helper functions for handling tags */

void addTag(TAD::TagList& tl, const std::string& tag)
{
    size_t i = tag.find('=', 1);
    if (i == std::string::npos) {
        tl.set(tag, "");
    } else {
        tl.set(tag.substr(0, i), tag.substr(i + 1));
    }
}

TAD::TagList createTagList(const std::vector<std::string>& tags)
{
    TAD::TagList tl;
    for (size_t i = 0; i < tags.size(); i++)
        addTag(tl, tags[i]);
    return tl;
}

void removeValueRelatedTags(TAD::ArrayContainer& array)
{
    for (size_t i = 0; i < array.componentCount(); i++) {
        array.componentTagList(i).unset("MINVAL");
        array.componentTagList(i).unset("MAXVAL");
    }
}

/* Helper function for handling boxes */

std::vector<size_t> getBoxFromArray(const TAD::ArrayContainer& array)
{
    std::vector<size_t> box(array.dimensionCount() * 2);
    for (size_t i = 0; i < array.dimensionCount(); i++) {
        box[i] = 0;
        box[i + array.dimensionCount()] = array.dimension(i);
    }
    return box;
}

std::vector<size_t> restrictBoxToArray(const std::vector<size_t>& box, const TAD::ArrayContainer& array)
{
    std::vector<size_t> newBox(array.dimensionCount() * 2, 0);
    bool isEmpty = false;
    for (size_t i = 0; i < array.dimensionCount(); i++) {
        if (box[i] >= array.dimension(i) || box[i + array.dimensionCount()] == 0) {
            isEmpty = true;
            break;
        }
    }
    if (!isEmpty) {
        for (size_t i = 0; i < array.dimensionCount(); i++) {
            newBox[i] = box[i];
            newBox[i + array.dimensionCount()] = std::min(box[i + array.dimensionCount()], array.dimension(i) - box[i]);
        }
    }
    return newBox;
}

bool boxIsEmpty(const std::vector<size_t>& box)
{
    for (size_t i = box.size() / 2; i < box.size(); i++) {
        if (box[i] == 0)
            return true;
    }
    return false;
}

void initBoxIndex(const std::vector<size_t>& box, std::vector<size_t>& index)
{
    for (size_t i = 0; i < index.size(); i++)
        index[i] = box[i];
}

bool incBoxIndex(const std::vector<size_t>& box, std::vector<size_t>& index)
{
    if (index.size() == 0 || box.size() != index.size() * 2 || boxIsEmpty(box))
        return false;
    size_t dimToInc = index.size() - 1;
    while (index[dimToInc] == box[dimToInc] + box[index.size() + dimToInc] - 1) {
        if (dimToInc == 0)
            return false;
        dimToInc--;
    }
    index[dimToInc]++;
    for (size_t j = dimToInc + 1; j < index.size(); j++) {
        index[j] = box[j];
    }
    return true;
}

/* tad commands */

int tad_help(void)
{
    fprintf(stderr,
            "Usage: tad <command> [options...] [arguments...]\n"
            "Available commands:\n"
            "  convert\n"
            "  create\n"
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
    cmdLine.addOptionWithoutArg("normalize", 'n');
    cmdLine.addOptionWithArg("type", 't', parseType);
    cmdLine.addOptionWithoutArg("append", 'a');
    cmdLine.addOptionWithArg("input", 'i');
    cmdLine.addOptionWithArg("output", 'o');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 2, 2, errMsg)) {
        fprintf(stderr, "tad convert: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad convert [option]... <infile|-> <outfile|->\n"
                "Convert to a new type and/or format.\n"
                "Options:\n"
                "  -i|--input=TAG       set input hints such as FORMAT=gdal, DPI=300 etc\n"
                "  -o|--output=TAG      set output hints such as FORMAT=gdal, COMPRESSION=9 etc\n"
                "  -t|--type=T          convert to new type (int8, uint8, int16, uint16, int32, uint32,\n"
                "                       int64, uint64, float32, float64)\n"
                "  -n|--normalize       create/assume floating point values in [-1,1]/[0,1]\n"
                "                       when converting to/from signed/unsigned integer values\n"
                "  -a|--append          append to the output file instead of overwriting (not always possible)\n");
        return 0;
    }

    const std::string& inFileName = cmdLine.arguments()[0];
    const std::string& outFileName = cmdLine.arguments()[1];
    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::TagList exporterHints = createTagList(cmdLine.valueList("output"));
    TAD::Importer importer(inFileName, importerHints);
    TAD::Exporter exporter(outFileName, cmdLine.isSet("append") ? TAD::Append : TAD::Overwrite, exporterHints);
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

int tad_create(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithArg("output", 'o');
    cmdLine.addOptionWithArg("dimensions", 'd', parseUIntLargerThanZeroList);
    cmdLine.addOptionWithArg("components", 'c', parseUIntLargerThanZero);
    cmdLine.addOptionWithArg("type", 't', parseType);
    cmdLine.addOptionWithArg("n", 'n', parseUIntLargerThanZero, "1");
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 1, 1, errMsg)) {
        fprintf(stderr, "tad create: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad create [option]... <outfile|->\n"
                "Create zero-filled arrays.\n"
                "Options:\n"
                "  -o|--output=TAG      set output hints such as FORMAT=gdal, COMPRESSION=9 etc\n"
                "  -d|--dimensions=D0[,D1,...]  set dimensions, e.g. W,H for 2D\n"
                "  -c|--components=C    set number of components per element\n"
                "  -t|--type=T          set type (int8, uint8, int16, uint16, int32, uint32,\n"
                "                       int64, uint64, float32, float64)\n"
                "  -n|--n=N             set number of arrays to create (default 1)\n");
        return 0;
    }
    if (!cmdLine.isSet("dimensions")) {
        fprintf(stderr, "tad create: --dimensions is missing\n");
        return 1;
    }
    if (!cmdLine.isSet("components")) {
        fprintf(stderr, "tad create: --components is missing\n");
        return 1;
    }
    if (!cmdLine.isSet("type")) {
        fprintf(stderr, "tad create: --type is missing\n");
        return 1;
    }

    const std::string& outFileName = cmdLine.arguments()[0];
    TAD::TagList exporterHints = createTagList(cmdLine.valueList("output"));
    TAD::Exporter exporter(outFileName, TAD::Overwrite, exporterHints);
    TAD::Error err = TAD::ErrorNone;
    std::vector<size_t> dimensions = getUIntList(cmdLine.value("dimensions"));
    size_t components = getUInt(cmdLine.value("components"));
    TAD::Type type = getType(cmdLine.value("type"));
    size_t n = getUInt(cmdLine.value("n"));
    TAD::ArrayContainer array(dimensions, components, type);
    std::memset(array.data(), 0, array.dataSize());
    for (size_t i = 0; i < n; i++) {
        err = exporter.writeArray(array);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad create: %s: %s\n", outFileName.c_str(), TAD::strerror(err));
            break;
        }
    }

    return (err == TAD::ErrorNone ? 0 : 1);
}

int tad_diff(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithArg("input", 'i');
    cmdLine.addOptionWithArg("output", 'o');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 3, 3, errMsg)) {
        fprintf(stderr, "tad diff: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad diff [option]... <infile0|-> <infile1|-> <outfile|->\n"
                "Compute the absolute difference.\n"
                "Options:\n"
                "  -i|--input=TAG       set input hints such as FORMAT=gdal, DPI=300 etc\n"
                "  -o|--output=TAG      set output hints such as FORMAT=gdal, COMPRESSION=9 etc\n");
        return 0;
    }

    const std::string& inFileName0 = cmdLine.arguments()[0];
    const std::string& inFileName1 = cmdLine.arguments()[1];
    const std::string& outFileName = cmdLine.arguments()[2];
    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::TagList exporterHints = createTagList(cmdLine.valueList("output"));
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

void tad_info_print_taglist(const TAD::TagList& tl, bool space = true)
{
    for (auto it = tl.cbegin(); it != tl.cend(); it++) {
        printf("%s%s=%s\n", space ? "    " : "", it->first.c_str(), it->second.c_str());
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
    cmdLine.addOptionWithArg("input", 'i');
    cmdLine.addOptionWithoutArg("statistics", 's');
    cmdLine.addOptionWithArg("box", 'b', parseUIntList);
    cmdLine.addOptionWithoutArg("dimensions", 'D');
    cmdLine.addOptionWithArg("dimension", 'd', parseUInt);
    cmdLine.addOptionWithoutArg("components", 'c');
    cmdLine.addOptionWithoutArg("type", 't');
    cmdLine.addOptionWithArg("global-tag");
    cmdLine.addOptionWithoutArg("global-tags");
    cmdLine.addOptionWithArg("dimension-tag", 0, parseUIntAndName);
    cmdLine.addOptionWithArg("dimension-tags", 0, parseUInt);
    cmdLine.addOptionWithArg("component-tag", 0, parseUIntAndName);
    cmdLine.addOptionWithArg("component-tags", 0, parseUInt);
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 1, 1, errMsg)) {
        fprintf(stderr, "tad info: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad info [option]... <infile|->\n"
                "Print information. Default output consists of an overview, all tags,\n"
                "and optionally statistics (with -s) which are optionally restricted\n"
                "to a box of interest.\n"
                "Options:\n"
                "  -i|--input=TAG       set input hints such as FORMAT=gdal, DPI=300 etc\n"
                "  -s|--statistics      print statistics\n"
                "  -b|--box=INDEX,SIZE  set box to operate on, e.g. X,Y,WIDTH,HEIGHT for 2D\n"
                "The following option disable default output:\n"
                "  -D|--dimensions      print number of dimensions\n"
                "  -d|--dimension=D     print dimension D, e.g. -d 0 for width in 2D\n"
                "  -c|--components      print number of array element components\n"
                "  -t|--type            print data type\n"
                "  --global-tag=NAME    print value of this global tag\n"
                "  --global-tags        print all global tags\n"
                "  --dimension-tag=D,N  print value of tag named N of dimension D\n"
                "  --dimension-tags=D   print all tags of dimension D\n"
                "  --component-tag=C,N  print value of tag named N of component C\n"
                "  --component-tags=C   print all tags of component C\n");
        return 0;
    }

    const std::string& inFileName = cmdLine.arguments()[0];
    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::Importer importer(inFileName, importerHints);
    TAD::Error err = TAD::ErrorNone;
    int arrayCounter = 0;
    bool defaultOutput = !(
               cmdLine.isSet("dimensions")
            || cmdLine.isSet("dimension")
            || cmdLine.isSet("components")
            || cmdLine.isSet("type")
            || cmdLine.isSet("global-tag")
            || cmdLine.isSet("global-tags")
            || cmdLine.isSet("dimension-tag")
            || cmdLine.isSet("dimension-tags")
            || cmdLine.isSet("component-tag")
            || cmdLine.isSet("component-tags"));
    std::vector<size_t> box;
    if (cmdLine.isSet("box"))
        box = getUIntList(cmdLine.value("box"));

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
        if (cmdLine.isSet("dimensions")) {
            printf("%zu\n", array.dimensionCount());
        }
        if (cmdLine.isSet("dimension")) {
            std::vector<std::string> dimList = cmdLine.valueList("dimension");
            for (size_t i = 0; i < dimList.size(); i++) {
                size_t dim = getUInt(dimList[i]);
                if (dim >= array.dimensionCount()) {
                    fprintf(stderr, "tad info: %s: no such dimension %zu\n", inFileName.c_str(), dim);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                printf("%zu\n", array.dimension(dim));
            }
        }
        if (cmdLine.isSet("components")) {
            printf("%zu\n", array.componentCount());
        }
        if (cmdLine.isSet("type")) {
            printf("%s\n", TAD::typeToString(getType(cmdLine.value("type"))));
        }
        if (cmdLine.isSet("global-tag")) {
            std::vector<std::string> nameList = cmdLine.valueList("global-tag");
            for (size_t i = 0; i < nameList.size(); i++) {
                const std::string& name = nameList[i];
                if (!array.globalTagList().contains(name)) {
                    fprintf(stderr, "tad info: %s: no global tag %s\n", inFileName.c_str(), name.c_str());
                    err = TAD::ErrorInvalidData;
                    break;
                }
                printf("%s\n", array.globalTagList().value(name).c_str());
            }
        }
        if (cmdLine.isSet("global-tags")) {
            tad_info_print_taglist(array.globalTagList(), false);
        }
        if (cmdLine.isSet("dimension-tag")) {
            std::vector<std::string> dimNameList = cmdLine.valueList("dimension-tag");
            for (size_t i = 0; i < dimNameList.size(); i++) {
                size_t dim;
                std::string name;
                getUIntAndName(dimNameList[i], &dim, &name);
                if (dim >= array.dimensionCount()) {
                    fprintf(stderr, "tad info: %s: no such dimension %zu\n", inFileName.c_str(), dim);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                if (!array.dimensionTagList(dim).contains(name)) {
                    fprintf(stderr, "tad info: %s: no tag %s for dimension %zu\n", inFileName.c_str(), name.c_str(), dim);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                printf("%s\n", array.dimensionTagList(dim).value(name).c_str());
            }
        }
        if (cmdLine.isSet("dimension-tags")) {
            std::vector<std::string> dimList = cmdLine.valueList("dimension-tags");
            for (size_t i = 0; i < dimList.size(); i++) {
                size_t dim = getUInt(dimList[i]);
                if (dim >= array.dimensionCount()) {
                    fprintf(stderr, "tad info: %s: no such dimension %zu\n", inFileName.c_str(), dim);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                tad_info_print_taglist(array.dimensionTagList(dim), false);
            }
        }
        if (cmdLine.isSet("component-tag")) {
            std::vector<std::string> compNameList = cmdLine.valueList("component-tag");
            for (size_t i = 0; i < compNameList.size(); i++) {
                size_t comp;
                std::string name;
                getUIntAndName(compNameList[i], &comp, &name);
                if (comp >= array.componentCount()) {
                    fprintf(stderr, "tad info: %s: no such component %zu\n", inFileName.c_str(), comp);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                if (!array.componentTagList(comp).contains(name)) {
                    fprintf(stderr, "tad info: %s: no tag %s for component %zu\n", inFileName.c_str(), name.c_str(), comp);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                printf("%s\n", array.componentTagList(comp).value(name).c_str());
            }
        }
        if (cmdLine.isSet("component-tags")) {
            std::vector<std::string> compList = cmdLine.valueList("component-tags");
            for (size_t i = 0; i < compList.size(); i++) {
                size_t comp = getUInt(compList[i]);
                if (comp >= array.componentCount()) {
                    fprintf(stderr, "tad info: %s: no such component %zu\n", inFileName.c_str(), comp);
                    err = TAD::ErrorInvalidData;
                    break;
                }
                tad_info_print_taglist(array.componentTagList(comp), false);
            }
        }
        if (defaultOutput) {
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
            printf("array %d: %zu x %s, size %s (%s)\n",
                    arrayCounter, array.componentCount(),
                    TAD::typeToString(array.componentType()),
                    sizeString.c_str(), tad_info_human_readable_memsize(array.dataSize()).c_str());
            if (array.globalTagList().size() > 0) {
                printf("  global:\n");
                tad_info_print_taglist(array.globalTagList());
            }
            for (size_t i = 0; i < array.dimensionCount(); i++) {
                if (array.dimensionTagList(i).size() > 0) {
                    printf("  dimension %zu:\n", i);
                    tad_info_print_taglist(array.dimensionTagList(i));
                }
            }
            for (size_t i = 0; i < array.componentCount(); i++) {
                if (array.componentTagList(i).size() > 0) {
                    printf("  component %zu:\n", i);
                    tad_info_print_taglist(array.componentTagList(i));
                }
            }
            if (cmdLine.isSet("statistics")) {
                std::vector<size_t> index(array.dimensions().size());
                std::vector<size_t> localBox;
                if (box.size() > 0) {
                    if (box.size() != index.size() * 2) {
                        fprintf(stderr, "tad info: %s: box does not match dimensions\n", inFileName.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    localBox = restrictBoxToArray(box, array);
                } else {
                    localBox = getBoxFromArray(array);
                }
                TAD::Array<float> floatArray = convert(array, TAD::float32);
                std::vector<size_t> finiteValues(array.componentCount(), 0);
                std::vector<float> tmpMinVals(array.componentCount(), 0.0f);
                std::vector<float> tmpMaxVals(array.componentCount(), 0.0f);
                std::vector<double> sums(array.componentCount(), 0.0);
                std::vector<double> sumsOfSquares(array.componentCount(), 0.0);
                if (!boxIsEmpty(localBox)) {
                    initBoxIndex(localBox, index);
                    for (;;) {
                        size_t e = array.toLinearIndex(index);
                        for (size_t i = 0; i < array.componentCount(); i++) {
                            float val = floatArray.get<float>(e, i);
                            if (std::isfinite(val)) {
                                finiteValues[i]++;
                                if (finiteValues[i] == 1) {
                                    tmpMinVals[i] = val;
                                    tmpMaxVals[i] = val;
                                } else if (val < tmpMinVals[i]) {
                                    tmpMinVals[i] = val;
                                } else if (val > tmpMaxVals[i]) {
                                    tmpMaxVals[i] = val;
                                }
                                sums[i] += val;
                                sumsOfSquares[i] += val * val;
                            }
                        }
                        if (!incBoxIndex(localBox, index))
                            break;
                    }
                }
                for (size_t i = 0; i < array.componentCount(); i++) {
                    float minVal = std::numeric_limits<float>::quiet_NaN();
                    float maxVal = std::numeric_limits<float>::quiet_NaN();
                    float sampleMean = std::numeric_limits<float>::quiet_NaN();
                    float sampleVariance = std::numeric_limits<float>::quiet_NaN();
                    float sampleDeviation = std::numeric_limits<float>::quiet_NaN();
                    if (finiteValues[i] > 0) {
                        minVal = tmpMinVals[i];
                        maxVal = tmpMaxVals[i];
                        sampleMean = sums[i] / finiteValues[i];
                        if (finiteValues[i] > 1) {
                            sampleVariance = (sumsOfSquares[i] - sums[i] / finiteValues[i] * sums[i]) / (finiteValues[i] - 1);
                            if (sampleVariance < 0.0f)
                                sampleVariance = 0.0f;
                            sampleDeviation = std::sqrt(sampleVariance);
                        } else if (finiteValues[i] == 1) {
                            sampleVariance = 0.0f;
                            sampleDeviation = 0.0f;
                        }
                    }
                    printf("  component %zu: min=%g max=%g mean=%g var=%g dev=%g\n", i,
                            minVal, maxVal, sampleMean, sampleVariance, sampleDeviation);
                }
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
    } else if (std::strcmp(argv[1], "create") == 0) {
        retval = tad_create(argc - 1, &(argv[1]));
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
