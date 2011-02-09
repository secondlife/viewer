 once



#include <atlbase.h>



/* static*/ class RuntimeDiagnostics

{

public:

    static void CheckPauseOnStartupOption(

        )

    {

        DWORD dwPause;

        if (GetExecutionOption(TEXT("PauseOnStartup"), &dwPause) == S_OK && dwPause != 0)

        {

            while (true)

            {

                if (IsDebuggerPresent())

                {

                    __debugbreak();

                    break;

                }



                Sleep(1000);

            }

        }

    }



private:

    static HRESULT GetExecutionOption(

        LPCTSTR szOptionName,

        DWORD *pValue

        )

    {

        HRESULT hr;

        DWORD nChar;



        *pValue = 0;



        CRegKey rkeyExecutionOptions;

        hr = WIN32_ERROR_CALL( rkeyExecutionOptions.Open(

            HKEY_LOCAL_MACHINE,

            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options"),

            KEY_READ

            ) );



        if (hr == S_OK)

        {

            TCHAR szModPath[MAX_PATH + 1];

            nChar = GetModuleFileName(NULL, szModPath, ARRAYSIZE(szModPath));

            hr = WIN32_BOOL_CALL(nChar > 0 && nChar < ARRAYSIZE(szModPath));



            if (hr == S_OK)

            {

                nChar = GetFileNameCharOffset(szModPath);



                if (nChar != MAXDWORD)

                {

                    CRegKey rkeyExe;

                    hr = WIN32_ERROR_CALL( rkeyExe.Open(rkeyExecutionOptions, szModPath + nChar, KEY_QUERY_VALUE) );

                    if (hr == S_OK)

                    {

                        hr = WIN32_ERROR_CALL( rkeyExe.QueryDWORDValue(szOptionName, *pValue) );

                    }

                }

                else

                {

                    hr = E_FAIL;

                }

            }

        }



        return hr;

    }



    static DWORD GetFileNameCharOffset(LPCTSTR szPath)

    {

        if (!szPath)

        {

            return MAXDWORD;

        }



        // Find the last slash

        DWORD dwReturn = 0;

        for (DWORD c = 0; szPath[c] != 0; c++)

        {

            if (c >= INT_MAX)

            {

                return MAXDWORD; // string did not terminate

            }



            if (szPath[c] == '\\' || szPath[c] == '/' || (szPath[c] == ':' && c == 1))

            {

                dwReturn = c + 1;

            }

        }



        // Make sure that the string doesn't end at the slash

        if (szPath[dwReturn] == 0)

        {

            dwReturn = MAXDWORD;

        }



        return dwReturn;

    }



    static inline HRESULT WIN32_ERROR(LONG lError)

    {

        return HRESULT_FROM_WIN32(lError);

    }



    static inline HRESULT WIN32_LAST_ERROR()

    {

        return WIN32_ERROR(GetLastError());

    }



    static inline HRESULT WIN32_BOOL_CALL(BOOL rc)

    {

        if (!rc)

            return WIN32_LAST_ERROR();

        return S_OK;

    }



    static inline HRESULT WIN32_ERROR_CALL(LONG lError)

    {

        if (lError != ERROR_SUCCESS)

            return WIN32_ERROR(lError);

        return S_OK;

    }

};

