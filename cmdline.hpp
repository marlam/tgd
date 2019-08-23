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

#ifndef TAD_CMDLINE_H
#define TAD_CMDLINE_H

#include <vector>
#include <string>

class CmdLine {
private:
    class Option {
    public:
        std::string name;
        char shortName;
        bool requiresArgument;
        bool isSet;
        bool (*parseValue)(const std::string&);
        std::string value;
     
        Option(const std::string& n, char sn) :
            name(n), shortName(sn), requiresArgument(false),
            isSet(false), parseValue(nullptr), value(std::string())
        {
        }

        Option(const std::string& n, char sn,
                bool (*pV)(const std::string&),
                const std::string& v) :
            name(n), shortName(sn), requiresArgument(true),
            isSet(false), parseValue(pV), value(v)
        {
        }
    };

    std::vector<Option> _options;
    std::vector<std::string> _arguments;

    size_t optionIndex(const std::string& optionName) const;

public:
    CmdLine();

    // Add option that takes no arguments.
    void addOptionWithoutArg(const std::string& name, char shortName = 0);

    // Add option that requires an argument.
    void addOptionWithArg(const std::string& name, char shortName = 0,
            bool (*parseValue)(const std::string&) = nullptr,
            const std::string& defaultValue = std::string());

    // Parse command line. If maxArgs is -1 then the number of arguments is unlimited.
    bool parse(int argc, char* argv[], int minArgs, int maxArgs, std::string& errMsg);

    // Check if option is set.
    bool isSet(const std::string& optionName) const;

    // Get value of option.
    const std::string& value(const std::string& optionName) const;

    // Get list of arguments.
    const std::vector<std::string>& arguments() const;
};

#endif
