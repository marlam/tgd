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
#include <getopt.h>

#include "cmdline.hpp"


CmdLine::CmdLine()
{
    addOptionWithoutArg("help");
}

size_t CmdLine::optionIndex(const std::string& optionName) const
{
    size_t i;
    for (i = 0; i < _options.size(); i++)
        if (optionName == _options[i].name)
            break;
    assert(i < _options.size());
    return i;
}

void CmdLine::addOptionWithoutArg(const std::string& name, char shortName)
{
    _options.push_back(Option(name, shortName));
}

void CmdLine::addOptionWithArg(const std::string& name, char shortName,
        bool (*parseValue)(const std::string&),
        const std::string& defaultValue)
{
    _options.push_back(Option(name, shortName, parseValue, defaultValue));
}

void CmdLine::addOrderedOptionWithoutArg(const std::string& name, char shortName)
{
    _options.push_back(Option(name, shortName, true));
}

void CmdLine::addOrderedOptionWithArg(const std::string& name, char shortName,
        bool (*parseValue)(const std::string&),
        const std::string& defaultValue)
{
    _options.push_back(Option(name, shortName, parseValue, defaultValue, true));
}

bool CmdLine::parse(int argc, char* argv[], int minArgs, int maxArgs, std::string& errMsg)
{
    const int optValBase = 1024;
    std::vector<struct ::option> getoptStructs(_options.size() + 1);
    std::string shortOptString(":");
    for (size_t i = 0; i < _options.size(); i++) {
        getoptStructs[i].name = _options[i].name.c_str();
        getoptStructs[i].has_arg = _options[i].requiresArgument ? 1 : 0;
        getoptStructs[i].flag = 0;
        getoptStructs[i].val = optValBase + i;
        if (_options[i].shortName) {
            shortOptString += _options[i].shortName;
            if (_options[i].requiresArgument)
                shortOptString += ':';
        }
    }
    getoptStructs.back().name = 0;
    getoptStructs.back().has_arg = 0;
    getoptStructs.back().flag = 0;
    getoptStructs.back().val = 0;

    bool error = false;
    opterr = 0;
    optind = 1;
    while (!error) {
        int optVal = getopt_long(argc, argv, shortOptString.c_str(), getoptStructs.data(), nullptr);
        if (optVal == -1) {
            break;
        } else if (optVal == '?') {
            if (optopt == 0) {
                errMsg = std::string("invalid option ") + argv[optind - 1];
            } else if (optopt >= optValBase) {
                errMsg = std::string("option --") + _options[optopt - optValBase].name
                    + std::string(" does not take an argument");
            } else {
                errMsg = std::string("invalid option -") + std::string(size_t(1), char(optopt));
            }
            error = true;
            break;
        } else if (optVal == ':') {
            assert(optopt >= optValBase);
            errMsg = std::string("option --") + _options[optopt - optValBase].name
                + std::string(" requires an argument");
            error = true;
            break;
        } else if (optVal < optValBase) {
            // short option form; find the true index
            for (size_t i = 0; i < _options.size(); i++)
                if (optVal == _options[i].shortName)
                    optVal = i;
        } else {
            optVal -= optValBase;
        }
        _options[optVal].isSet = true;
        if (_options[optVal].requiresArgument) {
            _options[optVal].valueList.push_back(std::string(optarg));
            if (_options[optVal].parseValue && !_options[optVal].parseValue(optarg)) {
                errMsg = std::string("invalid argument for --") + _options[optVal].name;
                error = true;
            }
        }
        if (_options[optVal].orderMatters) {
            _orderedOptionNames.push_back(_options[optVal].name);
            _orderedOptionValues.push_back(
                      _options[optVal].requiresArgument
                    ? _options[optVal].valueList.back()
                    : std::string());
        }
    }
    if (!error && !isSet("help")) {
        int args = argc - optind;
        if (args < minArgs) {
            errMsg = std::string("too few arguments");
            error = true;
        } else if (maxArgs >= 0 && args > maxArgs) {
            errMsg = std::string("too many arguments");
            error = true;
        } else {
            for (int i = 0; i < args; i++)
                _arguments.push_back(argv[optind + i]);
        }
    }
    return !error;
}

bool CmdLine::isSet(const std::string& optionName) const
{
    return _options[optionIndex(optionName)].isSet;
}

const std::string& CmdLine::value(const std::string& optionName) const
{
    static std::string emptyValue;
    const std::vector<std::string>& vl = valueList(optionName);
    if (vl.size() == 0)
        return emptyValue;
    else
        return vl.back();
}

const std::vector<std::string>& CmdLine::valueList(const std::string& optionName) const
{
    return _options[optionIndex(optionName)].valueList;
}

const std::vector<std::string>& CmdLine::arguments() const
{
    return _arguments;
}

const std::vector<std::string>& CmdLine::orderedOptionNames() const
{
    return _orderedOptionNames;
}

const std::vector<std::string>& CmdLine::orderedOptionValues() const
{
    return _orderedOptionValues;
}
