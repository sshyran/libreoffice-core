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

#ifndef INCLUDED_XMLOFF_PROGRESSBARHELPER_HXX
#define INCLUDED_XMLOFF_PROGRESSBARHELPER_HXX

#include <sal/config.h>
#include <xmloff/dllapi.h>
#include <com/sun/star/task/XStatusIndicator.hpp>

inline constexpr OUStringLiteral XML_PROGRESSRANGE = u"ProgressRange";
inline constexpr OUStringLiteral XML_PROGRESSMAX = u"ProgressMax";
inline constexpr OUStringLiteral XML_PROGRESSCURRENT = u"ProgressCurrent";
inline constexpr OUStringLiteral XML_PROGRESSREPEAT = u"ProgressRepeat";

class XMLOFF_DLLPUBLIC ProgressBarHelper
{
            css::uno::Reference < css::task::XStatusIndicator >   xStatusIndicator;
            sal_Int32                                             nRange;
            sal_Int32                                             nReference;
            sal_Int32                                             nValue;
            double                                                fOldPercent;
            bool                                                  bStrict;
            // #96469#; if the value goes over the Range the progressbar starts again
            bool                                                  bRepeat;

#ifdef DBG_UTIL
            bool                                                  bFailure;
#endif
public:
            ProgressBarHelper(const css::uno::Reference < css::task::XStatusIndicator>& xStatusIndicator,
                                const bool bStrict);
            ~ProgressBarHelper();

            void SetRange(sal_Int32 nVal) { nRange = nVal; }
            void SetReference(sal_Int32 nVal) { nReference = nVal; }
            void SetValue(sal_Int32 nValue);
            void SetRepeat(bool bValue) { bRepeat = bValue; }
            void Increment(sal_Int32 nInc = 1) { SetValue( nValue+nInc ); }
            void End() { if (xStatusIndicator.is()) xStatusIndicator->end(); }

            // set the new reference and returns the new value which gives the
            // Progress Bar the same position as before
            void ChangeReference(sal_Int32 nNewReference);

            sal_Int32 GetReference() const { return nReference; }
            sal_Int32 GetValue() const { return nValue; }
            bool GetRepeat() const { return bRepeat; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
