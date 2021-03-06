/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 * Copyright (C) 2012-2019 by UCSC Computational Genomics Lab
 *
 * Released under the MIT license, see LICENSE.txt
 */

#ifndef _HALCLPARSER_H
#define _HALCLPARSER_H

#include "halAlignmentInstance.h"
#include "halDefs.h"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace hal {

    /**
     * Very basic command line parser. Modeled after Python's. Supports
     * options. Ex: --threshold 10.0
     * flags. Ex: --overwrite
     * positional arugments. Ex: outfile.txt 100
     * in any combination.
     *
     * This adds a sect of standard options for HAL access.
     *
     * The mode option controls what standard options to add to the parser, it doesn't
     * actually specified the access mode.
     */
    class CLParser {
      public:
        CLParser(unsigned mode = READ_ACCESS);

        /** Destructor */
        ~CLParser() {
        }

        /** Set the prefix string for an optional argument [Default = "--"]
         * @param prefix The prefix string */
        void setOptionPrefix(const std::string &prefix);

        /** Add an optional command line argument to the parser
         * @param name Name of option: will take form of <prefix><name> <value>
         * @param description Description of the option
         * @param defaultValue Default Value */
        template <typename T> void addOption(const std::string &name, const std::string &descritpion, const T &defaultValue);

        /** Get the value of an option in the parsed arguments
         * @param name Name of option */
        template <typename T> T getOption(const std::string &name) const;

        /* Get an option value or default, checking for obsoleteName as an alternative.  Warn if
         * obsolete name is used */
        template <typename T> T getOptionAlt(const std::string &name, const std::string &obsoleteName) const;

        /** Check if option exists in the parser
         * @param name Name of option to check */
        bool hasOption(const std::string &name) const;

        /** Check if option was specified on the command line.
         * @param name Name of option to check */
        bool specifiedOption(const std::string &name) const;

        /** Add a positional (mandatory) command line argument
         * The argument is given as a value on the command line
         * ie with no name or prefix or anything.
         * There is no default value because it is mandaatory
         * @param name Name of the argument
         * @description description string */
        void addArgument(const std::string &name, const std::string &description);

        /** Get the value of a positional argument
         * @param name Name of argument to get */
        template <typename T> T getArgument(const std::string &name) const;

        /** Check if argument exists in the parser
         * @param name Name of option to check */
        bool hasArgument(const std::string &name) const;

        /** Add an optional command line flag to the parser (ie no value taken)
        * @param name Name of option: will take form of <prefix><name>
        * @param description Description of the option
        * @param defaultValue The value if the flag is not specifed.
        * if the flag is provided than !defaultValue is set. */
        void addOptionFlag(const std::string &name, const std::string &description, bool defaultValue);

        /** Get the value of an option flag in the parsed arguments
         * @param name Name of option */
        bool getFlag(const std::string &name) const;

        /** Check if option flag exists in the parser
         * @param name Name of option to check */
        bool hasFlag(const std::string &name) const;

        /** Check if option flag was specified on the command line
         * @param name Name of option to check */
        bool specifiedFlag(const std::string &name) const;

        /* Get a flag, checking for obsoleteName as an alternative.  Warn if
         * obsolete name is used */
        bool getFlagAlt(const std::string &name, const std::string &obsoleteName) const;

        /** Get value of option or flag or argument by name */
        template <typename T> T get(const std::string &name) const;

        /** Set global description of tool */
        void setDescription(const std::string &description);

        /** Set an example */
        void setExample(const std::string &example);

        /** Parse the command line arguments, throwing exception on
         * failure. */
        void parseOptions(int argc, char **argv);

        /** Print the help screen to output stream */
        void printUsage(std::ostream &os) const;

      private:
        template <typename T> T convertFromString(const std::string &token) const;

        static size_t lineWidth;
        static std::string multiLine(const std::string &line, size_t indent);

        struct Option {
            std::string _description;
            std::string _defaultValue;
            std::string _value;
            bool _flag;
            bool _specified;
        };

        struct Argument {
            std::string _name;
            std::string _description;
            std::string _value;
            bool _specified;
        };

        std::string _prefix;
        std::string _exeName;
        std::string _description;
        std::string _example;
        std::vector<Argument> _args;
        std::map<std::string, Option> _options;
        size_t _maxArgLen;
        size_t _maxOptLen;
    };

    template <typename T> inline T CLParser::convertFromString(const std::string &token) const {
        std::stringstream ss(token);
        T value;
        try {
            ss << token;
            ss >> value;
        } catch (...) {
            throw hal_exception("type conversion error parsing value: " + token);
        }
        return value;
    }

    // above function won't work on strings with spaces so we specialize!
    template <> inline std::string CLParser::convertFromString<std::string>(const std::string &token) const {
        return token;
    }

    template <> inline const std::string &CLParser::convertFromString<const std::string &>(const std::string &token) const {
        return token;
    }

    template <typename T>
    inline void CLParser::addOption(const std::string &name, const std::string &descritpion, const T &defaultValue) {
        if (hasOption(name) || hasFlag(name) || hasArgument(name)) {
            throw hal_exception(std::string("name ") + name + "already present");
        }
        std::stringstream ss;
        Option opt;
        try {
            ss << defaultValue;
            opt._description = descritpion;
            opt._defaultValue = ss.str();
            opt._value = opt._defaultValue;
            opt._flag = false;
            opt._specified = false;
        } catch (...) {
            throw hal_exception(std::string("type conversion adding option ") + name);
        }

        _options.insert(std::pair<std::string, Option>(name, opt));
        _maxOptLen = std::max(_maxOptLen, name.length());
    }

    template <typename T> inline T CLParser::getOption(const std::string &name) const {
        std::map<std::string, Option>::const_iterator i = _options.find(name);
        if (i == _options.end() || i->second._flag == true) {
            throw hal_exception(std::string("Option ") + name + " not recognized");
        }
        return convertFromString<T>(i->second._value);
    }

    template <typename T> inline T CLParser::getArgument(const std::string &name) const {
        for (size_t i = 0; i < _args.size(); ++i) {
            if (_args[i]._name == name) {
                return convertFromString<T>(_args[i]._value);
            }
        }
        throw hal_exception(std::string("Argument ") + name + " not recognized");
    }

    template <typename T> inline T CLParser::getOptionAlt(const std::string &name, const std::string &obsoleteName) const {
        if (specifiedOption(obsoleteName)) {
            std::cerr << "Warning: --" << obsoleteName << " is obsolete, use --" << name << std::endl;
            return getOption<T>(obsoleteName);
        } else {
            return getOption<T>(name);
        }
    }

    inline bool CLParser::getFlagAlt(const std::string &name, const std::string &obsoleteName) const {
        if (specifiedFlag(obsoleteName)) {
            std::cerr << "Warning: --" << obsoleteName << " is obsolete, use --" << name << std::endl;
            return getFlag(obsoleteName);
        } else {
            return getFlag(name);
        }
    }

    template <typename T> inline T CLParser::get(const std::string &name) const {
        if (hasArgument(name)) {
            return getArgument<T>(name);
        } else if (hasOption(name)) {
            return getOption<T>(name);
        } else if (hasFlag(name)) {
            return (T)getFlag(name);
        } else {
            throw hal_exception(std::string("Name ") + name + " not recognized");
        }
    }
}

#endif
// Local Variables:
// mode: c++
// End:
