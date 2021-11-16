/*
 * Copyright (C) 2019, 2020, 2021
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

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>
#ifdef _WIN32
# include <fcntl.h>
#endif

#include <string>
#include <vector>

#ifdef TAD_WITH_MUPARSER
# include <chrono>
# include <random>
# include <muParser.h>
#endif

#include "array.hpp"
#include "io.hpp"
#include "foreach.hpp"
#include "operators.hpp"

#include "cmdline.hpp"


/* Helper functions to parse command line options */

bool parseUnderscore(const std::string& value)
{
    size_t i = value.find_first_of('_');
    return (i != std::string::npos && i == value.find_last_of('_')
            && value.find_first_not_of(" _") == std::string::npos);
}

bool parseUInt(const std::string& value, bool allowZero)
{
    bool ok = true;
    long long val = -1;
    size_t idx = 0;
    try {
        // read a signed value and discard negative values later since stoull accepts "-1" (why??)
        val = std::stoll(value, &idx);
    }
    catch (...) {
        ok = false;
    }
    if (ok && (idx != value.length() || val < 0 || (!allowZero && val == 0))) {
        ok = false;
    }
    return ok;
}

bool parseUInt(const std::string& value)
{
    return parseUInt(value, true);
}

bool parseUIntUnderscore(const std::string& value)
{
    return (parseUnderscore(value) || parseUInt(value, true));
}

bool parseUIntLargerThanZero(const std::string& value)
{
    return parseUInt(value, false);
}

size_t getUInt(const std::string& value)
{
    return std::stoull(value);
}

static const size_t underscoreValue = std::numeric_limits<size_t>::max();

size_t getUIntUnderscore(const std::string& value)
{
    return (parseUnderscore(value) ? underscoreValue : getUInt(value));
}

bool parseUIntList(const std::string& value, bool allowZero, bool allowUnderscore)
{
    for (size_t i = 0; i <= value.length();) {
        size_t j = value.find_first_of(',', i);
        std::string singleValue = value.substr(i, (j == std::string::npos ? std::string::npos : j - i));
        if (!((allowUnderscore && parseUnderscore(singleValue)) || parseUInt(singleValue, allowZero)))
            return false;
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
    return true;
}

bool parseUIntList(const std::string& value)
{
    return parseUIntList(value, true, false);
}

bool parseUIntLargerThanZeroList(const std::string& value)
{
    return parseUIntList(value, false, false);
}

bool parseUIntUnderscoreList(const std::string& value)
{
    return parseUIntList(value, true, true);
}

std::vector<size_t> getUIntList(const std::string& value, bool allowUnderscore)
{
    std::vector<size_t> values;
    for (size_t i = 0; i < value.length();) {
        size_t j = value.find_first_of(',', i);
        std::string singleValue = value.substr(i, (j == std::string::npos ? std::string::npos : j - i));
        if (allowUnderscore && parseUnderscore(singleValue))
            values.push_back(underscoreValue);
        else
            values.push_back(getUInt(singleValue));
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
    return values;
}

std::vector<size_t> getUIntList(const std::string& value)
{
    return getUIntList(value, false);
}

std::vector<size_t> getUIntUnderscoreList(const std::string& value)
{
    return getUIntList(value, true);
}

bool parseType(const std::string& value)
{
    TAD::Type t;
    return TAD::typeFromString(value, &t);
}

TAD::Type getType(const std::string& value)
{
    TAD::Type t = TAD::uint8;
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

bool parseNameAndValue(const std::string& value)
{
    size_t i = value.find_first_of('=');
    return (i != std::string::npos && i > 0);
}

void getNameAndValue(const std::string& value, std::string* n, std::string* v)
{
    size_t i = value.find_first_of('=');
    *n = value.substr(0, i);
    *v = value.substr(i + 1);
}

bool parseUIntAndNameAndValue(const std::string& value)
{
    if (parseUIntAndName(value)) {
        size_t u;
        std::string v;
        getUIntAndName(value, &u, &v);
        return parseNameAndValue(v);
    } else {
        return false;
    }
}

void getUIntAndNameAndValue(const std::string& value, size_t* u, std::string* n, std::string* v)
{
    std::string t;
    getUIntAndName(value, u, &t);
    getNameAndValue(t, n, v);
}

bool parseRange(const std::string& value)
{
    bool aOk = false;
    bool bOk = false;
    bool sOk = false;

    size_t comma = value.find_first_of(',');
    sOk = (comma == std::string::npos || parseUInt(value.substr(comma + 1)));
    std::string val = value.substr(0, comma);

    size_t dash = val.find_first_of('-');
    if (dash == std::string::npos) {
        aOk = parseUInt(val);
        bOk = aOk;
    } else {
        aOk = (dash == 0 || parseUInt(val.substr(0, dash)));
        bOk = (dash == val.length() - 1 || parseUInt(val.substr(dash + 1)));
    }

    return aOk && bOk && sOk;
}

void getRange(const std::string& value, size_t* a, size_t* b, size_t* s)
{
    size_t comma = value.find_first_of(',');
    *s = (comma == std::string::npos ? 1 : getUInt(value.substr(comma + 1)));
    std::string val = value.substr(0, comma);

    size_t dash = val.find_first_of('-');
    if (dash == std::string::npos) {
        *a = getUInt(val);
        *b = *a;
    } else {
        *a = (dash == 0 ? 0 : getUInt(val.substr(0, dash)));
        *b = (dash == val.length() - 1 ? std::numeric_limits<size_t>::max() : getUInt(val.substr(dash + 1)));
    }
}

bool indexInRange(size_t i, size_t a, size_t b, size_t s)
{
    return (i >= a && i <= b && (i - a) % s == 0);
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

bool incBoxIndex(const std::vector<size_t>& box, std::vector<size_t>& index, size_t incDim = 0)
{
    if (index.size() == 0 || box.size() != index.size() * 2 || boxIsEmpty(box))
        return false;
    size_t dimToInc = incDim;
    while (index[dimToInc] == box[dimToInc] + box[index.size() + dimToInc] - 1) {
        if (dimToInc == index.size() - 1)
            return false;
        dimToInc++;
    }
    index[dimToInc]++;
    for (size_t j = 0; j < dimToInc; j++) {
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
            "  create\n"
            "  convert\n"
            "  calc\n"
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
                "\n"
                "Create zero-filled arrays.\n"
                "\n"
                "Options:\n"
                "  -o|--output=TAG            set output hints such as FORMAT=gdal etc.\n"
                "  -d|--dimensions=D0[,D1,...]  set dimensions, e.g. W,H for 2D\n"
                "  -c|--components=C          set number of components per element\n"
                "  -t|--type=T                set type (int8, uint8, int16, uint16, int32,\n"
                "                             uint32, int64, uint64, float32, float64)\n"
                "  -n|--n=N                   set number of arrays to create (default 1)\n");
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
    cmdLine.addOptionWithArg("input", 'i');
    cmdLine.addOptionWithArg("output", 'o');
    cmdLine.addOptionWithArg("keep", 'k', parseRange);
    cmdLine.addOptionWithArg("drop", 'd', parseRange);
    cmdLine.addOptionWithoutArg("split", 's');
    cmdLine.addOptionWithoutArg("append", 'a');
    cmdLine.addOptionWithArg("merge-dimension", 'D', parseUIntUnderscore);
    cmdLine.addOptionWithoutArg("merge-components", 'C');
    cmdLine.addOptionWithArg("box", 'b', parseUIntList);
    cmdLine.addOptionWithArg("dimensions", 'd', parseUIntUnderscoreList);
    cmdLine.addOptionWithArg("components", 'c', parseUIntUnderscoreList);
    cmdLine.addOptionWithArg("type", 't', parseType);
    cmdLine.addOptionWithoutArg("normalize", 'n');
    cmdLine.addOrderedOptionWithoutArg("unset-all-tags");
    cmdLine.addOrderedOptionWithArg("global-tag", 0, parseNameAndValue);
    cmdLine.addOrderedOptionWithArg("unset-global-tag");
    cmdLine.addOrderedOptionWithoutArg("unset-global-tags");
    cmdLine.addOrderedOptionWithArg("dimension-tag", 0, parseUIntAndNameAndValue);
    cmdLine.addOrderedOptionWithArg("unset-dimension-tag", 0, parseUIntAndName);
    cmdLine.addOrderedOptionWithArg("unset-dimension-tags", 0, parseUInt);
    cmdLine.addOrderedOptionWithArg("component-tag", 0, parseUIntAndNameAndValue);
    cmdLine.addOrderedOptionWithArg("unset-component-tag", 0, parseUIntAndName);
    cmdLine.addOrderedOptionWithArg("unset-component-tags", 0, parseUInt);
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 2, -1, errMsg)) {
        fprintf(stderr, "tad convert: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad convert [option]... <infile|-> [<infile...>] <outfile|->\n"
                "\n"
                "Convert input(s) and write to a new output.\n"
                "\n"
                "Options:\n"
                "  -i|--input=TAG             set input hints such as FORMAT=gdal, DPI=300 etc.\n"
                "  -o|--output=TAG            set output hints such as FORMAT=gdal etc.\n"
                "  -k|--keep=A-B[,S]          keep the specified arrays, drop others\n"
                "  -d|--drop=A-B[,S]          drop the specified arrays, keep others\n"
                "  -s|--split                 split input into multiple output files named with\n"
                "                             file name template sequence %%[n]N (see manual)\n"
                "  -a|--append                append to the output file instead of overwriting\n"
                "                             (not always possible)\n"
                "  -D|--merge-dimension=D     merge input arrays along the given dimension (e.g.\n"
                "                             -D0 to arrange images left to right); the special\n"
                "                             value _ will create a new dimension (e.g. to\n"
                "                             combine images into a 3D volume)\n"
                "  -C|--merge-components      merge input arrays at the component level, e.g. to\n"
                "                             combine an RGB image and an alpha mask into an RGBA\n"
                "                             image file\n"
                "  -b|--box=INDEX,SIZE        set box to operate on, e.g. X,Y,WIDTH,HEIGHT for 2D\n"
                "  -d|--dimensions=LIST       copy these input dimensions to the output in the\n"
                "                             given order; the special entry _ will create a new\n"
                "                             dimension of size 1\n"
                "  -c|--components=LIST       copy these input components to the output in the\n"
                "                             given order; the special entry _ will create a new\n"
                "                             component initialized to zero\n"
                "  -t|--type=T                convert to new type (int8, uint8, int16, uint16,\n"
                "                             int32, uint32, int64, uint64, float32, float64)\n"
                "  -n|--normalize             create/assume floating point values in [-1,1]/[0,1]\n"
                "                             when converting to/from signed/unsigned integers\n"
                "  --unset-all-tags           unset all tags\n"
                "  --global-tag=N=V           set global tag N to value V\n"
                "  --unset-global-tag=N       unset global tag N\n"
                "  --unset-global-tags        unset all global tags\n"
                "  --dimension-tag=D,N=V      set tag N of dimension D to value V\n"
                "  --unset-dimension-tag=D,N  unset tag N of dimension D\n"
                "  --unset-dimension-tags=D   unset all tags of dimension D\n"
                "  --component-tag=C,N=V      set tag N of component C to value V\n"
                "  --unset-component-tag=C,N  unset tag N of component C\n"
                "  --unset-component-tags=C   unset all tags of component C\n");
        return 0;
    }
    if (cmdLine.isSet("keep") && cmdLine.isSet("drop")) {
        fprintf(stderr, "tad convert: cannot use both --keep and --drop\n");
        return 1;
    }

    TAD::TagList exporterHints = createTagList(cmdLine.valueList("output"));
    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::Type type = getType(cmdLine.value("type"));

    bool mergeComponents = cmdLine.isSet("merge-components");
    bool mergeDimension = cmdLine.isSet("merge-dimension");
    if (mergeComponents && mergeDimension) {
        fprintf(stderr, "tad convert: cannot use both --merge-components and --merge-dimension\n");
        return 1;
    }
    size_t mergeDimensionArg = (mergeDimension ? getUIntUnderscore(cmdLine.value("merge-dimension")) : 0);

    std::vector<size_t> box;
    if (cmdLine.isSet("box"))
        box = getUIntList(cmdLine.value("box"));

    std::vector<size_t> dimensions;
    if (cmdLine.isSet("dimensions")) {
        dimensions = getUIntUnderscoreList(cmdLine.value("dimensions"));
        if (dimensions.size() == 0) {
            fprintf(stderr, "tad convert: --dimensions must not be empty\n");
            return 1;
        }
        for (size_t i = 0; i < dimensions.size(); i++) {
            if (dimensions[i] != underscoreValue) {
                for (size_t j = 0; j < i; j++) {
                    if (dimensions[i] == dimensions[j]) {
                        fprintf(stderr, "tad convert: --dimensions list must not contain duplicates\n");
                        return 1;
                    }
                }
            }
        }
    }

    std::vector<size_t> components;
    if (cmdLine.isSet("components")) {
        components = getUIntUnderscoreList(cmdLine.value("components"));
        if (components.size() == 0) {
            fprintf(stderr, "tad convert: --components must not be empty\n");
            return 1;
        }
    }

    std::vector<size_t> A, B, S;
    std::vector<std::string> ranges;
    if (cmdLine.isSet("keep")) {
        ranges = cmdLine.valueList("keep");
    } else if (cmdLine.isSet("drop")) {
        ranges = cmdLine.valueList("drop");
    }
    A.resize(ranges.size());
    B.resize(ranges.size());
    S.resize(ranges.size());
    for (size_t i = 0; i < ranges.size(); i++) {
        getRange(ranges[i], &(A[i]), &(B[i]), &(S[i]));
    }

    std::string splitTemplate;
    size_t splitTemplateFirstIndex = 0;
    size_t splitTemplateLastIndex = 0;
    size_t splitTemplateFieldWidth = 0;
    std::string outFileName;
    TAD::Exporter exporter;
    if (cmdLine.isSet("split")) {
        splitTemplate = cmdLine.arguments()[cmdLine.arguments().size() - 1];
        splitTemplateFirstIndex = splitTemplate.find_first_of('%');
        splitTemplateLastIndex = splitTemplate.find_first_of('N', splitTemplateFirstIndex);
        size_t l = splitTemplateLastIndex - splitTemplateFirstIndex - 1;
        if (splitTemplateFirstIndex == std::string::npos
                || splitTemplateLastIndex == std::string::npos
                || (l > 0 && !parseUIntLargerThanZero(splitTemplate.substr(splitTemplateFirstIndex + 1, l)))) {
            fprintf(stderr, "tad convert: --split: output file template does not contain valid %%[n]N\n");
            return 1;
        }
        splitTemplateFieldWidth = (l == 0 ? 6 : getUInt(splitTemplate.substr(splitTemplateFirstIndex + 1, l)));
    } else {
        outFileName = cmdLine.arguments()[cmdLine.arguments().size() - 1];
        exporter.initialize(outFileName, cmdLine.isSet("append") ? TAD::Append : TAD::Overwrite, exporterHints);
    }

    TAD::Error err = TAD::ErrorNone;
    size_t arrayIndex = 0;
    std::vector<TAD::Importer> importers(cmdLine.arguments().size() - 1);
    for (size_t i = 0; i < importers.size(); i++)
        importers[i].initialize(cmdLine.arguments()[i], importerHints);
    bool loopOverInputArgs = !mergeComponents && !mergeDimension;
    for (size_t i = 0; i < (loopOverInputArgs ? importers.size() : 1); i++) {
        for (;;) {
            if (!importers[i].hasMore(&err)) {
                if (err != TAD::ErrorNone) {
                    fprintf(stderr, "tad convert: %s: %s\n", importers[i].fileName().c_str(), TAD::strerror(err));
                }
                break;
            }
            TAD::ArrayContainer array;
            std::string inputName;
            if (!mergeComponents && !mergeDimension) {
                array = importers[i].readArray(&err);
                if (err != TAD::ErrorNone) {
                    fprintf(stderr, "tad convert: %s: %s\n", importers[i].fileName().c_str(), TAD::strerror(err));
                    break;
                }
                inputName = importers[i].fileName() + std::string(" array ") + std::to_string(arrayIndex);
            } else {
                std::vector<TAD::ArrayContainer> arrays(importers.size());
                for (size_t j = 0; j < importers.size(); j++) {
                    arrays[j] = importers[j].readArray(&err);
                    if (err != TAD::ErrorNone) {
                        fprintf(stderr, "tad convert: %s: %s\n", importers[j].fileName().c_str(), TAD::strerror(err));
                        break;
                    }
                }
                if (err != TAD::ErrorNone)
                    break;
                inputName = std::string("merged array ") + std::to_string(arrayIndex);
                if (mergeComponents) {
                    size_t componentCount = 0;
                    bool compatible = true;
                    for (size_t j = 1; j < importers.size(); j++) {
                        if (arrays[j].componentType() != arrays[0].componentType()
                                || arrays[j].dimensionCount() != arrays[0].dimensionCount()) {
                            compatible = false;
                            break;
                        }
                        for (size_t k = 0; k < arrays[0].dimensionCount(); k++) {
                            if (arrays[j].dimension(k) != arrays[0].dimension(k)) {
                                compatible = false;
                                break;
                            }
                        }
                        if (!compatible)
                            break;
                        componentCount += arrays[j].componentCount();
                    }
                    if (!compatible) {
                        fprintf(stderr, "tad convert: %s: incompatible input arrays\n", inputName.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    array = TAD::ArrayContainer(arrays[0].dimensions(), componentCount, arrays[0].componentType());
                    for (size_t e = 0; e < array.elementCount(); e++) {
                        unsigned char* dst = static_cast<unsigned char*>(array.get(e));
                        for (size_t j = 0; j < arrays.size(); j++) {
                            const unsigned char* src = static_cast<const unsigned char*>(arrays[j].get(e));
                            std::memcpy(dst, src, arrays[j].elementSize());
                            dst += arrays[j].elementSize();
                        }
                    }
                    array.globalTagList() = arrays[0].globalTagList();
                    for (size_t j = 0; j < array.dimensionCount(); j++)
                        array.dimensionTagList(j) = arrays[0].dimensionTagList(j);
                    size_t componentIndex = 0;
                    for (size_t j = 0; j < arrays.size(); j++)
                        for (size_t k = 0; k < arrays[j].componentCount(); k++)
                            array.componentTagList(componentIndex++) = arrays[j].componentTagList(k);
                } else { // mergeDimension
                    bool compatible = true;
                    if (mergeDimensionArg != underscoreValue && mergeDimensionArg >= arrays[0].dimensionCount()) {
                        fprintf(stderr, "tad convert: %s: no dimension %zu\n", inputName.c_str(), mergeDimensionArg);
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    for (size_t j = 1; j < importers.size(); j++) {
                        if (arrays[j].componentType() != arrays[0].componentType()
                                || arrays[j].componentCount() != arrays[0].componentCount()
                                || arrays[j].dimensionCount() != arrays[0].dimensionCount()) {
                            compatible = false;
                            break;
                        }
                        for (size_t k = 0; k < arrays[0].dimensionCount(); k++) {
                            if (k == mergeDimension)
                                continue;
                            if (arrays[j].dimension(k) != arrays[0].dimension(k)) {
                                compatible = false;
                                break;
                            }
                        }
                        if (!compatible)
                            break;
                    }
                    if (!compatible) {
                        fprintf(stderr, "tad convert: %s: incompatible input arrays\n", inputName.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    std::vector<size_t> dimensions = arrays[0].dimensions();
                    if (mergeDimensionArg == underscoreValue) {
                        dimensions.push_back(arrays.size());
                    } else {
                        for (size_t j = 1; j < arrays.size(); j++)
                            dimensions[mergeDimensionArg] += arrays[j].dimension(mergeDimensionArg);
                    }
                    array = TAD::ArrayContainer(dimensions, arrays[0].componentCount(), arrays[0].componentType());

                    std::vector<size_t> blockSizes(arrays.size(), 0);
                    size_t blockSizeSum = 0;
                    for (size_t j = 0; j < arrays.size(); j++) {
                        size_t elementsInBlock = 1;
                        for (size_t k = 0; k < array.dimensionCount(); k++) {
                            if (k < arrays[j].dimensionCount())
                                elementsInBlock *= arrays[j].dimension(k);
                            if (k == mergeDimensionArg)
                                break;
                        }
                        blockSizes[j] = elementsInBlock * array.elementSize();
                        blockSizeSum += blockSizes[j];
                    }
                    unsigned char* dst = static_cast<unsigned char*>(array.data());
                    for (size_t block = 0; block * blockSizeSum < array.dataSize(); block++) {
                        for (size_t j = 0; j < arrays.size(); j++) {
                            const unsigned char* src = static_cast<const unsigned char*>(arrays[j].data()) + block * blockSizes[j];
                            std::memcpy(dst, src, blockSizes[j]);
                            dst += blockSizes[j];
                        }
                    }

                    array.globalTagList() = arrays[0].globalTagList();
                    for (size_t j = 0; j < arrays[0].dimensionCount(); j++)
                        array.dimensionTagList(j) = arrays[0].dimensionTagList(j);
                    for (size_t j = 0; j < array.componentCount(); j++)
                        array.componentTagList(j) = arrays[0].componentTagList(j);
                }
            }
            bool keep = true;
            if (cmdLine.isSet("keep")) {
                keep = false;
                for (size_t i = 0; i < ranges.size(); i++) {
                    if (indexInRange(arrayIndex, A[i], B[i], S[i])) {
                        keep = true;
                        break;
                    }
                }
            } else if (cmdLine.isSet("drop")) {
                for (size_t i = 0; i < ranges.size(); i++) {
                    if (indexInRange(arrayIndex, A[i], B[i], S[i])) {
                        keep = false;
                        break;
                    }
                }
            }
            if (keep) {
                if (box.size() > 0) {
                    if (box.size() != array.dimensionCount() * 2) {
                        fprintf(stderr, "tad convert: %s: box does not match dimensions\n", inputName.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    std::vector<size_t> localBox = restrictBoxToArray(box, array);
                    if (boxIsEmpty(localBox)) {
                        fprintf(stderr, "tad convert: %s: empty box\n", inputName.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    TAD::ArrayContainer arrayBox(
                            std::vector<size_t>(localBox.begin() + array.dimensionCount(), localBox.end()),
                            array.componentCount(), array.componentType());
                    std::vector<size_t> arrayIndex(array.dimensionCount());
                    std::vector<size_t> boxIndex(array.dimensionCount());
                    initBoxIndex(localBox, arrayIndex);
                    size_t lowestDimBoxSize = localBox[array.dimensionCount()];
                    for (;;) {
                        for (size_t i = 0; i < array.dimensionCount(); i++)
                            boxIndex[i] = arrayIndex[i] - localBox[i];
                        std::memcpy(arrayBox.get(boxIndex), array.get(arrayIndex),
                                lowestDimBoxSize * array.elementSize());
                        if (!incBoxIndex(localBox, arrayIndex, 1))
                            break;
                    }
                    arrayBox.globalTagList() = array.globalTagList();
                    for (size_t i = 0; i < array.dimensionCount(); i++)
                        arrayBox.dimensionTagList(i) = array.dimensionTagList(i);
                    for (size_t i = 0; i < array.componentCount(); i++)
                        arrayBox.componentTagList(i) = array.componentTagList(i);
                    array = arrayBox;
                }
                if (cmdLine.isSet("dimensions")) {
                    bool valid = true;
                    for (size_t i = 0; i < dimensions.size(); i++) {
                        if (dimensions[i] != underscoreValue && dimensions[i] >= array.dimensionCount()) {
                            fprintf(stderr, "tad convert: %s: no dimension %zu\n", inputName.c_str(), dimensions[i]);
                            err = TAD::ErrorInvalidData;
                            valid = false;
                            break;
                        }
                    }
                    if (!valid)
                        break;
                    std::vector<size_t> dimensionsNew(dimensions.size());
                    std::vector<size_t> srcIndexMap(array.dimensionCount(), underscoreValue);
                    for (size_t i = 0; i < dimensions.size(); i++) {
                        if (dimensions[i] == underscoreValue) {
                            dimensionsNew[i] = 1;
                        } else {
                            dimensionsNew[i] = array.dimension(dimensions[i]);
                            srcIndexMap[dimensions[i]] = i;
                        }
                    }
                    TAD::ArrayContainer arrayNew(dimensionsNew, array.componentCount(), array.componentType());
                    std::vector<size_t> srcIndex(array.dimensionCount());
                    std::vector<size_t> dstIndex(arrayNew.dimensionCount());
                    for (size_t e = 0; e < arrayNew.elementCount(); e++) {
                        void* dst = arrayNew.get(e);
                        arrayNew.toVectorIndex(e, dstIndex.data());
                        for (size_t i = 0; i < srcIndex.size(); i++)
                            srcIndex[i] = (srcIndexMap[i] == underscoreValue ? 0 : dstIndex[srcIndexMap[i]]);
                        void* src = array.get(srcIndex);
                        std::memcpy(dst, src, array.elementSize());
                    }
                    arrayNew.globalTagList() = array.globalTagList();
                    for (size_t i = 0; i < arrayNew.dimensionCount(); i++)
                        if (dimensions[i] != underscoreValue)
                            arrayNew.dimensionTagList(i) = array.dimensionTagList(dimensions[i]);
                    for (size_t i = 0; i < arrayNew.componentCount(); i++)
                        arrayNew.componentTagList(i) = array.componentTagList(i);
                    array = arrayNew;
                }
                if (cmdLine.isSet("components")) {
                    bool valid = true;
                    for (size_t i = 0; i < components.size(); i++) {
                        if (components[i] != underscoreValue && components[i] >= array.componentCount()) {
                            fprintf(stderr, "tad convert: %s: no component %zu\n", inputName.c_str(), components[i]);
                            err = TAD::ErrorInvalidData;
                            valid = false;
                            break;
                        }
                    }
                    if (!valid)
                        break;
                    TAD::ArrayContainer arrayNew(array.dimensions(), components.size(), array.componentType());
                    for (size_t i = 0; i < components.size(); i++) {
                        for (size_t e = 0; e < array.elementCount(); e++) {
                            unsigned char* dst = reinterpret_cast<unsigned char*>(arrayNew.get(e));
                            dst += i * array.componentSize();
                            if (components[i] == underscoreValue) {
                                std::memset(dst, 0, array.componentSize());
                            } else {
                                assert(components[i] < array.componentCount());
                                const unsigned char* src = reinterpret_cast<const unsigned char*>(array.get(e));
                                src += components[i] * array.componentSize();
                                std::memcpy(dst, src, array.componentSize());
                            }
                        }
                    }
                    arrayNew.globalTagList() = array.globalTagList();
                    for (size_t i = 0; i < array.dimensionCount(); i++)
                        arrayNew.dimensionTagList(i) = array.dimensionTagList(i);
                    for (size_t i = 0; i < arrayNew.componentCount(); i++)
                        if (components[i] != underscoreValue)
                            arrayNew.componentTagList(i) = array.componentTagList(components[i]);
                    array = arrayNew;
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
                for (size_t o = 0; o < cmdLine.orderedOptionNames().size(); o++) {
                    const std::string& optName = cmdLine.orderedOptionNames()[o];
                    const std::string& optVal = cmdLine.orderedOptionValues()[o];
                    if (optName == "unset-all-tags") {
                        array.globalTagList().clear();
                        for (size_t d = 0; d < array.dimensionCount(); d++)
                            array.dimensionTagList(d).clear();
                        for (size_t c = 0; c < array.componentCount(); c++)
                            array.componentTagList(c).clear();
                    } else if (optName == "global-tag") {
                        std::string n, v;
                        getNameAndValue(optVal, &n, &v);
                        array.globalTagList().set(n, v);
                    } else if (optName == "unset-global-tag") {
                        array.globalTagList().unset(optVal);
                    } else if (optName == "unset-global-tags") {
                        array.globalTagList().clear();
                    } else if (optName == "dimension-tag") {
                        size_t d;
                        std::string n, v;
                        getUIntAndNameAndValue(optVal, &d, &n, &v);
                        if (d >= array.dimensionCount()) {
                            fprintf(stderr, "tad convert: %s: no such dimension %zu\n", inputName.c_str(), d);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.dimensionTagList(d).set(n, v);
                    } else if (optName == "unset-dimension-tag") {
                        size_t d;
                        std::string n;
                        getUIntAndName(optVal, &d, &n);
                        if (d >= array.dimensionCount()) {
                            fprintf(stderr, "tad convert: %s: no such dimension %zu\n", inputName.c_str(), d);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.dimensionTagList(d).unset(n);
                    } else if (optName == "unset-dimension-tags") {
                        size_t d = getUInt(optVal);
                        if (d >= array.dimensionCount()) {
                            fprintf(stderr, "tad convert: %s: no such dimension %zu\n", inputName.c_str(), d);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.dimensionTagList(d).clear();
                    } else if (optName == "component-tag") {
                        size_t c;
                        std::string n, v;
                        getUIntAndNameAndValue(optVal, &c, &n, &v);
                        if (c >= array.componentCount()) {
                            fprintf(stderr, "tad convert: %s: no such component %zu\n", inputName.c_str(), c);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.componentTagList(c).set(n, v);
                    } else if (optName == "unset-component-tag") {
                        size_t c;
                        std::string n;
                        getUIntAndName(optVal, &c, &n);
                        if (c >= array.componentCount()) {
                            fprintf(stderr, "tad convert: %s: no such component %zu\n", inputName.c_str(), c);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.componentTagList(c).unset(n);
                    } else if (optName == "unset-component-tags") {
                        size_t c = getUInt(optVal);
                        if (c >= array.componentCount()) {
                            fprintf(stderr, "tad convert: %s: no such component %zu\n", inputName.c_str(), c);
                            err = TAD::ErrorInvalidData;
                            break;
                        }
                        array.componentTagList(c).clear();
                    }
                }
                if (err != TAD::ErrorNone) {
                    break;
                }
                if (cmdLine.isSet("split")) {
                    std::string arrayIndexString = std::to_string(arrayIndex);
                    outFileName = splitTemplate.substr(0, splitTemplateFirstIndex);
                    if (arrayIndexString.length() < splitTemplateFieldWidth)
                        outFileName += std::string(splitTemplateFieldWidth - arrayIndexString.length(), '0');
                    outFileName += arrayIndexString;
                    outFileName += splitTemplate.substr(splitTemplateLastIndex + 1);
                    exporter.initialize(outFileName, cmdLine.isSet("append") ? TAD::Append : TAD::Overwrite, exporterHints);
                }
                err = exporter.writeArray(array);
                if (err != TAD::ErrorNone) {
                    fprintf(stderr, "tad convert: %s: %s\n", outFileName.c_str(), TAD::strerror(err));
                    break;
                }
            }
            arrayIndex++;
        }
        if (err != TAD::ErrorNone) {
            break;
        }
    }

    return (err == TAD::ErrorNone ? 0 : 1);
}

#ifdef TAD_WITH_MUPARSER
class Calc;
Calc* calcSingleton;

class Calc
{
public:
    // limitations
    static const size_t maxDimensionCount = 10;
    static const size_t maxComponentCount = 32;

private:
    // the expressions
    const std::vector<std::string>& expressions;
    // the parsers
    std::vector<mu::Parser> parsers;
    // user-defined variable management
    size_t expressionIndex;
    std::vector<std::vector<std::pair<std::string, std::unique_ptr<double>>>> added_vars;
    // pseudo-random numbers
    std::mt19937_64 prng;
    std::uniform_real_distribution<double> uniform_distrib;
    std::normal_distribution<double> gaussian_distrib;
    // variables
    double var_array_count;
    double var_stream_index;
    double var_dimensions;
    std::vector<double> var_dim;
    double var_components;
    std::vector<double> var_box;
    std::vector<double> var_boxdim;
    double var_index;
    std::vector<double> var_i;
    std::vector<double> var_v;

    static constexpr double pi = 3.1415926535897932384626433832795029;
    static constexpr double e = 2.7182818284590452353602874713526625;

    static double mod(double x, double y) { return x - y * floor(x / y); }

    static double deg(double x) { return x * 180.0 / pi; }
    static double rad(double x) { return x * pi / 180.0; }
    static double atan2(double y, double x) { return std::atan2(y, x); }
    static double pow(double x, double y) { return std::pow(x, y); }
    static double exp2(double x) { return std::exp2(x); }
    static double cbrt(double x) { return std::cbrt(x); }
    static double int_(double x) { return (long long)x; }
    static double ceil(double x) { return std::ceil(x); }
    static double floor(double x) { return std::floor(x); }
    static double round(double x) { return std::round(x); }
    static double trunc(double x) { return std::trunc(x); }
    static double fract(double x) { return x - floor(x); }

    static double med(const double* x, int n)
    {
        std::vector<double> values(x, x + n);
        std::sort(values.begin(), values.end());
        if (n % 2 == 1) {
            return values[n / 2];
        } else {
            return (values[n / 2 - 1] + values[n / 2]) / 2.0;
        }
    }

    static double clamp(double x, double minval, double maxval) { return std::min(maxval, std::max(minval, x)); }
    static double step(double x, double edge) { return (x < edge ? 0.0 : 1.0); }
    static double smoothstep(double x, double edge0, double edge1) { double t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); return t * t * (3.0 - t * 2.0); }
    static double mix(double x, double y, double t) { return x * (1.0 - t) + y * t; }

    static double unary_plus(double x) { return x; }

    static double seed(double x) { calcSingleton->prng.seed(x); return 0.0; }
    static double random() { return calcSingleton->uniform_distrib(calcSingleton->prng); }
    static double gaussian() { return calcSingleton->gaussian_distrib(calcSingleton->prng); }

    static double* add_var(const char* name, void* expressionIndexVoid)
    {
        size_t expressionIndex = *reinterpret_cast<size_t*>(expressionIndexVoid);
        calcSingleton->added_vars[expressionIndex].push_back(std::make_pair(
                    std::string(name), std::unique_ptr<double>(new double(0.0))));
        return calcSingleton->added_vars[expressionIndex].back().second.get();
    }

    static double input_value(size_t a, size_t e, size_t c)
    {
        const std::vector<TAD::ArrayContainer>& input_arrays = calcSingleton->input_arrays;
        double v = std::numeric_limits<double>::quiet_NaN();
        switch (input_arrays[a].componentType()) {
        case TAD::int8:
            v = input_arrays[a].get<int8_t>(e)[c];
            break;
        case TAD::uint8:
            v = input_arrays[a].get<uint8_t>(e)[c];
            break;
        case TAD::int16:
            v = input_arrays[a].get<int16_t>(e)[c];
            break;
        case TAD::uint16:
            v = input_arrays[a].get<uint16_t>(e)[c];
            break;
        case TAD::int32:
            v = input_arrays[a].get<int32_t>(e)[c];
            break;
        case TAD::uint32:
            v = input_arrays[a].get<uint32_t>(e)[c];
            break;
        case TAD::int64:
            v = input_arrays[a].get<int64_t>(e)[c];
            break;
        case TAD::uint64:
            v = input_arrays[a].get<uint64_t>(e)[c];
            break;
        case TAD::float32:
            v = input_arrays[a].get<float>(e)[c];
            break;
        case TAD::float64:
            v = input_arrays[a].get<double>(e)[c];
            break;
        }
        return v;
    }

    static double v(const double* dx, int n)
    {
        const std::vector<TAD::ArrayContainer>& input_arrays = calcSingleton->input_arrays;

        if (n != 3 && n != static_cast<int>(input_arrays[0].dimensionCount() + 2))
            return std::numeric_limits<double>::quiet_NaN();

        size_t a = 0;
        size_t c = 0;
        std::vector<size_t> index(n - 2);
        for (int i = 0; i < n; i++) {
            long long tmp = dx[i];
            if (i == 0) {
                if (tmp < 0) {
                    a = 0;
                } else if (static_cast<size_t>(tmp) >= input_arrays.size()) {
                    a = input_arrays.size() - 1;
                } else {
                    a = tmp;
                }
            } else if (i == n - 1) {
                if (tmp < 0) {
                    c = 0;
                } else if (static_cast<size_t>(tmp) >= input_arrays[a].componentCount()) {
                    c = input_arrays[a].componentCount() - 1;
                } else {
                    c = tmp;
                }
            } else {
                if (tmp < 0) {
                    index[i - 1] = 0;
                } else if (n == 3 && static_cast<size_t>(tmp) >= input_arrays[a].elementCount()) {
                    index[i - 1] = input_arrays[a].elementCount() - 1;
                } else if (n != 3 && static_cast<size_t>(tmp) >= input_arrays[a].dimension(i - 1)) {
                    index[i - 1] = input_arrays[a].dimension(i - 1) - 1;
                } else {
                    index[i - 1] = tmp;
                }
            }
        }
        size_t linearIndex;
        if (n == 3) {
            linearIndex = index[0];
        } else {
            linearIndex = input_arrays[a].toLinearIndex(index);
        }

        double r = input_value(a, linearIndex, c);
        return r;
    }

    static double copy(const double* dx, int n)
    {
        const std::vector<TAD::ArrayContainer>& input_arrays = calcSingleton->input_arrays;

        if (n != 2 && n != static_cast<int>(input_arrays[0].dimensionCount() + 1))
            return std::numeric_limits<double>::quiet_NaN();

        size_t a = 0;
        std::vector<size_t> index(n - 1);
        for (int i = 0; i < n; i++) {
            long long tmp = dx[i];
            if (i == 0) {
                if (tmp < 0) {
                    a = 0;
                } else if (static_cast<size_t>(tmp) >= input_arrays.size()) {
                    a = input_arrays.size() - 1;
                } else {
                    a = tmp;
                }
            } else {
                if (tmp < 0) {
                    index[i - 1] = 0;
                } else if (n == 2 && static_cast<size_t>(tmp) >= input_arrays[a].elementCount()) {
                    index[i - 1] = input_arrays[a].elementCount() - 1;
                } else if (n != 2 && static_cast<size_t>(tmp) >= input_arrays[a].dimension(i - 1)) {
                    index[i - 1] = input_arrays[a].dimension(i - 1) - 1;
                } else {
                    index[i - 1] = tmp;
                }
            }
        }
        size_t linearIndex;
        if (n == 2) {
            linearIndex = index[0];
        } else {
            linearIndex = input_arrays[a].toLinearIndex(index);
        }

        std::vector<double>& var_v = calcSingleton->var_v;
        for (size_t c = 0; c < var_v.size(); c++) {
            double v = std::numeric_limits<double>::quiet_NaN();
            if (c < input_arrays[a].componentCount())
                v = input_value(a, linearIndex, c);
            var_v[c] = v;
        }
        return 0.0;
    }

public:
    // input arrays
    std::vector<TAD::ArrayContainer> input_arrays;

    // constructor
    Calc(const std::vector<std::string>& expressions, size_t inputCount) :
        expressions(expressions),
        parsers(expressions.size()),
        added_vars(expressions.size()),
        uniform_distrib(0.0, 1.0),
        gaussian_distrib(0.0, 1.0),
        var_dim(maxDimensionCount),
        var_box(maxDimensionCount),
        var_boxdim(maxDimensionCount),
        var_i(maxDimensionCount),
        var_v(maxComponentCount),
        input_arrays(inputCount)
    {
        calcSingleton = this;
        for (size_t i = 0; i < parsers.size(); i++) {
            // standard functionality, mostly compatible with mucalc
            parsers[i].ClearConst();
            parsers[i].DefineConst("e", e);
            parsers[i].DefineConst("pi", pi);
            parsers[i].DefineOprt("%", mod, mu::prMUL_DIV, mu::oaLEFT, true);
            parsers[i].DefineFun("deg", deg);
            parsers[i].DefineFun("rad", rad);
            parsers[i].DefineFun("atan2", atan2);
            parsers[i].DefineFun("fract", fract);
            parsers[i].DefineFun("pow", pow);
            parsers[i].DefineFun("exp2", exp2);
            parsers[i].DefineFun("cbrt", cbrt);
            parsers[i].DefineFun("int", int_);
            parsers[i].DefineFun("ceil", ceil);
            parsers[i].DefineFun("floor", floor);
            parsers[i].DefineFun("round", round);
            parsers[i].DefineFun("trunc", trunc);
            parsers[i].DefineFun("med", med);
            parsers[i].DefineFun("clamp", clamp);
            parsers[i].DefineFun("step", step);
            parsers[i].DefineFun("smoothstep", smoothstep);
            parsers[i].DefineFun("mix", mix);
            parsers[i].DefineFun("seed", seed, false);
            parsers[i].DefineFun("random", random, false);
            parsers[i].DefineFun("gaussian", gaussian, false);
            parsers[i].DefineInfixOprt("+", unary_plus);
            parsers[i].SetVarFactory(add_var, &expressionIndex);
            // functions to get or copy input array values
            parsers[i].DefineFun("v", v);
            parsers[i].DefineFun("copy", copy);
            // input-dependent variables
            parsers[i].DefineVar("array_count", &var_array_count);
            parsers[i].DefineVar("stream_index", &var_stream_index);
            parsers[i].DefineVar("dimensions", &var_dimensions);
            for (size_t j = 0; j < maxDimensionCount; j++)
                parsers[i].DefineVar((std::string("dim") + std::to_string(j)).c_str(), &(var_dim[j]));
            parsers[i].DefineVar("components", &var_components);
            for (size_t j = 0; j < maxDimensionCount; j++)
                parsers[i].DefineVar((std::string("box") + std::to_string(j)).c_str(), &(var_box[j]));
            for (size_t j = 0; j < maxDimensionCount; j++)
                parsers[i].DefineVar((std::string("boxdim") + std::to_string(j)).c_str(), &(var_boxdim[j]));
            parsers[i].DefineVar("index", &var_index);
            for (size_t j = 0; j < maxDimensionCount; j++)
                parsers[i].DefineVar((std::string("i") + std::to_string(j)).c_str(), &(var_i[j]));
            for (size_t j = 0; j < maxComponentCount; j++)
                parsers[i].DefineVar((std::string("v") + std::to_string(j)).c_str(), &(var_v[j]));
            // the expression
            parsers[i].SetExpr(expressions[i]);
        }

        // initialize random number generator
        prng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    void init(size_t arrayIndex, const std::vector<size_t>& box)
    {
        var_array_count = input_arrays.size();
        var_stream_index = arrayIndex;
        var_dimensions = input_arrays[0].dimensionCount();
        for (size_t i = 0; i < maxDimensionCount; i++) {
            if (i < input_arrays[0].dimensionCount()) {
                var_dim[i] = input_arrays[0].dimension(i);
                var_box[i] = box[i];
                var_boxdim[i] = box[input_arrays[0].dimensionCount() + i];
            } else {
                var_dim[i] = std::numeric_limits<double>::quiet_NaN();
                var_box[i] = std::numeric_limits<double>::quiet_NaN();
                var_boxdim[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
        var_components = input_arrays[0].componentCount();
    }

    void setIndex(const std::vector<size_t>& index, size_t e)
    {
        var_index = e;
        for (size_t i = 0; i < maxDimensionCount; i++) {
            if (i < input_arrays[0].dimensionCount())
                var_i[i] = index[i];
            else
                var_i[i] = std::numeric_limits<double>::quiet_NaN();
        }
        for (size_t i = 0; i < maxComponentCount; i++) {
            if (i < input_arrays[0].componentCount())
                var_v[i] = input_value(0, e, i);
            else
                var_v[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }

    bool evaluate()
    {
        bool ok = true;
        for (size_t i = 0; i < parsers.size(); i++) {
            expressionIndex = i;
            try {
                parsers[i].Eval();
            }
            catch (mu::Parser::exception_type& e) {
                // Fix up the exception before reporting the error
                mu::string_type expr = e.GetExpr();
                mu::string_type token = e.GetToken();
                mu::EErrorCodes code = e.GetCode();
                size_t pos = e.GetPos();
                // Let positions start at 1 and fix position reported for EOF
                if (code != mu::ecUNEXPECTED_EOF)
                    pos++;
                if (pos == 0)
                    pos = 1;
                // Remove excess blank from token
                if (token.back() == ' ')
                    token.pop_back();
                mu::Parser::exception_type fixed_err(code, pos, token);
                // Report the fixed error
                fprintf(stderr, "tad calc: expression %zu: %s\n", i, fixed_err.GetMsg().c_str());
                fprintf(stderr, "tad calc: %s\n", expressions[i].c_str());
                fprintf(stderr, "tad calc: %s^\n", std::string(fixed_err.GetPos() - 1, ' ').c_str());
                ok = false;
                break;
            }
        }
        return ok;
    }

    void getElement(TAD::ArrayContainer& array, size_t e)
    {
        for (size_t i = 0; i < array.componentCount(); i++) {
            switch (array.componentType()) {
            case TAD::int8:
                array.set<int8_t>(e, i, var_v[i]);
                break;
            case TAD::uint8:
                array.set<uint8_t>(e, i, var_v[i]);
                break;
            case TAD::int16:
                array.set<int16_t>(e, i, var_v[i]);
                break;
            case TAD::uint16:
                array.set<uint16_t>(e, i, var_v[i]);
                break;
            case TAD::int32:
                array.set<int32_t>(e, i, var_v[i]);
                break;
            case TAD::uint32:
                array.set<uint32_t>(e, i, var_v[i]);
                break;
            case TAD::int64:
                array.set<int64_t>(e, i, var_v[i]);
                break;
            case TAD::uint64:
                array.set<uint64_t>(e, i, var_v[i]);
                break;
            case TAD::float32:
                array.set<float>(e, i, var_v[i]);
                break;
            case TAD::float64:
                array.set<double>(e, i, var_v[i]);
                break;
            }
        }
    }
};
#endif

int tad_calc(int argc, char* argv[])
{
    CmdLine cmdLine;
    cmdLine.addOptionWithArg("input", 'i');
    cmdLine.addOptionWithArg("output", 'o');
    cmdLine.addOptionWithArg("box", 'b', parseUIntList);
    cmdLine.addOptionWithArg("expression", 'e');
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 2, -1, errMsg)) {
        fprintf(stderr, "tad calc: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad calc [option]... <infile|-> [<infile|->...] <outfile|->\n"
                "\n"
                "Calculate array element components via mathematical expressions given with the\n"
                "option -e.\n"
                "\n"
                "The expression(s) are evaluated for all components of all array elements of the\n"
                "output array.\n"
                "Values are assigned to these components by setting the variables v0,v1,... .\n"
                "For each output array, the values of all corresponding input arrays are\n"
                "available and can be used in the calculations.\n"
                "The first input defines the output dimensions and components.\n"
                "\n"
                "Available constants:\n"
                "  pi, e\n"
                "Available functions:\n"
                "  deg, rad,\n"
                "  sin, asin, cos, acos, tan, atan, atan2,\n"
                "  sinh, asinh, cosh, acosh, tanh, atanh,\n"
                "  pow, exp, exp2, exp10, log, ln, log2, log10, sqrt, cbrt,\n"
                "  abs, sign, fract, int, ceil, floor, round, rint, trunc,\n"
                "  min, max, sum, avg, med,\n"
                "  clamp, step, smoothstep, mix\n"
                "  random, gaussian, seed\n"
                "Available operators:\n"
                "  ^, *, /, %%, +, -, ==, !=, <, >, <=, >=, ||, &&, ?:\n"
                "Available input information (in the form of variables):\n"
                "  array_count     - number of input arrays\n"
                "  stream_index    - index of the current array in the input stream\n"
                "  dimensions      - number of dimensions of the output array (e.g. 2 for 2D)\n"
                "  dim0, dim1, ... - dimensions (e.g. dim0=width, dim1=height for 2D)\n"
                "  components      - number of components of the output array (e.g. 3 for RGB)\n"
                "  box0, box1, ... - offset of the box (see option --box)\n"
                "  boxdim0, ...    - dimensions of the box (see option --box)\n"
                "  index           - linear index of the current output element\n"
                "                    (e.g. y * width + x for 2D)\n"
                "  i0, i1, ...     - index of the current output element (e.g. i0=x, i1=y for 2D)\n"
                "Function to access input arrays\n"
                "  (all arguments will be clamped to allowed range):\n"
                "  v(a, e, c)      - get the value of component c of element e in input array a\n"
                "  v(a, i0, ..., c)- get the value of component c of element (i0, i1, ...) in\n"
                "                    input array a\n"
                "Output variables:\n"
                "  v0, v1, ...     - output element components, e.g. v0=R, v1=G, v2=B for images\n"
                "Function to copy an element from an input array into the output variables\n"
                "  (all arguments will be clamped to allowed range):\n"
                "  copy(a, e)      - set variables v0,v1,... from element e in input array a\n"
                "  copy(a, i0, ...)- set variables v0,v1,... from element (i0, i1, ...) in input\n"
                "                    array a\n"
                "Example expressions:\n"
                "  convert BGR image data into RGB:\n"
                "    v0=v(0,index,2), v1=v(0,index,1), v2=v(0,index,0)\n"
                "  compute the difference of two images:\n"
                "    v0=v(0,index,0)-v(1,index,0), v1=v(0,index,1)-v(1,index,1),\n"
                "    v2=v(0,index,2)-v(1,index,2)\n"
                "  flip image vertically:\n"
                "    x=i0, y=dim1-1-i1, copy(0,x,y)\n"
                "  copy a small image into a larger image at position x=80, y=60:\n"
                "    --box=80,60,500,500  x=i0-box0, y=i1-box1, copy(1,x,y)\n"
                "  apply a Gaussian filter to an image:\n"
                "    v0 = (v(0,i0,i1-1,0) + v(0,i0-1,i1,0) + 2*v(0,i0,i1,0)\n"
                "        + v(0,i0+1,i1,0) + v(0,i0,i1+1,0)) / 6,\n"
                "    v1 = (v(0,i0,i1-1,1) + v(0,i0-1,i1,1) + 2*v(0,i0,i1,1)\n"
                "        + v(0,i0+1,i1,1) + v(0,i0,i1+1,1)) / 6,\n"
                "    v2 = (v(0,i0,i1-1,2) + v(0,i0-1,i1,2) + 2*v(0,i0,i1,2)\n"
                "        + v(0,i0+1,i1,2) + v(0,i0,i1+1,2)) / 6\n"
                "\n"
                "Options:\n"
                "  -i|--input=TAG             set input hints such as FORMAT=gdal, DPI=300 etc.\n"
                "  -o|--output=TAG            set output hints such as FORMAT=gdal etc.\n"
                "  -b|--box=INDEX,SIZE        set box to operate on, e.g. X,Y,WIDTH,HEIGHT for 2D\n"
                "  -e|--expression=E          evaluate expression E (can be used more than once)\n");
        return 0;
    }
    if (!cmdLine.isSet("expression")) {
        fprintf(stderr, "tad calc: missing --expression\n");
        return 1;
    }

#ifndef TAD_WITH_MUPARSER
    fprintf(stderr, "tad calc: command not available (libmuparser is missing)\n");
    return 1;
#else
    size_t inputCount = cmdLine.arguments().size() - 1;
    const std::vector<std::string>& inFileNames = cmdLine.arguments();
    const std::string& outFileName = cmdLine.arguments().back();
    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::TagList exporterHints = createTagList(cmdLine.valueList("output"));
    std::vector<TAD::Importer> importers(inputCount);
    for (size_t i = 0; i < inputCount; i++)
        importers[i].initialize(inFileNames[i], importerHints);
    TAD::Exporter exporter(outFileName, TAD::Overwrite, exporterHints);

    std::vector<size_t> box;
    if (cmdLine.isSet("box"))
        box = getUIntList(cmdLine.value("box"));

    Calc calc(cmdLine.valueList("expression"), inputCount);

    TAD::Error err = TAD::ErrorNone;

    size_t arrayIndex = 0;
    for (;;) {
        /* read inputs */
        if (!importers[0].hasMore(&err)) {
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad calc: %s: %s\n", inFileNames[0].c_str(), TAD::strerror(err));
            }
            break;
        }
        calc.input_arrays[0] = importers[0].readArray(&err);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad calc: %s: %s\n", inFileNames[0].c_str(), TAD::strerror(err));
            break;
        }
        if (calc.input_arrays[0].dimensionCount() > Calc::maxDimensionCount
                || calc.input_arrays[0].componentCount() > Calc::maxComponentCount) {
            fprintf(stderr, "tad calc: %s: too many dimensions or components\n", inFileNames[0].c_str());
            break;
        }
        for (size_t i = 1; i < importers.size(); i++) {
            if (!importers[i].hasMore(&err)) {
                // give up only on error, but not on EOF if at least one array from that input was read
                if (err != TAD::ErrorNone) {
                    fprintf(stderr, "tad calc: %s: %s\n", inFileNames[i].c_str(), TAD::strerror(err));
                } else if (arrayIndex == 0) {
                    fprintf(stderr, "tad calc: %s: missing array\n", inFileNames[i].c_str());
                    err = TAD::ErrorInvalidData;
                }
                break;
            }
            calc.input_arrays[i] = importers[i].readArray(&err);
            if (err != TAD::ErrorNone) {
                fprintf(stderr, "tad calc: %s: %s\n", inFileNames[i].c_str(), TAD::strerror(err));
                break;
            }
        }
        if (err != TAD::ErrorNone) {
            break;
        }

        /* set up output array */
        TAD::ArrayContainer array = calc.input_arrays[0].deepCopy();

        /* set up box to operate on */
        std::vector<size_t> index(array.dimensionCount());
        std::vector<size_t> localBox;
        if (box.size() > 0) {
            if (box.size() != index.size() * 2) {
                fprintf(stderr, "tad calc: %s: box does not match dimensions\n", inFileNames[0].c_str());
                err = TAD::ErrorInvalidData;
                break;
            }
            localBox = restrictBoxToArray(box, array);
        } else {
            localBox = getBoxFromArray(array);
        }

        /* setup calculator for this array */
        calc.init(arrayIndex, localBox);

        /* calc */
        if (!boxIsEmpty(localBox)) {
            /* initialize box index */
            initBoxIndex(localBox, index);
            for (;;) {
                /* get linear index */
                size_t e = array.toLinearIndex(index);
                /* give indices to calc */
                calc.setIndex(index, e);
                /* evaluate */
                if (!calc.evaluate()) {
                    err = TAD::ErrorInvalidData;
                    break;
                }
                /* read back the updated element */
                calc.getElement(array, e);
                /* increment index */
                if (!incBoxIndex(localBox, index))
                    break;
            }
        }
        if (err != TAD::ErrorNone) {
            break;
        }

        err = exporter.writeArray(array);
        if (err != TAD::ErrorNone) {
            fprintf(stderr, "tad calc: %s: %s\n", outFileName.c_str(), TAD::strerror(err));
            break;
        }
        arrayIndex++;
    }

    return (err == TAD::ErrorNone ? 0 : 1);
#endif
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
                "\n"
                "Compute the absolute difference.\n"
                "\n"
                "Options:\n"
                "  -i|--input=TAG             set input hints such as FORMAT=gdal, DPI=300 etc.\n"
                "  -o|--output=TAG            set output hints such as FORMAT=gdal etc.\n");
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
            err = TAD::ErrorInvalidData;
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
    cmdLine.addOrderedOptionWithoutArg("dimensions", 'D');
    cmdLine.addOrderedOptionWithArg("dimension", 'd', parseUInt);
    cmdLine.addOrderedOptionWithoutArg("components", 'c');
    cmdLine.addOrderedOptionWithoutArg("type", 't');
    cmdLine.addOrderedOptionWithArg("global-tag");
    cmdLine.addOrderedOptionWithoutArg("global-tags");
    cmdLine.addOrderedOptionWithArg("dimension-tag", 0, parseUIntAndName);
    cmdLine.addOrderedOptionWithArg("dimension-tags", 0, parseUInt);
    cmdLine.addOrderedOptionWithArg("component-tag", 0, parseUIntAndName);
    cmdLine.addOrderedOptionWithArg("component-tags", 0, parseUInt);
    std::string errMsg;
    if (!cmdLine.parse(argc, argv, 1, -1, errMsg)) {
        fprintf(stderr, "tad info: %s\n", errMsg.c_str());
        return 1;
    }
    if (cmdLine.isSet("help")) {
        fprintf(stderr, "Usage: tad info [option]... <infile|->...\n"
                "\n"
                "Print information. Default output consists of an overview, all tags,\n"
                "and optionally statistics (with -s) which are optionally restricted\n"
                "to a box of interest.\n"
                "\n"
                "Options:\n"
                "  -i|--input=TAG             set input hints such as FORMAT=gdal, DPI=300 etc\n"
                "  -s|--statistics            print statistics\n"
                "  -b|--box=INDEX,SIZE        set box to operate on, e.g. X,Y,WIDTH,HEIGHT for 2D\n"
                "\n"
                "The following options disable default output, and instead print their own\n"
                "output in the order in which they are given:\n"
                "  -D|--dimensions            print number of dimensions\n"
                "  -d|--dimension=D           print dimension D, e.g. -d 0 for width in 2D\n"
                "  -c|--components            print number of array element components\n"
                "  -t|--type                  print data type\n"
                "  --global-tag=NAME          print value of this global tag\n"
                "  --global-tags              print all global tags\n"
                "  --dimension-tag=D,N        print value of tag named N of dimension D\n"
                "  --dimension-tags=D         print all tags of dimension D\n"
                "  --component-tag=C,N        print value of tag named N of component C\n"
                "  --component-tags=C         print all tags of component C\n");
        return 0;
    }

    TAD::TagList importerHints = createTagList(cmdLine.valueList("input"));
    TAD::Error err = TAD::ErrorNone;
    size_t arrayCounter = 0;
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

    for (size_t arg = 0; arg < cmdLine.arguments().size(); arg++) {
        const std::string& inFileName = cmdLine.arguments()[arg];
        TAD::Importer importer(inFileName, importerHints);
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
            for (size_t o = 0; o < cmdLine.orderedOptionNames().size(); o++) {
                const std::string& optName = cmdLine.orderedOptionNames()[o];
                const std::string& optVal = cmdLine.orderedOptionValues()[o];
                if (optName == "dimensions") {
                    printf("%zu\n", array.dimensionCount());
                } else if (optName == "dimension") {
                    size_t dim = getUInt(optVal);
                    if (dim >= array.dimensionCount()) {
                        fprintf(stderr, "tad info: %s: no such dimension %zu\n", inFileName.c_str(), dim);
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    printf("%zu\n", array.dimension(dim));
                } else if (optName == "components") {
                    printf("%zu\n", array.componentCount());
                } else if (optName == "type") {
                    printf("%s\n", TAD::typeToString(getType(cmdLine.value("type"))));
                } else if (optName == "global-tag") {
                    if (!array.globalTagList().contains(optVal)) {
                        fprintf(stderr, "tad info: %s: no global tag %s\n", inFileName.c_str(), optVal.c_str());
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    printf("%s\n", array.globalTagList().value(optVal).c_str());
                } else if (optName == "global-tags") {
                    tad_info_print_taglist(array.globalTagList(), false);
                } else if (optName == "dimension-tag") {
                    size_t dim;
                    std::string name;
                    getUIntAndName(optVal, &dim, &name);
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
                } else if (optName == "dimension-tags") {
                    size_t dim = getUInt(optVal);
                    if (dim >= array.dimensionCount()) {
                        fprintf(stderr, "tad info: %s: no such dimension %zu\n", inFileName.c_str(), dim);
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    tad_info_print_taglist(array.dimensionTagList(dim), false);
                } else if (optName == "component-tag") {
                    size_t comp;
                    std::string name;
                    getUIntAndName(optVal, &comp, &name);
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
                } else if (optName == "component-tags") {
                    size_t comp = getUInt(optVal);
                    if (comp >= array.componentCount()) {
                        fprintf(stderr, "tad info: %s: no such component %zu\n", inFileName.c_str(), comp);
                        err = TAD::ErrorInvalidData;
                        break;
                    }
                    tad_info_print_taglist(array.componentTagList(comp), false);
                }
            }
            if (err != TAD::ErrorNone) {
                break;
            }
            if (defaultOutput) {
                std::string sizeString;
                if (array.dimensionCount() == 0) {
                    sizeString = "0";
                } else {
                    sizeString = std::to_string(array.dimension(0));
                    for (size_t i = 1; i < array.dimensionCount(); i++) {
                        sizeString += 'x';
                        sizeString += std::to_string(array.dimension(i));
                    }
                }
                printf("array %zu: %zu x %s, size %s (%s)\n",
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
                    std::vector<size_t> index(array.dimensionCount());
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
        if (err != TAD::ErrorNone)
            break;
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
    } else if (std::strcmp(argv[1], "create") == 0) {
        retval = tad_create(argc - 1, &(argv[1]));
    } else if (std::strcmp(argv[1], "convert") == 0) {
        retval = tad_convert(argc - 1, &(argv[1]));
    } else if (std::strcmp(argv[1], "calc") == 0) {
        retval = tad_calc(argc - 1, &(argv[1]));
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
