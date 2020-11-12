/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <basegfx/units/LengthUnit.hxx>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class LengthTest : public CppUnit::TestFixture
{
public:
    void test()
    {
        gfx::Length cm = 1_cm + 5_cm - 2_cm;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, cm.as_cm(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.04, cm.as_m(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(40.0, cm.as_mm(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(1440000), cm.raw());

        gfx::Length cm2 = 5_cm * 2;
        CPPUNIT_ASSERT_EQUAL(sal_Int64(3600000), cm2.raw());
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, cm2.as_cm(), 1e-4);

        // 1 km - 50 m = 950 m = 95000 cm
        gfx::Length cm3 = 100000_cm - 5000_cm;
        CPPUNIT_ASSERT_EQUAL(sal_Int64(34200000000), cm3.raw());
        CPPUNIT_ASSERT_DOUBLES_EQUAL(95000.0, cm3.as_cm(), 1e-4);

        gfx::Length cm4(1_cm);
        cm4 /= 2;
        CPPUNIT_ASSERT_EQUAL(sal_Int64(180000), cm4.raw());
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, cm4.as_cm(), 1e-4);

        // (635 * 20) + 3 * (635 * 15) = 41275EMU
        gfx::LengthUnit pt = 1_pt + 3_px;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(3.25, pt.as_pt(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, pt.as_twip(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0451, pt.as_in(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(41275), pt.raw());

        gfx::LengthUnit inch = 1_in; // 1440 * 635
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1440.0, inch.as_twip(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(96.0, inch.as_px(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, inch.as_in(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(914400.0, inch.as_emu(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(914400), inch.raw());

        // Conversion
        sal_Int64 asNumber(17_pt);
        asNumber += sal_Int64(1_pt);
        gfx::Length asLength = gfx::LengthUnit::emu(asNumber);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(18 * 635 * 20), asLength.raw());

        gfx::Length maximum = gfx::LengthUnit::emu(SAL_MAX_INT64);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(256204778801.5, maximum.as_m(), 1e-1);
        // 256204778 km
        CPPUNIT_ASSERT_EQUAL(sal_Int64(SAL_MAX_INT64), maximum.raw());

        gfx::Length minimum = gfx::LengthUnit::emu(SAL_MIN_INT64);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(-256204778801.5, minimum.as_m(), 1e-1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(double(SAL_MIN_INT64), minimum.as_emu(), 1e-1);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(SAL_MIN_INT64), minimum.raw());

        // 27 emu + 33 emu + 360 emu = 420
        gfx::Length emus = 27_emu + 33_emu + 1_hmm;
        CPPUNIT_ASSERT_EQUAL(sal_Int64(420), emus.raw());

        //  Creation from number
        int number = 10;
        auto asCm = gfx::Length::cm(number);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, asCm.as_cm(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(3600000), asCm.raw());

        auto asMm = gfx::Length::mm(number);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, asMm.as_mm(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(360000), asMm.raw());

        auto asInch = gfx::Length::in(number);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, asInch.as_in(), 1e-4);
        CPPUNIT_ASSERT_EQUAL(sal_Int64(9144000), asInch.raw());
    }

    CPPUNIT_TEST_SUITE(LengthTest);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(LengthTest);
