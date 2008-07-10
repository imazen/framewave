/*
Copyright (c) 2006-2008 Advanced Micro Devices, Inc. All Rights Reserved.
This software is subject to the Apache v2.0 License.
*/


#include "UnitTestFramework.h"
#include "ut_ThresholdAndCompare.h"

#include "ThresholdAndCompareObjects.h"
#include "fwSignal.h"

class Test_sThreshold_LT_16s_I : public SDstLenThreshold_NoCmpOp<S16, S16>
{
public:
    Test_sThreshold_LT_16s_I( UnitTestCatalogBase & parent ) : SDstLenThreshold_NoCmpOp<S16, S16>( parent, "fwsThreshold_LT_16s_I", fwsThreshold_LT_16s_I )    {}

    virtual void RunAll()
    {
        S16 threshold;

        threshold = 127;
        RunTest( "0 126 127 201 100", "127 127 127 201 127", threshold );
    }
};

class Test_sThreshold_LT_16s : public SrcDstLenThreshold_NoCmpOp<S16, S16, S16>
{
public:
    Test_sThreshold_LT_16s( UnitTestCatalogBase & parent ) : SrcDstLenThreshold_NoCmpOp<S16, S16, S16>( parent, "fwsThreshold_LT_16s", fwsThreshold_LT_16s )    {}

    virtual void RunAll()
    {
        S16 threshold;

        threshold = 127;
        RunTest( "0 126 127 201 100", "127 127 127 201 127", threshold );
    }
};

DEFINE_TEST_TABLE( Catalog_sThreshold_LT )

TEST_ENTRY( Test_sThreshold_LT_16s_I )
TEST_ENTRY( Test_sThreshold_LT_16s )

END_TEST_TABLE()
