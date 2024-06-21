/**
 * @file lltextbase.h
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLTEXTVALIDATE_H
#define LL_LLTEXTVALIDATE_H

#include "llstring.h"
#include "llinitparam.h"
#include <boost/function.hpp>

namespace LLTextValidate
{
    class ValidatorImpl
    {
    public:
        ValidatorImpl() {}
        virtual ~ValidatorImpl() {}

        virtual bool validate(const std::string& str) = 0;
        virtual bool validate(const LLWString& str) = 0;

        bool setError(std::string name, LLSD values = LLSD()) { return mLastErrorName = name, mLastErrorValues = values, false; }
        bool resetError() { return mLastErrorName.clear(), mLastErrorValues.clear(), true; }
        const std::string& getLastErrorName() const { return mLastErrorName; }
        const LLSD& getLastErrorValues() const { return mLastErrorValues; }

        void setLastErrorShowTime();
        U32 getLastErrorShowTime() const { return mLastErrorShowTime; }

    protected:
        std::string mLastErrorName;
        LLSD mLastErrorValues;
        U32 mLastErrorShowTime { 0 };
    };

    class Validator
    {
    public:
        Validator() : mImpl(nullptr) {}
        Validator(ValidatorImpl& impl) : mImpl(&impl) {}
        Validator(const Validator& validator) : mImpl(validator.mImpl) {}
        Validator(const Validator* validator) : mImpl(validator->mImpl) {}

        bool validate(const std::string& str) const { return !mImpl || mImpl->validate(str); }
        bool validate(const LLWString& str) const { return !mImpl || mImpl->validate(str); }

        operator bool() const { return mImpl; }

        static const U32 SHOW_LAST_ERROR_TIMEOUT_SEC = 30;
        void showLastErrorUsingTimeout(U32 timeout = SHOW_LAST_ERROR_TIMEOUT_SEC);

    private:
        ValidatorImpl* mImpl;
    };

    // Available validators
    extern Validator validateFloat;
    extern Validator validateInt;
    extern Validator validatePositiveS32;
    extern Validator validateNonNegativeS32;
    extern Validator validateNonNegativeS32NoSpace;
    extern Validator validateAlphaNum;
    extern Validator validateAlphaNumSpace;
    extern Validator validateASCIIPrintableNoPipe;
    extern Validator validateASCIIPrintableNoSpace;
    extern Validator validateASCII;
    extern Validator validateASCIINoLeadingSpace;
    extern Validator validateASCIIWithNewLine;

    // Add available validators to the internal map
    struct Validators : public LLInitParam::TypeValuesHelper<Validator, Validators>
    {
        static void declareValues();
    };
};

#endif
