/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <idlc.hxx>
#include <rtl/alloc.h>
#include <rtl/ustring.hxx>
#include <rtl/strbuf.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <osl/process.h>
#include <osl/diagnose.h>
#include <osl/thread.h>
#include <osl/file.hxx>

#if defined(_WIN32)
#include <io.h>
#endif

#ifdef  SAL_UNX
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include <string.h>

using namespace ::osl;

extern int yyparse();
extern FILE* yyin;
extern int yydebug;

static char tmpFilePattern[512];

bool isFileUrl(std::string_view fileName)
{
    return o3tl::starts_with(fileName, "file://");
}

OString convertToAbsoluteSystemPath(const OString& fileName)
{
    OUString uSysFileName;
    OUString uFileName(fileName.getStr(), fileName.getLength(), osl_getThreadTextEncoding());
    if ( isFileUrl(fileName) )
    {
        if (FileBase::getSystemPathFromFileURL(uFileName, uSysFileName)
            != FileBase::E_None)
        {
            OSL_ASSERT(false);
        }
    } else
    {
        OUString uWorkingDir, uUrlFileName, uTmp;
        if (osl_getProcessWorkingDir(&uWorkingDir.pData) != osl_Process_E_None)
        {
            OSL_ASSERT(false);
        }
        if (FileBase::getFileURLFromSystemPath(uFileName, uTmp)
            != FileBase::E_None)
        {
            OSL_ASSERT(false);
        }
        if (FileBase::getAbsoluteFileURL(uWorkingDir, uTmp, uUrlFileName)
            != FileBase::E_None)
        {
            OSL_ASSERT(false);
        }
        if (FileBase::getSystemPathFromFileURL(uUrlFileName, uSysFileName)
            != FileBase::E_None)
        {
            OSL_ASSERT(false);
        }
    }

    return OUStringToOString(uSysFileName, osl_getThreadTextEncoding());
}

OString convertToFileUrl(const OString& fileName)
{
    if ( !isFileUrl(fileName) )
    {
        OString tmp = convertToAbsoluteSystemPath(fileName);
        OUString uFileName(tmp.getStr(), tmp.getLength(), osl_getThreadTextEncoding());
        OUString uUrlFileName;
        if (FileBase::getFileURLFromSystemPath(uFileName, uUrlFileName)
            != FileBase::E_None)
        {
            OSL_ASSERT(false);
        }
        return OUStringToOString(uUrlFileName, osl_getThreadTextEncoding());
    }

    return fileName;
}

static OString makeTempName(const OString& prefix)
{
    OUString uTmpPath;
    OString tmpPath;

    if ( osl_getEnvironment(OUString("TMP").pData, &uTmpPath.pData) != osl_Process_E_None )
    {
        if ( osl_getEnvironment(OUString("TEMP").pData, &uTmpPath.pData) != osl_Process_E_None )
        {
#if defined(_WIN32)
            tmpPath = OString("c:\\temp");
#else
            tmpPath = OString("/tmp");
#endif
        }
    }

    if ( !uTmpPath.isEmpty() )
        tmpPath = OUStringToOString(uTmpPath, RTL_TEXTENCODING_UTF8);

#if defined(_WIN32) || defined(SAL_UNX)

    OSL_ASSERT( sizeof(tmpFilePattern) >
                o3tl::make_unsigned( tmpPath.getLength()
                           + RTL_CONSTASCII_LENGTH( PATH_SEPARATOR )
                           + prefix.getLength()
                           + RTL_CONSTASCII_LENGTH( "XXXXXX") ) );

    tmpFilePattern[ sizeof(tmpFilePattern)-1 ] = '\0';
    strncpy(tmpFilePattern, tmpPath.getStr(), sizeof(tmpFilePattern)-1);
    strncat(tmpFilePattern, PATH_SEPARATOR, sizeof(tmpFilePattern)-1-strlen(tmpFilePattern));
    strncat(tmpFilePattern, prefix.getStr(), sizeof(tmpFilePattern)-1-strlen(tmpFilePattern));
    strncat(tmpFilePattern, "XXXXXX", sizeof(tmpFilePattern)-1-strlen(tmpFilePattern));

#ifdef SAL_UNX
    // coverity[secure_temp] - https://communities.coverity.com/thread/3179
    int nDescriptor = mkstemp(tmpFilePattern);
    if( -1 == nDescriptor )
    {
        fprintf(stderr, "idlc: mkstemp(\"%s\") failed: %s\n", tmpFilePattern, strerror(errno));
        exit( 1 );
    }
    // the file shall later be reopened by stdio functions
    close( nDescriptor );
#else
    (void) mktemp(tmpFilePattern);
#endif
#endif

    return tmpFilePattern;
}

bool copyFile(const OString* source, const OString& target)
{
    bool bRet = true;

    FILE* pSource = source == nullptr ? stdin : fopen(source->getStr(), "rb");
    if ( !pSource )
        return false;

    FILE* pTarget = fopen(target.getStr(), "wb");
    if ( !pTarget )
    {
        fclose(pSource);
        return false;
    }

    size_t const totalSize = 512;
    char   pBuffer[totalSize + 1];

    while ( !feof(pSource) )
    {
        size_t readSize = fread(pBuffer, 1, totalSize, pSource);
        if ( readSize > 0 && !ferror(pSource) )
        {
            if ( (fwrite(pBuffer, 1, readSize, pTarget)) != readSize || ferror(pTarget) )
            {
                if (source != nullptr) {
                    fclose(pSource);
                }
                fclose(pTarget);
                return false;
            }
        }
    }

    if (source != nullptr) {
        fclose(pSource);
    }
    if ( fclose(pTarget) )
        bRet = false;

    return bRet;
}

sal_Int32 compileFile(const OString * pathname)
{
    // preprocess input file
    OString tmpFile = makeTempName("idli_");
    OString preprocFile = makeTempName("idlf_");

    OString fileName;
    if (pathname == nullptr) {
        fileName = "stdin";
    } else {
        fileName = *pathname;
    }

    if ( !copyFile(pathname, tmpFile) )
    {
          fprintf(stderr, "%s: could not copy %s%s to %s\n",
                idlc()->getOptions()->getProgramName().getStr(),
                pathname == nullptr ? "" : "file ", fileName.getStr(),
                tmpFile.getStr());
          exit(99);
    }

    idlc()->setFileName(fileName);
    idlc()->setMainFileName(fileName);
    idlc()->setRealFileName(tmpFile);

    ::std::vector< OUString> lCppArgs;
    lCppArgs.emplace_back("-DIDL");
    lCppArgs.emplace_back("-C");
#ifdef SYSTEM_UCPP_IS_GCC
    // -nostdinc Do not search the standard system directories for header files
    lCppArgs.emplace_back("-nostdinc");
    // with gcc cpp, even when not explicitly including anything, /usr/include/stdc-predef.h
    // gets inserted without -nostdinc
#else
    // -zI Do not use the standard (compile-time) include path.
    lCppArgs.emplace_back("-zI");
#endif

    Options* pOptions = idlc()->getOptions();

    OString filePath;
    sal_Int32 index = fileName.lastIndexOf(SEPARATOR);

    if ( index > 0)
    {
        filePath = fileName.copy(0, index);

        if ( !filePath.isEmpty() )
        {
            OString cppArgs = "-I" + filePath;
            lCppArgs.push_back(OStringToOUString(
                cppArgs.replace('\\', '/'),
                RTL_TEXTENCODING_UTF8));
        }
    }

    if ( pOptions->isValid("-D") )
    {
        OString dOpt = pOptions->getOption("-D");
        sal_Int32 nIndex = 0;
        do
        {
            std::string_view token = o3tl::getToken(dOpt, 0, ' ', nIndex );
            if (token.size())
                lCppArgs.push_back("-D" + OStringToOUString(token, RTL_TEXTENCODING_UTF8));
        } while( nIndex != -1 );
    }

    if ( pOptions->isValid("-I") )
    {
        OString incOpt = pOptions->getOption("-I");
        sal_Int32 nIndex = 0;
        do
        {
            std::string_view token = o3tl::getToken(incOpt, 0, ' ', nIndex );
            if (token.size())
                lCppArgs.push_back("-I" + OStringToOUString(token, RTL_TEXTENCODING_UTF8));
        } while( nIndex != -1 );
    }

    lCppArgs.emplace_back("-o");

    lCppArgs.push_back(OStringToOUString(preprocFile, RTL_TEXTENCODING_UTF8));

    lCppArgs.push_back(OStringToOUString(tmpFile, RTL_TEXTENCODING_UTF8));

    OUString cpp;
    OUString startDir;
#ifndef SYSTEM_UCPP
    if (osl_getExecutableFile(&cpp.pData) != osl_Process_E_None) {
        OSL_ASSERT(false);
    }

    sal_Int32 idx= cpp.lastIndexOf("idlc");
    cpp = cpp.copy(0, idx);

#if defined(_WIN32)
    cpp += "ucpp.exe";
#else
    cpp += "ucpp";
#endif
#else // SYSTEM_UCPP
    cpp = OUString(UCPP);
#endif
    oslProcess      hProcess = nullptr;
    oslProcessError procError = osl_Process_E_None;

    const int nCmdArgs = lCppArgs.size();
    std::unique_ptr<rtl_uString*[]> pCmdArgs(new rtl_uString*[nCmdArgs]);

    int i = 0;
    for (auto const& elem : lCppArgs)
    {
        pCmdArgs[i++] = elem.pData;
    }

    procError = osl_executeProcess( cpp.pData, pCmdArgs.get(), nCmdArgs, osl_Process_WAIT,
                                    nullptr, startDir.pData, nullptr, 0, &hProcess );

    oslProcessInfo hInfo;
    hInfo.Size = sal_uInt32(sizeof(oslProcessInfo));
    if (osl_getProcessInfo(hProcess, osl_Process_EXITCODE, &hInfo)
        != osl_Process_E_None)
    {
        OSL_ASSERT(false);
    }

    if ( procError || (hInfo.Code != 0) )
    {
        if ( procError != osl_Process_E_None )
            fprintf(stderr, "%s: starting preprocessor failed\n", pOptions->getProgramName().getStr());
        else
            fprintf(stderr, "%s: preprocessing %s%s failed\n",
                    pOptions->getProgramName().getStr(),
                    pathname == nullptr ? "" : "file ", fileName.getStr());

        osl_freeProcessHandle(hProcess);
        exit(hInfo.Code ? hInfo.Code : 99);
    }
    osl_freeProcessHandle(hProcess);

    if (unlink(tmpFile.getStr()) != 0)
    {
        fprintf(stderr, "%s: Could not remove cpp input file %s\n",
                 pOptions->getProgramName().getStr(), tmpFile.getStr());
        exit(99);
    }

    if ( pOptions->isValid("-E") )
    {
        if (unlink(preprocFile.getStr()) != 0)
        {
            fprintf(stderr, "%s: Could not remove parser input file %s\n",
                       pOptions->getProgramName().getStr(), preprocFile.getStr());
            exit(99);
        }
        exit(0);
    }

    // parse file
    yyin = fopen(preprocFile.getStr(), "r");
    if (yyin == nullptr)
    {
        fprintf(stderr, "%s: Could not open cpp output file %s\n",
                   pOptions->getProgramName().getStr(), preprocFile.getStr());
        exit(99);
    }

    //yydebug = 0 no trace information
    //yydebug = 1 parser produce trace information
    yydebug = 0;

    yyparse();
    sal_Int32 nErrors = idlc()->getErrorCount();

    fclose(yyin);
    if (unlink(preprocFile.getStr()) != 0)
    {
        fprintf(stderr, "%s: Could not remove parser input file %s\n",
                pOptions->getProgramName().getStr(), preprocFile.getStr());
        exit(99);
    }

    return nErrors;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
