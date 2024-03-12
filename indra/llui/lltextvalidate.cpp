/** 
 * @file lltextvalidate.cpp
 * @brief Text validation helper functions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"

#include "lltextvalidate.h"

#include "llnotificationsutil.h"

#include "llresmgr.h" // for LLLocale

namespace LLTextValidate
{

static S32 strtol(const std::string& str) { return ::strtol(str.c_str(), NULL, 10); }
static S32 strtol(const LLWString& str) { return ::strtol(wstring_to_utf8str(str).c_str(), NULL, 10); }

static LLSD llsd(const std::string& str) { return LLSD(str); }
static LLSD llsd(const LLWString& str) { return LLSD(wstring_to_utf8str(str)); }
template <class CHAR>
LLSD llsd(CHAR ch) { return llsd(std::basic_string<CHAR>(1, ch)); }

void ValidatorImpl::setLastErrorShowTime()
{
    mLastErrorShowTime = (U32Seconds)LLTimer::getTotalTime();
}

void Validator::showLastErrorUsingTimeout(U32 timeout)
{
    if (mImpl && (U32Seconds)LLTimer::getTotalTime() >= mImpl->getLastErrorShowTime() + timeout)
    {
        mImpl->setLastErrorShowTime();
        LLNotificationsUtil::add(mImpl->getLastErrorName(), mImpl->getLastErrorValues());
    }
}

// Limits what characters can be used to [1234567890.-] with [-] only valid in the first position.
// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
// the simple reasons that intermediate states may be invalid even if the final result is valid.
class FloatValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR> &str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        std::basic_string<CHAR> trimmed = str;
        LLStringUtilBase<CHAR>::trim(trimmed);
        S32 len = trimmed.length();
        if (0 < len)
        {
            // May be a comma or period, depending on the locale
            CHAR decimal_point = LLResMgr::getInstance()->getDecimalPoint();

            S32 i = 0;

            // First character can be a negative sign
            if ('-' == trimmed.front())
            {
                i++;
            }

            for (; i < len; i++)
            {
                CHAR ch = trimmed[i];
                if ((decimal_point != ch) && !LLStringOps::isDigit(ch))
                {
                    return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", i + 1).with("CH", llsd(ch)));
                }
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateFloatImpl;
Validator validateFloat(validateFloatImpl);

// Limits what characters can be used to [1234567890-] with [-] only valid in the first position.
// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
// the simple reasons that intermediate states may be invalid even if the final result is valid.
class IntValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR> &str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        std::basic_string<CHAR> trimmed = str;
        LLStringUtilBase<CHAR>::trim(trimmed);
        S32 len = trimmed.length();
        if (0 < len)
        {
            S32 i = 0;

            // First character can be a negative sign
            if ('-' == trimmed.front())
            {
                i++;
            }

            for (; i < len; i++)
            {
                CHAR ch = trimmed[i];
                if (!LLStringOps::isDigit(ch))
                {
                    return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", i + 1).with("CH", llsd(ch)));
                }
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateIntImpl;
Validator validateInt(validateIntImpl);

class PositiveS32Validator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        std::basic_string<CHAR> trimmed = str;
        LLStringUtilBase<CHAR>::trim(trimmed);
        S32 len = trimmed.length();
        if (0 < len)
        {
            CHAR ch = trimmed.front();

            if (('-' == ch) || ('0' == ch))
            {
                return setError("ValidatorInvalidInitialCharacter", LLSD().with("CH", llsd(ch)));
            }

            for (S32 i = 0; i < len; ++i)
            {
                ch = trimmed[i];
                if (!LLStringOps::isDigit(ch))
                {
                    return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", i + 1).with("CH", llsd(ch)));
                }
            }
        }

        S32 val = strtol(trimmed);
        if (val <= 0)
        {
            return setError("ValidatorInvalidNumberString", LLSD().with("STR", llsd(trimmed)));
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validatePositiveS32Impl;
Validator validatePositiveS32(validatePositiveS32Impl);

class NonNegativeS32Validator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        std::basic_string<CHAR> trimmed = str;
        LLStringUtilBase<CHAR>::trim(trimmed);
        S32 len = trimmed.length();
        if (0 < len)
        {
            CHAR ch = trimmed.front();

            if ('-' == ch)
            {
                return setError("ValidatorInvalidInitialCharacter", LLSD().with("CH", llsd(ch)));
            }

            for (S32 i = 0; i < len; ++i)
            {
                ch = trimmed[i];
                if (!LLStringOps::isDigit(ch))
                {
                    return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", i + 1).with("CH", llsd(ch)));
                }
            }
        }

        S32 val = strtol(trimmed);
        if (val < 0)
        {
            return setError("ValidatorInvalidNumberString", LLSD().with("STR", llsd(trimmed)));
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateNonNegativeS32Impl;
Validator validateNonNegativeS32(validateNonNegativeS32Impl);

class NonNegativeS32NoSpaceValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        std::basic_string<CHAR> test_str = str;
        S32 len = test_str.length();
        if (0 < len)
        {
            CHAR ch = test_str.front();

            if ('-' == ch)
            {
                return setError("ValidatorInvalidInitialCharacter", LLSD().with("CH", llsd(ch)));
            }

            for (S32 i = 0; i < len; ++i)
            {
                ch = test_str[i];
                if (!LLStringOps::isDigit(ch) || LLStringOps::isSpace(ch))
                {
                    return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", i + 1).with("CH", llsd(ch)));
                }
            }
        }

        S32 val = strtol(test_str);
        if (val < 0)
        {
            return setError("ValidatorInvalidNumberString", LLSD().with("STR", llsd(test_str)));
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateNonNegativeS32NoSpaceImpl;
Validator validateNonNegativeS32NoSpace(validateNonNegativeS32NoSpaceImpl);

class AlphaNumValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if (!LLStringOps::isAlnum(ch))
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateAlphaNumImpl;
Validator validateAlphaNum(validateAlphaNumImpl);

class AlphaNumSpaceValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        LLLocale locale(LLLocale::USER_LOCALE);

        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if (!(LLStringOps::isAlnum(ch) || (' ' == ch)))
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateAlphaNumSpaceImpl;
Validator validateAlphaNumSpace(validateAlphaNumSpaceImpl);

// Used for most names of things stored on the server, due to old file-formats
// that used the pipe (|) for multiline text storage.  Examples include
// inventory item names, parcel names, object names, etc.
class ASCIIPrintableNoPipeValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if (ch < 0x20 || ch > 0x7f || ch == '|' ||
                (ch != ' ' && !LLStringOps::isAlnum(ch) && !LLStringOps::isPunct(ch)))
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateASCIIPrintableNoPipeImpl;
Validator validateASCIIPrintableNoPipe(validateASCIIPrintableNoPipeImpl);

// Used for avatar names
class ASCIIPrintableNoSpaceValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if (ch < 0x20 || ch > 0x7f || LLStringOps::isSpace(ch) ||
                (!LLStringOps::isAlnum(ch) && !LLStringOps::isPunct(ch)))
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateASCIIPrintableNoSpaceImpl;
Validator validateASCIIPrintableNoSpace(validateASCIIPrintableNoSpaceImpl);

class ASCIIValidator : public ValidatorImpl
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if (ch < 0x20 || ch > 0x7f)
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateASCIIImpl;
Validator validateASCII(validateASCIIImpl);

class ASCIINoLeadingSpaceValidator : public ASCIIValidator
{
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        if (LLStringOps::isSpace(str.front()))
        {
            return false;
        }

        return ASCIIValidator::validate(str);
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateASCIINoLeadingSpaceImpl;
Validator validateASCIINoLeadingSpace(validateASCIINoLeadingSpaceImpl);

class ASCIIWithNewLineValidator : public ValidatorImpl
{
    // Used for multiline text stored on the server.
    // Example is landmark description in Places SP.
    template <class CHAR>
    bool validate(const std::basic_string<CHAR>& str)
    {
        S32 len = str.length();
        while (len--)
        {
            CHAR ch = str[len];

            if ((ch < 0x20 && ch != 0xA) || ch > 0x7f)
            {
                return setError("ValidatorInvalidNthCharacter", LLSD().with("NR", len + 1).with("CH", llsd(ch)));
            }
        }

        return resetError();
    }

public:
    /*virtual*/ bool validate(const std::string& str) override { return validate<char>(str); }
    /*virtual*/ bool validate(const LLWString& str) override { return validate<llwchar>(str); }
} validateASCIIWithNewLineImpl;
Validator validateASCIIWithNewLine(validateASCIIWithNewLineImpl);

void Validators::declareValues()
{
    declare("ascii", validateASCII);
    declare("float", validateFloat);
    declare("int", validateInt);
    declare("positive_s32", validatePositiveS32);
    declare("non_negative_s32", validateNonNegativeS32);
    declare("alpha_num", validateAlphaNum);
    declare("alpha_num_space", validateAlphaNumSpace);
    declare("ascii_printable_no_pipe", validateASCIIPrintableNoPipe);
    declare("ascii_printable_no_space", validateASCIIPrintableNoSpace);
    declare("ascii_with_newline", validateASCIIWithNewLine);
}

} // namespace LLTextValidate
