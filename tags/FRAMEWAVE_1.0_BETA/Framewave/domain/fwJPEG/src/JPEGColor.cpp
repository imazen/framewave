/*
Copyright (c) 2006-2008 Advanced Micro Devices, Inc. All Rights Reserved.
This software is subject to the Apache v2.0 License.
*/

//************************************************************************* 
//This file include the combined color conversion functions from JPEG Chapter
//	fwiRGBToYCbCr444LS_MCU_8u16s_C3P3R
//  fwiRGBToYCbCr422LS_MCU_8u16s_C3P3R
//	fwiRGBToYCbCr411LS_MCU_8u16s_C3P3R
//	fwiBGRToYCbCr444LS_MCU_8u16s_C3P3R
//	fwiBGR565ToYCbCr444LS_MCU_16u16s_C3P3R
//	fwiBGR555ToYCbCr444LS_MCU_16u16s_C3P3R
//	fwiBGRToYCbCr422LS_MCU_8u16s_C3P3R
//	fwiBGR565ToYCbCr422LS_MCU_16u16s_C3P3R
//	fwiBGR555ToYCbCr422LS_MCU_16u16s_C3P3R
//	fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R
//	fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R
//	fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R
//	fwiCMYKToYCCK444LS_MCU_8u16s_C4P4R
//	fwiCMYKToYCCK422LS_MCU_8u16s_C4P4R
//	fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R
//	fwiYCbCr444ToRGBLS_MCU_16s8u_P3C3R
//	fwiYCbCr422ToRGBLS_MCU_16s8u_P3C3R
//	fwiYCbCr411ToRGBLS_MCU_16s8u_P3C3R
//	fwiYCbCr444ToBGRLS_MCU_16s8u_P3C3R
//	fwiYCbCr444ToBGR565LS_MCU_16s16u_P3C3R
//	fwiYCbCr444ToBGR555LS_MCU_16s16u_P3C3R
//	fwiYCbCr422ToBGRLS_MCU_16s8u_P3C3R
//	fwiYCbCr422ToBGR565LS_MCU_16s16u_P3C3R
//	fwiYCbCr422ToBGR555LS_MCU_16s16u_P3C3R
//	fwiYCbCr411ToBGRLS_MCU_16s8u_P3C3R
//	fwiYCbCr411ToBGR565LS_MCU_16s16u_P3C3R
//	fwiYCbCr411ToBGR555LS_MCU_16s16u_P3C3R
//	fwiYCCK444ToCMYKLS_MCU_16s8u_P4C4R
//	fwiYCCK422ToCMYKLS_MCU_16s8u_P4C4R
//	fwiYCCK411ToCMYKLS_MCU_16s8u_P4C4R

//************************************************************************* 

#include "fwdev.h"
#include "fwJPEG.h"
#include "FwSharedCode_SSE2.h"
//#include "system.h"

using namespace OPT_LEVEL;

#if BUILD_NUM_AT_LEAST( 102 )
#ifndef __JPEG_COLOR
#define __JPEG_COLOR
#define FWJ_LIMIT5(val) (((val)<=0)?0:(((val)>=0x1f)?0x1f:((unsigned short)(val))))
#define FWJ_LIMIT6(val) (((val)<=0)?0:(((val)>=0x3f)?0x3f:((unsigned short)(val))))
#endif

namespace
{	
	#define STEPCHECK1(X) if (X<=0) return fwStsStepErr

	template <typename LoadClass, void StoreClass( const __m128i & r, const __m128i & g, const __m128i & b, void * pAlignedData )>
static	SYS_INLINE void ProcessHalfBlock (Fw16s *pSrcYa, Fw16s *pSrcYb, Fw16s *pSrcCb, Fw16s *pSrcCr, Fw8u *pDstRGB, int dstStep)
	{
		// Y:				Cb:					Cr:									RGB:
		//		  ----8----		  ---4-----4---		  ---4-----4---		==>		  --8---8--
		//		8 |   Ay   |	4 |A_cb | B_cb|		4 |A_cr | B_cr|		==>			8 | A | B |
		//		8 |   By   |	  -------------		  -------------		==>		  ---------
		//		  ----------
		
		__m128i y, cbVal, crVal, constant, rA, gA, bA, rB, gB, bB;
		constant = _mm_set1_epi16		( (S16)(128.5*64)	);

		for (U8 j=0; j<8; ++j)
		{
			//--------
			// BLOCK A
			//--------
			// Read Y
			LoadClass::Load( y, (__m128i*)pSrcYa );		// Y =		{  Y7,  Y6,  Y5,  Y4,  Y3,  Y2 , Y1,  Y0 }
			// Read Cr
			LoadClass::Load( crVal, (__m128i*)pSrcCr );	// crVal =	{ Cr7, Cr6, Cr5, Cr4, Cr3, Cr2, Cr1, Cr0 }
			// Unpack LO Cr with itself
			rB = _mm_unpacklo_epi16 ( crVal, crVal	);	// cr	 =	{ Cr3, Cr3, Cr2, Cr2, Cr1, Cr1, Cr0, Cr0 }
			
			rA = _mm_set1_epi16		( (short)90		);		// R = ( 1.402*2^8 )
			rA = _mm_mullo_epi16	( rA, rB		);		// R = ( 1.402*Cr )*(2^8)
			rA = _mm_adds_epi16		( rA, constant	);		// R = ( 128.5 + 1.402*Cr )*(2^8)
			rA = _mm_srai_epi16		( rA, 6			);		// R = ((128.5 + 1.402*Cr )*(2^8)) / (2^8)
			rA = _mm_adds_epi16		( rA, y			);		// R = Y + 128.5 + 1.402*Cr

			// Read Cb
			LoadClass::Load( cbVal, (__m128i*)pSrcCb );	// cbVal =	{ Cb7, Cb6, Cb5, Cb4, Cb3, Cb2, Cb1, Cb0 }
			// Unpack LO Cb with itself		
			gB = _mm_unpacklo_epi16	( cbVal, cbVal	);	// cb	 =	{ Cb3, Cb3, Cb2, Cb2, Cb1, Cb1, Cb0, Cb0 }

			gA = _mm_set1_epi16		( (short)(-46)	);		// G = ( -.71414*2^8 )
			gA = _mm_mullo_epi16	( gA, rB		);		// G = ( -.71414*Cr )*(2^8)
			gA = _mm_adds_epi16		( gA, constant	);		// G = ( -.71414*Cr + 128.5 )*(2^8)
			rB = _mm_set1_epi16		( (short)(-22)	);		// cr= ( -0.34414*(2^8) )
			rB = _mm_mullo_epi16	( rB, gB		);		// cr= ( -0.34414*Cb )*(2^8)
			gA = _mm_adds_epi16		( gA, rB		);		// G = ( -.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^8)
			gA = _mm_srai_epi16		( gA, 6			);		// G = ((-.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^8)) / (2^8)
			gA = _mm_adds_epi16		( gA, y			);		// G = Y - 0.34414*Cb - 0.71414*Cr + 128.5 
			
			bA = _mm_set1_epi16		( (short)(113)	);		// B = ( 1.772*(2^8) )
			bA = _mm_mullo_epi16	( bA, gB		);		// B = ( 1.772*Cb )*(2^8)
			bA = _mm_adds_epi16		( bA, constant	);		// B = ( 1.772*Cb + 128.5 )*(2^8)
			bA = _mm_srai_epi16		( bA, 6			);		// B = ((1.772*Cb + 128.5 )*(2^8)) / (2^8)
			bA = _mm_adds_epi16		( bA, y			);		// B = Y + 1.772*Cb + 128.5
			
			//--------
			// BLOCK B
			//-------
			// Read Y
			LoadClass::Load( y, (__m128i*)pSrcYb );			// Y =		{  Y7,  Y6,  Y5,  Y4,  Y3,  Y2 , Y1,  Y0 }
			// Unpack HI Cr with itself
			crVal = _mm_unpackhi_epi16 ( crVal, crVal	);	// cr	 =	{ Cr7, Cr7, Cr6, Cr6, Cr5, Cr5, Cr4, Cr4 }
			rB = _mm_set1_epi16		( (short)(90)	);
			rB = _mm_mullo_epi16	( rB, crVal		);		// R = ( 1.402*Cr )*(2^8)
			rB = _mm_adds_epi16		( rB, constant	);		// R = ( 128.5 + 1.402*Cr )*(2^8)
			rB = _mm_srai_epi16		( rB, 6			);		// R = ((128.5 + 1.402*Cr )*(2^8)) / (2^8)
			rB = _mm_adds_epi16		( rB, y			);		// R = Y + 128.5 + 1.402*Cr

			// Unpack HI Cb with itself (HI:block A, LO:block B)		
			cbVal = _mm_unpackhi_epi16	( cbVal, cbVal	);	// cb	 =	{ Cb7, Cb7, Cb6, Cb6, Cb5, Cb5, Cb4, Cb4 }

			gB = _mm_set1_epi16		( (short)(-46)	);
			gB = _mm_mullo_epi16	( gB, crVal		);		// g = ( -.71414*Cr )*(2^8)
			gB = _mm_adds_epi16		( gB, constant	);		// G = ( -.71414*Cr + 128.5 )*(2^8)
			crVal = _mm_set1_epi16	( (short)(-22)	);
			crVal =_mm_mullo_epi16	( crVal, cbVal	);		// cr= ( -0.34414*Cb )
			gB = _mm_adds_epi16		( gB, crVal		);		// G = ( -.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^8)
			gB = _mm_srai_epi16		( gB, 6			);		// G = ((-.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^8)) / (2^8)
			gB = _mm_adds_epi16		( gB, y			);		// G = Y - 0.34414*Cb - 0.71414*Cr + 128.5 
			
			bB = _mm_set1_epi16		( (short)(113)	);
			bB = _mm_mullo_epi16	( bB, cbVal		);		// B = ( 1.772*Cb )*(2^8)
			bB = _mm_adds_epi16		( bB, constant	);		// B = ( 1.772*Cb + 128.5 )*(2^8)
			bB = _mm_srai_epi16		( bB, 6			);		// B = ((1.772*Cb + 128.5 )*(2^8)) / (2^8)
			bB = _mm_adds_epi16		( bB, y			);		// B = Y + 1.772*Cb + 128.5

			// Pack R, G, and B
			rA = _mm_packus_epi16	( rA, rB		);		// R = { Rf, Re, Rd, Rc, Rb, Ra, R9, R8, R7, R6, R5, R4, R3, R2, R1, R0 }
			gA = _mm_packus_epi16	( gA, gB		);		// G = { Gf, Ge, Gd, Gc, Gb, Ga, G9, G8, G7, G6, G5, G4, G3, G2, G1, G0 }
			bA = _mm_packus_epi16	( bA, bB		);		// B = { Bf, Be, Bd, Bc, Bb, Ba, B9, B8, B7, B6, B5, B4, B3, B2, B1, B0 }

			StoreClass ( rA, gA, bA, pDstRGB );
			//CBL_SSE2::ConvertAndStoreAligned_3P_to_3C_8bit( rA, gA, bA, pDstRGB );
			pSrcYa += ( 8 ); 
			pSrcYb += ( 8 );
			pSrcCb += ( (j&0x1)*8 );
			pSrcCr += ( (j&0x1)*8 );
			pDstRGB += ( dstStep );
		}
	}

	SYS_INLINE FwStatus iYCbCr411ToRGBLS_MCU_16s8u_P3C3R_SSE2( const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep )
	{

		S16 *pSrcYA, *pSrcYB, *pSrcCb, *pSrcCr;
		U8 *pDst;

		// Y:				Cb:					Cr:									RGB:
		//		  ----8-----		  ---4-----4---		  ---4-----4---		==>			  --8---8--
		//		8 |   Ay   |		4 |A_cb | B_cb|		4 |A_cr | B_cr|		==>			8 | A | B |
		//		8 |   By   |		  -------------		  -------------		==>			  ---------
		//		8 |   Cy   |		4 |C_cb | D_cb|		4 |C_cr | D_cr|		==>			8 | C | D |
		//		8 |   Dy   |		  -------------		  -------------		==>			  ---------
		//		  ----------

		// Process top half and bottom half independently

		pSrcYA = (S16*)pSrcMCU[0];	
		pSrcYB = pSrcYA + 8*8;				// 64 pixels between Sections A & B
		pSrcCb = (S16*)pSrcMCU[1];
		pSrcCr = (S16*)pSrcMCU[2];
		pDst = pDstRGB;
		
		// Convert 16x8pixels
		if ( IS_ALIGNED16_5(pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst) && IS_ALIGNED16_1(dstStep) )
			ProcessHalfBlock <	CBL_LIBRARY::CBL_AlignedLoad,
								CBL_SSE2::ConvertAndStoreAligned_3P_to_3C_8bit	> (pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst, dstStep);
		else
			ProcessHalfBlock <	CBL_LIBRARY::CBL_UnalignedLoad, 
								CBL_SSE2::ConvertAndStoreUnaligned_3P_to_3C_8bit> (pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst, dstStep);

		pSrcYA = (S16*)pSrcMCU[0] + 8*16;	// 128 pixels from block A to C
		pSrcYB = pSrcYA + 8*8;				// 64 pixels between Sections C & D for Y
		pSrcCb = (S16*)pSrcMCU[1] + 8*4;	// 32 pixels between Sections C & D for Cb
		pSrcCr = (S16*)pSrcMCU[2] + 8*4;	// 32 pixels between Sections C & D for Cr
		pDst = pDstRGB + 8*dstStep;			// Process bottom half of 16x16 pixels (8x16)
		
		// Convert 16x8pixels
		if ( IS_ALIGNED16_5(pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst) && IS_ALIGNED16_1(dstStep) )
			ProcessHalfBlock <	CBL_LIBRARY::CBL_AlignedLoad, 
								CBL_SSE2::ConvertAndStoreAligned_3P_to_3C_8bit	> (pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst, dstStep);
		else
			ProcessHalfBlock <	CBL_LIBRARY::CBL_UnalignedLoad, 
								CBL_SSE2::ConvertAndStoreUnaligned_3P_to_3C_8bit> (pSrcYA, pSrcYB, pSrcCb, pSrcCr, pDst, dstStep);

		return fwStsNoErr;
	}

static SYS_INLINE void fwiYCbCr411ToRGBLS_MCU_16s8u_P3C3R_8x8Block (Fw16s *pSrcY, Fw16s *pSrcCb, Fw16s *pSrcCr, Fw8u *pDstRGB, int dstStep)
	{
		A32S y, x;
		A32S dstPos, srcYPos=0, srcCPos;
		A16S yVal, crVal, cbVal;

		for (y=0; y<8; ++y)
		{
			dstPos = y*dstStep;
			srcCPos = (y>>1)<<3; //(y>>1)*8;
			for (x=0; x<8; x+=2)
			{
				yVal = pSrcY[srcYPos++];
				cbVal = pSrcCb[srcCPos];
				crVal = pSrcCr[srcCPos++];
				// Compute RGB
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal + 1.402*crVal + 128.5);
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal - 0.34414*cbVal - 0.71414*crVal + 128.5);
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal + 1.772*cbVal + 128.5);

				yVal = pSrcY[srcYPos++];
				// Compute RGB
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal + 1.402*crVal + 128.5);
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal - 0.34414*cbVal - 0.71414*crVal + 128.5);
				pDstRGB[dstPos++] = FW_REF::Limits<A8U>::Sat( yVal + 1.772*cbVal + 128.5);
			}
		}
	}

	SYS_INLINE FwStatus iYCbCr411ToRGBLS_MCU_16s8u_P3C3R_REFR( const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep )
	{
		Fw16s y, srcCOffset, *pSrcY, *pSrcCb, *pSrcCr;
		Fw8u *pDstRGB_T;
		
		// Y:				Cb:					Cr:									RGB:
		//		  ----8----		  ---4-----4---		  ---4-----4---		==>			  --8---8--
		//		8 |   Ay   |		4 |A_cb | B_cb|		4 |A_cr | B_cr|		==>			8 | A | B |
		//		8 |   By   |		  -------------		  -------------		==>			  ---------
		//		8 |   Cy   |		4 |C_cb | D_cb|		4 |C_cr | D_cr|		==>			8 | C | D |
		//		8 |   Dy   |		  -------------		  -------------		==>			  ---------
		//		  ----------

		// Proces each block independently
		for (y=0; y<4; ++y)
		{
			// Arithmetic to point to the correct chroma block.  
			// (y&0x1)*4 = (y&0x1)<<2   = skip the left 4 pixels
			// (y&0x2)*2*8 = (y&0x2)<<4 = Skip the top 16x8 pixels
			// if y is odd
			//		Convert RIGHT hand side block
			// else
			//		Convert LEFT hand side block	
			// if y >=2 (bit 1 of y is set)
			//		Convert TOP half of block
			// else
			//		Convert BOTTOM half of block
			srcCOffset = ((y&0x1)<<2) + ((y&0x2)<<4);

			pSrcY = (Fw16s*)pSrcMCU[0] + (y<<6);//y*8*8;			// Pointer to the next block to be processed
			pSrcCb = (Fw16s*)pSrcMCU[1] + srcCOffset;
			pSrcCr = (Fw16s*)pSrcMCU[2] + srcCOffset;
			pDstRGB_T = pDstRGB + (y&0x1)*8*3 + ((y>>1)*8*dstStep);
			//pDstRGB_T = pDstRGB + (y&0x1)*8*3 + ((y>>1)*8*dstStep);
 
			// Process each 8x8 block
			fwiYCbCr411ToRGBLS_MCU_16s8u_P3C3R_8x8Block (pSrcY, pSrcCb, pSrcCr, pDstRGB_T, dstStep);
		}

		return fwStsNoErr;
	}
};

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr411ToRGBLS_MCU_16s8u_P3C3R)( const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep )
{
	if (pDstRGB==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 || pSrcMCU[1]==0 || pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	// will not allow dstStep overlap
	if (dstStep < 48) return fwStsStepErr;

	switch( Dispatch::Type<DT_SSE2>() )
	{
	case DT_SSE3:
	case DT_SSE2:
		return iYCbCr411ToRGBLS_MCU_16s8u_P3C3R_SSE2( pSrcMCU, pDstRGB, dstStep );
	default:
		return iYCbCr411ToRGBLS_MCU_16s8u_P3C3R_REFR( pSrcMCU, pDstRGB, dstStep );	
	}
}



//-----------------------------------------------------------------------
//Convert an 8*8 RGB image to the YCbCr color mode and create 444MCU
//
//-----------------------------------------------------------------------

FwStatus PREFIX_OPT(OPT_PREFIX, fwiRGBToYCbCr444LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcRGB, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcRGB==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);
	unsigned short RVal, GVal, BVal;
	int x, y, srcPos, result;

	for (y=0;y<8; y++) {//8*8 image
		srcPos = y*srcStep;
		for (x=0;x<8;x++) {
			RVal=pSrcRGB[srcPos++]; 
			GVal=pSrcRGB[srcPos++];
			BVal=pSrcRGB[srcPos++];
			//add 0.5 for nearest neighbor rounding
			result =  77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][x+y*8] = (Fw16s)((result>>8)-128);
			result = -43 * RVal - 85 * GVal	+ 128 * BVal + 128;
			pDstMCU [1][x+y*8] = (Fw16s)(result>>8);
			result = 128 * RVal - 107 * GVal - 21 * BVal + 128;
			pDstMCU [2][x+y*8] = (Fw16s)(result>>8);
		}
	}

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Convert an 16*8 RGB image to the YCbCr color mode and create 422MCU
//
//-----------------------------------------------------------------------
static void fwiRGBToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block(
	const Fw8u *pSrcRGB, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned char RVal, GVal, BVal;
	int result, x, y;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb=0, elementCr=0;

	srcOffset  = (blocknum<<3)*3;
	dstCOffset = blocknum << 2;
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		dstCPos = y*8 + dstCOffset;
		for (x=0;x<8;x++) {
			RVal = pSrcRGB[srcPos++];
			GVal = pSrcRGB[srcPos++];
			BVal = pSrcRGB[srcPos++];
			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

			if (!(x&1)){
				elementCb = -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr = 128 * RVal - 107 * GVal - 21 * BVal;
			} else {
				result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb + 256;
				pDstMCU [1][dstCPos] = (Fw16s)(result>>9);
				result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr + 256;
				pDstMCU [2][dstCPos++] = (Fw16s)(result>>9);
			}	
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiRGBToYCbCr422LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcRGB, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcRGB==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//					  ----------
	// Proces each block independently

	fwiRGBToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block (pSrcRGB, srcStep, pDstMCU, 0);
	fwiRGBToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block (pSrcRGB, srcStep, pDstMCU, 1);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Convert an 16*16 RGB image to the YCbCr color mode and create 411MCU
//
//-----------------------------------------------------------------------

FwStatus PREFIX_OPT(OPT_PREFIX, fwiRGBToYCbCr411LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcRGB, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcRGB==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);
	// 8x8 blocks:
	//				---------
	//				| A | B |
	//				---------
	//				| C | D |
	//				---------
	// Y:								Cb:								Cr:
	//		---------						-------------					-------------
	//		|   Ay  |						|A_cb | B_cb|					|A_cr | B_cr|
	//		|   By  |						-------------					-------------
	//		|   Cy  |						|C_cb | D_cb|					|C_cr | D_cr|
	//		|   Dy  |						-------------					-------------
	//		---------

	A16U rVal, gVal, bVal;
	A16S result;
	A32U rSum[8] = {0,0,0,0,0,0,0,0}, gSum[8] = {0,0,0,0,0,0,0,0}, bSum[8] = {0,0,0,0,0,0,0,0};
	A32S x, y, srcPos, dstYPos=0, dstCPos=0;

	// Process 2 8x8 block: A & B
	// Proces 2 rows at a time							//  Pixels
	for (y=0; y<8; ++y)									//  -------			---------	------	------
	{													//  | a b |	  -->	| ay by |	| cb |	| cr |
		// First Row									//	| c d |			| cy dy	|	------	------
		srcPos = y*srcStep;								//	-------			---------
		for (x=0; x<8; x+=2)
		{												
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1]+=rVal;
			gSum[x>>1]+=gVal;
			bSum[x>>1]+=bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1]+=rVal;
			gSum[x>>1]+=gVal;
			bSum[x>>1]+=bVal;
		}
		dstYPos = dstYPos+56;
		for ( ; x<16; x+=2)
		{
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos-64;
		
		// Second Row
		srcPos = (++y)*srcStep;
		for (x=0; x<8; x+=2)
		{												
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos+56;
		for ( ; x<16; x+=2)
		{
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos-64;
		
		// Compute and store Cb & Cr Values
		for (x=0; x<8; ++x)
		{
			rSum[x] = rSum[x]>>2;	// Average of 4 values
			gSum[x] = gSum[x]>>2;	// Average of 4 values
			bSum[x] = bSum[x]>>2;	// Average of 4 values
			result = (A16S)(-43 * rSum[x] - 85 * gSum[x] + 128 * bSum[x] + 128);
			pDstMCU[1][dstCPos] = (result>>8);		// cb
			result = (A16S)(128 * rSum[x] - 107 * gSum[x] - 21 * bSum[x] + 128);
			pDstMCU[2][dstCPos++] = (result>>8);	// cr
			rSum[x] = gSum[x] = bSum[x] = 0;
		}
	}
	// Process 2 8x8 block: C & D
	dstYPos = 128;
	for ( ; y<16; ++y)
	{
		// First Row
		srcPos = y*srcStep;
		for (x=0; x<8; x+=2)
		{												
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos+56;
		for ( ; x<16; x+=2)
		{
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos-64;
		
		// Second Row
		srcPos = (++y)*srcStep;
		for (x=0; x<8; x+=2)
		{												
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos+56;
		for ( ; x<16; x+=2)
		{
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (ay,cy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
			
			rVal = pSrcRGB[srcPos++];
			gVal = pSrcRGB[srcPos++];
			bVal = pSrcRGB[srcPos++];
			result = (rVal*77 + gVal*150 + bVal*29) >> 8;
			pDstMCU[0][dstYPos++] = (result - 128);			// (by,dy)
			rSum[x>>1] += rVal;
			gSum[x>>1] += gVal;
			bSum[x>>1] += bVal;
		}
		dstYPos = dstYPos-64;
		
		// Compute and store Cb & Cr
		for (x=0; x<8; ++x)
		{
			rSum[x] = rSum[x]>>2;	// Average of 4 values
			gSum[x] = gSum[x]>>2;	// Average of 4 values
			bSum[x] = bSum[x]>>2;	// Average of 4 values
			result = (A16S)(-43 * rSum[x] - 85 * gSum[x] + 128 * bSum[x] + 128);
			pDstMCU[1][dstCPos] = (result>>8);		// cb
			result = (A16S)(128 * rSum[x] - 107 * gSum[x] - 21 * bSum[x] + 128);
			pDstMCU[2][dstCPos++] = (result>>8);	// cr
			rSum[x] = gSum[x] = bSum[x] = 0;
		}
	}
	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Convert an 8*8 BGR image to the YCbCr color mode and create 444MCU
//
//-----------------------------------------------------------------------

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGRToYCbCr444LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);
	unsigned short RVal, GVal, BVal;
	int x, y, srcPos, result;

	for (y=0;y<8; y++) {//8*8 image
		srcPos = y*srcStep;
		for (x=0;x<8;x++) {
			BVal=pSrcBGR[srcPos++]; 
			GVal=pSrcBGR[srcPos++];
			RVal=pSrcBGR[srcPos++];

			//add 0.5 for nearest neighbor rounding
			result =  77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][x+y*8] = (Fw16s)((result>>8)-128);
			result = -43 * RVal - 85 * GVal	+ 128 * BVal + 128;
			pDstMCU [1][x+y*8] = (Fw16s)(result>>8);
			result = 128 * RVal - 107 * GVal - 21 * BVal + 128;
			pDstMCU [2][x+y*8] = (Fw16s)(result>>8);
		}
	}

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR565ToYCbCr444LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);
	unsigned short RVal, GVal, BVal;
	int x, y, srcPos, result;

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	for (y=0;y<8; y++) {//8*8 image
		srcPos = y*srcStep;
		for (x=0;x<8;x++) {
			//extend the RGB range to [0-255]
			BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
			GVal=((pSrcBGR[srcPos]>>5)&0x3f)<<2;
			RVal=(pSrcBGR[srcPos++]>>11)<<3;

			//add 0.5 for nearest neighbor rounding
			result =  77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][x+y*8] = (Fw16s)((result>>8)-128);
			result = -43 * RVal - 85 * GVal	+ 128 * BVal + 128;
			pDstMCU [1][x+y*8] = (Fw16s)(result>>8);
			result = 128 * RVal - 107 * GVal - 21 * BVal + 128;
			pDstMCU [2][x+y*8] = (Fw16s)(result>>8);
		}
	}

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR555ToYCbCr444LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);
	unsigned short RVal, GVal, BVal;
	int x, y, srcPos, result;

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	for (y=0;y<8; y++) {//8*8 image
		srcPos = y*srcStep;
		for (x=0;x<8;x++) {
			//extend the RGB range to [0-255]
			BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
			GVal=((pSrcBGR[srcPos]>>5)&0x1f)<<3;
			RVal=((pSrcBGR[srcPos++]>>10)&0x1f)<<3;

			//add 0.5 for nearest neighbor rounding
			result =  77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][x+y*8] = (Fw16s)((result>>8)-128);
			result = -43 * RVal - 85 * GVal	+ 128 * BVal + 128;
			pDstMCU [1][x+y*8] = (Fw16s)(result>>8);
			result = 128 * RVal - 107 * GVal - 21 * BVal + 128;
			pDstMCU [2][x+y*8] = (Fw16s)(result>>8);
		}
	}

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Convert an 16*8 BGR image to the YCbCr color mode and create 422MCU
//
//-----------------------------------------------------------------------
static void fwiBGRToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block(
	const Fw8u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned char RVal, GVal, BVal;
	int result, x, y;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb=0, elementCr=0;

	srcOffset  = (blocknum<<3)*3;
	dstCOffset = blocknum << 2;
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		dstCPos = y*8 + dstCOffset;
		for (x=0;x<8;x++) {
			BVal = pSrcBGR[srcPos++];
			GVal = pSrcBGR[srcPos++];
			RVal = pSrcBGR[srcPos++];
			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

			if (!(x&1)){
				elementCb = -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr = 128 * RVal - 107 * GVal - 21 * BVal;
			} else {
				result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb + 256;
				pDstMCU [1][dstCPos] = (Fw16s)(result>>9);
				result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr + 256;
				pDstMCU [2][dstCPos++] = (Fw16s)(result>>9);
			}	
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGRToYCbCr422LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//					  ----------
	// Proces each block independently

	fwiBGRToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGRToYCbCr422LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);

	return fwStsNoErr;
}

static void fwiBGR565ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block(
	const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned short RVal, GVal, BVal;
	int result, x, y;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb=0, elementCr=0;

	srcOffset  = blocknum << 3;
	dstCOffset = blocknum << 2;
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		dstCPos = y*8 + dstCOffset;
		for (x=0;x<8;x++) {		
			BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
			GVal=((pSrcBGR[srcPos]>>5)&0x3f)<<2;
			RVal=(pSrcBGR[srcPos++]>>11)<<3;

			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

			if (!(x&1)){
				elementCb = -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr = 128 * RVal - 107 * GVal - 21 * BVal;
			} else {
				result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb + 256;
				pDstMCU [1][dstCPos] = (Fw16s)(result>>9);
				result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr + 256;
				pDstMCU [2][dstCPos++] = (Fw16s)(result>>9);
			}	
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR565ToYCbCr422LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//					  ----------
	// Proces each block independently

	fwiBGR565ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGR565ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);

	return fwStsNoErr;
}

static void fwiBGR555ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block(
	const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned short RVal, GVal, BVal;
	int result, x, y;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb=0, elementCr=0;

	srcOffset  = blocknum << 3;
	dstCOffset = blocknum << 2;
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		dstCPos = y*8 + dstCOffset;
		for (x=0;x<8;x++) {		
			BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
			GVal=((pSrcBGR[srcPos]>>5)&0x1f)<<3;
			RVal=(pSrcBGR[srcPos++]>>10)<<3;

			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

			if (!(x&1)){
				elementCb = -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr = 128 * RVal - 107 * GVal - 21 * BVal;
			} else {
				result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb + 256;
				pDstMCU [1][dstCPos] = (Fw16s)(result>>9);
				result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr + 256;
				pDstMCU [2][dstCPos++] = (Fw16s)(result>>9);
			}	
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR555ToYCbCr422LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//					  ----------
	// Proces each block independently

	fwiBGR555ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGR555ToYCbCr422LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Convert an 16*16 BGR image to the YCbCr color mode and create 411MCU
//
//-----------------------------------------------------------------------
static void fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R_8x8Block(
	const Fw8u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned char RVal, GVal, BVal;
	int result, x, y, x1;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb[4]={0, 0, 0, 0}; 
	int elementCr[4]={0, 0, 0, 0};

	srcOffset  = ((blocknum&0x1)<<3)*3 + ((blocknum&0x2)<<2)*srcStep;
	dstCOffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		if (!(y&1)) {
			for (x=0;x<8;x++) {
				BVal = pSrcBGR[srcPos++];
				GVal = pSrcBGR[srcPos++];
				RVal = pSrcBGR[srcPos++];
				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
			}
		} else {
			dstCPos = (y>>1)*8 + dstCOffset;
			for (x=0;x<8;x++) {
				RVal = pSrcBGR[srcPos++];
				GVal = pSrcBGR[srcPos++];
				BVal = pSrcBGR[srcPos++];
				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				if (!(x&1)) {
					elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
					elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
				} else {
					result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb[x1] + 512;
					pDstMCU [1][dstCPos] = (Fw16s)(result>>10);
					elementCb[x1] = 0; //clean after using the value
					result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr[x1] + 512;
					pDstMCU [2][dstCPos++] = (Fw16s)(result>>10);
					elementCr[x1] = 0;
				}	
			}
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R)(const Fw8u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	// RGB:				 Y:			 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	4 |A_cb | B_cb|	4 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//	8 | C | D |	==>	8 |   Cy   |	4 |C_cb | D_cb|	4 |C_cr | D_cr|
	//	---------	==>	8 |   Dy   |	-------------	-------------	
	//					  ----------
	// Proces each block independently

	fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);
	fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 2);
	fwiBGRToYCbCr411LS_MCU_8u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 3);

	return fwStsNoErr;
}

static void fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block(
	const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned short RVal, GVal, BVal;
	int result, x, y, x1;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb[4]={0, 0, 0, 0}; 
	int elementCr[4]={0, 0, 0, 0};

	srcOffset  = ((blocknum&0x1)<<3) + ((blocknum&0x2)<<2)*srcStep;
	dstCOffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		if (!(y&1)) {
			for (x=0;x<8;x++) {				
				BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
				GVal=((pSrcBGR[srcPos]>>5)&0x3f)<<2;
				RVal=(pSrcBGR[srcPos++]>>11)<<3;

				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
			}
		} else {
			dstCPos = (y>>1)*8 + dstCOffset;
			for (x=0;x<8;x++) {
				BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
				GVal=((pSrcBGR[srcPos]>>5)&0x3f)<<2;
				RVal=(pSrcBGR[srcPos++]>>11)<<3;

				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				if (!(x&1)) {
					elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
					elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
				} else {
					result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb[x1] + 512;
					pDstMCU [1][dstCPos] = (Fw16s)(result>>10);
					elementCb[x1] = 0; //clean after using the value
					result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr[x1] + 512;
					pDstMCU [2][dstCPos++] = (Fw16s)(result>>10);
					elementCr[x1] = 0;
				}	
			}
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	// RGB:				 Y:			 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	4 |A_cb | B_cb|	4 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//	8 | C | D |	==>	8 |   Cy   |	4 |C_cb | D_cb|	4 |C_cr | D_cr|
	//	---------	==>	8 |   Dy   |	-------------	-------------	
	//					  ----------
	// Proces each block independently

	fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);
	fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 2);
	fwiBGR565ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 3);

	return fwStsNoErr;
}

static void fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block(
	const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3], int blocknum)
{
	unsigned short RVal, GVal, BVal;
	int result, x, y, x1;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb[4]={0, 0, 0, 0}; 
	int elementCr[4]={0, 0, 0, 0};

	srcOffset  = ((blocknum&0x1)<<3) + ((blocknum&0x2)<<2)*srcStep;
	dstCOffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		if (!(y&1)) {
			for (x=0;x<8;x++) {				
				BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
				GVal=((pSrcBGR[srcPos]>>5)&0x1f)<<3;
				RVal=(pSrcBGR[srcPos++]>>10)<<3;

				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
			}
		} else {
			dstCPos = (y>>1)*8 + dstCOffset;
			for (x=0;x<8;x++) {
				BVal=(pSrcBGR[srcPos]&0x1f)<<3; 
				GVal=((pSrcBGR[srcPos]>>5)&0x1f)<<3;
				RVal=(pSrcBGR[srcPos++]>>10)<<3;

				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos++] = (Fw16s)((result>>8)-128);

				x1=x>>1;
				if (!(x&1)) {
					elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
					elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
				} else {
					result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb[x1] + 512;
					pDstMCU [1][dstCPos] = (Fw16s)(result>>10);
					elementCb[x1] = 0; //clean after using the value
					result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr[x1] + 512;
					pDstMCU [2][dstCPos++] = (Fw16s)(result>>10);
					elementCr[x1] = 0;
				}	
			}
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R)(const Fw16u *pSrcBGR, int srcStep, Fw16s *pDstMCU[3])
{

	if (pSrcBGR==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	//srcStep is byte size
	srcStep = srcStep / sizeof (Fw16u);

	// RGB:				 Y:			 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	4 |A_cb | B_cb|	4 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//	8 | C | D |	==>	8 |   Cy   |	4 |C_cb | D_cb|	4 |C_cr | D_cr|
	//	---------	==>	8 |   Dy   |	-------------	-------------	
	//					  ----------
	// Proces each block independently

	fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 0);
	fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 1);
	fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 2);
	fwiBGR555ToYCbCr411LS_MCU_16u16s_C3P3R_8x8Block (pSrcBGR, srcStep, pDstMCU, 3);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Converts a CMYK 8*8, 16*8, 16*16 image to YCCK and creates corresponding
//MCU image.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiCMYKToYCCK444LS_MCU_8u16s_C4P4R)(const Fw8u *pSrcCMYK, int srcStep, Fw16s *pDstMCU[4])
{

	if (pSrcCMYK==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0 ||
		pDstMCU[3]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536
	//The final answer is equal to nearest neighbor rounding
	//SEE2 should use 16 bit data shift
	int result;
	unsigned char RVal, GVal, BVal;
	int x, y;
	int srcPos, dstPos;

	for (y=0;y<8; y++) {
		srcPos = y*srcStep;
		dstPos = y*8;
		for (x=0;x<8;x++) {
			RVal = ~(pSrcCMYK[srcPos++]);//R=255-C
			GVal = ~(pSrcCMYK[srcPos++]);//G=255-M
			BVal = ~(pSrcCMYK[srcPos++]);//B=255-Y

			//add 0.5 for nearest neighbor rounding
			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstPos] = (Fw16s)((result>>8)-128);
			result = -43 * RVal - 85 * GVal + 128 * BVal + 128;
			pDstMCU [1][dstPos] = (Fw16s)(result>>8);
			result = 128 * RVal - 107 * GVal - 21 * BVal + 128;
			pDstMCU [2][dstPos] = (Fw16s)(result>>8);
			pDstMCU [3][dstPos++] = pSrcCMYK[srcPos++]-128;
		}
	}

	return fwStsNoErr;
}

static void fwiCMYKToYCCK422LS_MCU_8u16s_C4P4R_8x8Block(
	const Fw8u *pSrcCMYK, int srcStep, Fw16s *pDstMCU[4], int blocknum)
{
	unsigned char RVal, GVal, BVal;
	int result, x, y;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb=0, elementCr=0;

	srcOffset  = blocknum << 5;
	dstCOffset = blocknum << 2;
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		dstCPos = y*8 + dstCOffset;
		for (x=0;x<8;x++) {
			RVal = ~(pSrcCMYK[srcPos++]);//R=255-C
			GVal = ~(pSrcCMYK[srcPos++]);//G=255-M
			BVal = ~(pSrcCMYK[srcPos++]);//B=255-Y
			result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
			pDstMCU [0][dstYPos] = (Fw16s)((result>>8)-128);
			pDstMCU [3][dstYPos++] = pSrcCMYK[srcPos++]-128;
			if (!(x&1)){
				elementCb = -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr = 128 * RVal - 107 * GVal - 21 * BVal;
			} else {
				result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb + 256;
				pDstMCU [1][dstCPos] = (Fw16s)(result>>9);
				result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr + 256;
				pDstMCU [2][dstCPos++] = (Fw16s)(result>>9);
			}	
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiCMYKToYCCK422LS_MCU_8u16s_C4P4R)(const Fw8u *pSrcCMYK, int srcStep, Fw16s *pDstMCU[4])
{

	if (pSrcCMYK==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0 ||
		pDstMCU[3]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//					  ----------
	// Proces each block independently

	fwiCMYKToYCCK422LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 0);
	fwiCMYKToYCCK422LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 1);

	return fwStsNoErr;
}

static void fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R_8x8Block(
	const Fw8u *pSrcCMYK, int srcStep, Fw16s *pDstMCU[4], int blocknum)
{
	unsigned char RVal, GVal, BVal;
	int result, x, y, x1;
	int srcPos, srcOffset, dstYPos, dstCOffset, dstCPos;
	int elementCb[4]={0, 0, 0, 0}; 
	int elementCr[4]={0, 0, 0, 0};

	srcOffset  = ((blocknum&0x1)<<5) + ((blocknum&0x2)<<2)*srcStep;
	dstCOffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	dstYPos    = blocknum << 6;

	//DEV code use shift 8 bit data for coeffcients
	//0.299*256=76.544, 0.587*256=150.272, 0.114*256=29.184
	//We use 77, 150, 29 as the modified coeff, and then shift the result
	//-0.16874*256 = -43.19744, -0.33126*256=-84.80256, 0.5*256=128
	//0.5*256=128, -0.41869*256 = -107.18464, -0.08131*256=-20.81536

	for (y=0;y<8; y++) {
		srcPos = y*srcStep + srcOffset;
		if (!(y&1)) {
			for (x=0;x<8;x++) {
				RVal = ~(pSrcCMYK[srcPos++]);//R=255-C
				GVal = ~(pSrcCMYK[srcPos++]);//G=255-M
				BVal = ~(pSrcCMYK[srcPos++]);//B=255-Y
				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos] = (Fw16s)((result>>8)-128);
				pDstMCU [3][dstYPos++] = pSrcCMYK[srcPos++]-128;
				x1=x>>1;
				elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
				elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
			}
		} else {
			dstCPos = (y>>1)*8 + dstCOffset;
			for (x=0;x<8;x++) {
				RVal = ~(pSrcCMYK[srcPos++]);//R=255-C
				GVal = ~(pSrcCMYK[srcPos++]);//G=255-M
				BVal = ~(pSrcCMYK[srcPos++]);//B=255-Y
				result = 77 * RVal + 150 * GVal + 29 * BVal + 128;
				pDstMCU [0][dstYPos] = (Fw16s)((result>>8)-128);
				pDstMCU [3][dstYPos++] = pSrcCMYK[srcPos++]-128;
				x1=x>>1;
				if (!(x&1)) {
					elementCb[x1] += -43 * RVal - 85 * GVal + 128 * BVal;
					elementCr[x1] += 128 * RVal - 107 * GVal - 21 * BVal;
				} else {
					result = -43 * RVal - 85 * GVal + 128 * BVal + elementCb[x1] + 512;
					pDstMCU [1][dstCPos] = (Fw16s)(result>>10);
					elementCb[x1] = 0; //clean after using the value
					result = 128 * RVal - 107 * GVal - 21 * BVal + elementCr[x1] + 512;
					pDstMCU [2][dstCPos++] = (Fw16s)(result>>10);
					elementCr[x1] = 0;
				}	
			}
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R)(const Fw8u *pSrcCMYK, int srcStep, Fw16s *pDstMCU[4])
{

	if (pSrcCMYK==0 || pDstMCU==0) return fwStsNullPtrErr;
	if (pDstMCU[0]==0 ||pDstMCU[1]==0 ||pDstMCU[2]==0 ||
		pDstMCU[3]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(srcStep);

	// RGB:				 Y, K:		 Cb:				Cr:				
	//	--8---8--	==>	  ----8----	     ---4-----4---	---4-----4---	  
	//	8 | A | B |	==>	8 |   Ay   |	4 |A_cb | B_cb|	4 |A_cr | B_cr|
	//	---------	==>	8 |   By   |	-------------	-------------	  
	//	8 | C | D |	==>	8 |   Cy   |	4 |C_cb | D_cb|	4 |C_cr | D_cr|
	//	---------	==>	8 |   Dy   |	-------------	-------------	
	//					  ----------
	// Proces each block independently

	fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 0);
	fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 1);
	fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 2);
	fwiCMYKToYCCK411LS_MCU_8u16s_C4P4R_8x8Block (pSrcCMYK, srcStep, pDstMCU, 3);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Creates a YCBCr image from MCU and then converts it to RGB format
//-----------------------------------------------------------------------
static FwStatus iYCbCr444ToRGBLS_MCU_16s8u_P3C3R_REF(const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep)
{

	if (pDstRGB==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	int srcPos=0, dstPos;
	Fw16s cbVal, crVal;

	for (y=0;y<8; y++) {
		//srcPos = y*8;
		dstPos = y*dstStep;
		for (x=0;x<8;x++) {
			cbVal = pSrcMCU[1][srcPos];
			crVal = pSrcMCU[2][srcPos];
			pDstRGB[dstPos++] = FW_REF::Limits<U8>::Sat(pSrcMCU[0][srcPos] + 1.402*crVal + 128.5);
			pDstRGB[dstPos++] = FW_REF::Limits<U8>::Sat(pSrcMCU[0][srcPos] - 0.34414*cbVal - 0.71414*crVal+ 128.5);
			pDstRGB[dstPos++] = FW_REF::Limits<U8>::Sat(pSrcMCU[0][srcPos++] + 1.772*cbVal + 128.5);
		}
	}
	return fwStsNoErr;
}

template <typename LoadClass, typename StoreClass>
static SYS_INLINE void fwiYCbCr444ToRGBLS_MCU_16s8u_P3C3R_SSE2_8x1 (Fw16s *ptrY, Fw16s *ptrCb, Fw16s *ptrCr, Fw8u *pDst)
{
	__m128i y, c, r, g, b, constant;

	constant = _mm_set1_epi16 ( (S16)(128.5*64)	);
	LoadClass::Load	(y, (__m128i *)ptrY);
	LoadClass::Load (c, (__m128i *)ptrCr);

	// RED
	r = _mm_set1_epi16		( (S16)90		);		// R = ( 1.402*2^6 )
	r = _mm_mullo_epi16		( r, c			);		// R = ( 1.402*Cr )*(2^6)
	r = _mm_adds_epi16		( r, constant	);		// R = ( 128.5 + 1.402*Cr )*(2^6)
	r = _mm_srai_epi16		( r, 6			);		// R = ((128.5 + 1.402*Cr )*(2^6)) / (2^6)
	r = _mm_adds_epi16		( r, y			);		// R = Y + 128.5 + 1.402*Cr

	// GREEN
	g = _mm_set1_epi16		( (S16)(-46)	);		// G = ( -.71414*2^6 )
	g = _mm_mullo_epi16		( g, c			);		// G = ( -.71414*Cr )*(2^6)
	LoadClass::Load			( c, (__m128i *)ptrCb);
	g = _mm_adds_epi16		( g, constant	);		// G = ( -.71414*Cr + 128.5 )*(2^6)
	b = _mm_set1_epi16		( (short)(-22)	);		// cr= ( -0.34414*(2^6) )
	b = _mm_mullo_epi16		( b, c			);		// cr= ( -0.34414*Cb )*(2^6)
	g = _mm_adds_epi16		( g, b			);		// G = ( -.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^6)
	g = _mm_srai_epi16		( g, 6			);		// G = ((-.71414*Cr + 128.5 + (-0.34414*Cb) )*(2^6)) / (2^6)
	g = _mm_adds_epi16		( g, y			);		// G = Y - 0.34414*Cb - 0.71414*Cr + 128.5 

	// BLUE
	b = _mm_set1_epi16		( (S16)(113)	);		// B = ( 1.772*(2^6) )
	b = _mm_mullo_epi16		( b, c			);		// B = ( 1.772*Cb )*(2^6)
	b = _mm_adds_epi16		( b, constant	);		// B = ( 1.772*Cb + 128.5 )*(2^6)
	b = _mm_srai_epi16		( b, 6			);		// B = ((1.772*Cb + 128.5 )*(2^6)) / (2^6)
	b = _mm_adds_epi16		( b, y			);		// B = Y + 1.772*Cb + 128.5

	CBL_SSE2::Convert_3P_to_3C_16bit( r, g, b);
	// r = {g2,r2,b1,g1,r1,b0,g0,r0}
	// g = {r5,b4,g4,r4,b3,g3,r3,b2}
	// b = {b7,g7,r7,b6,g6,r6,b5,g5}

	r = _mm_packus_epi16 (r, g);			// r = {r5,b4,g4,r4,b3,g3,r3,b2,g2,r2,b1,g1,r1,b0,g0,r0}
#pragma warning( disable: 4328 )
	StoreClass::Store ((__m128i*)pDst, r);
	b = _mm_packus_epi16 (b, b);			// b = {b7,g7,r7,b6,g6,r6,b5,g5,b7,g7,r7,b6,g6,r6,b5,g5}
	_mm_storel_epi64 ( ((__m128i*)pDst+1), b );
#pragma warning( default: 4328 )
}

static FwStatus iYCbCr444ToRGBLS_MCU_16s8u_P3C3R_SSE2(const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep)
{
	Fw16s *pSrcY=(Fw16s*)pSrcMCU[0], *pSrcCb=(Fw16s*)pSrcMCU[1], *pSrcCr=(Fw16s*)pSrcMCU[2];
	Fw8u *pDst;

	if (IS_ALIGNED16_5(pSrcY, pSrcCb, pSrcCr, pDstRGB, dstStep)){
		for (S32 j=0; j<8; ++j) {
			pDst = j*dstStep + pDstRGB;
			fwiYCbCr444ToRGBLS_MCU_16s8u_P3C3R_SSE2_8x1<CBL_LIBRARY::CBL_AlignedLoad, 
				CBL_LIBRARY::CBL_StreamingStore>(pSrcY, pSrcCb, pSrcCr, pDst);
			pSrcY += 8; 
			pSrcCb += 8; 
			pSrcCr += 8;	
		}
	} else {
		for (S32 j=0; j<8; ++j){
			pDst = j*dstStep + pDstRGB;
			fwiYCbCr444ToRGBLS_MCU_16s8u_P3C3R_SSE2_8x1<CBL_LIBRARY::CBL_UnalignedLoad,
				CBL_LIBRARY::CBL_UnalignedStore> (pSrcY, pSrcCb, pSrcCr, pDst);
			pSrcY += 8; 
			pSrcCb += 8; 
			pSrcCr += 8;
		}
	}
	return fwStsNoErr;
}
FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr444ToRGBLS_MCU_16s8u_P3C3R)(const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep)
{
	if (pDstRGB==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 || pSrcMCU[1]==0 || pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	if (dstStep<24)		// Make sure destination buffer can write at least 8 pixels in one row.
		return fwStsStepErr;

	switch( Dispatch::Type<DT_SSE2>() )
	{
	case DT_SSE3:
	case DT_SSE2:
		return iYCbCr444ToRGBLS_MCU_16s8u_P3C3R_SSE2( pSrcMCU, pDstRGB, dstStep );
	default:
		return iYCbCr444ToRGBLS_MCU_16s8u_P3C3R_REF( pSrcMCU, pDstRGB, dstStep );	
	}
}

static SYS_INLINE void fwiYCbCr4xxToRGBLS_MCU_16s8u_P3C3R_8x8Block (
	const Fw16s *pSrcMCU[3], Fw8u *pDstRGB, int dstStep, int blocknum, 
	int xshift, int yshift)
{
	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	unsigned char RVal, GVal, BVal;
	int ysrcPos, csrcPos, coffset, cPos, doffset, dstPos;

	ysrcPos = blocknum<<6;//*64
	coffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	doffset = ((blocknum&0x1)<<3) * 3 + ((blocknum&0x2)<<2)* dstStep;

	for (y=0; y<8; y++) {
		csrcPos = (y>>yshift)*8 + coffset;
		dstPos = y*dstStep + doffset;
		for (x=0;x<8;x++) {
			//add 128 for each element of YCC
			cPos = csrcPos + (x>>xshift);
			RVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] + 
				1.402*pSrcMCU[2][cPos] + 128.5);
			GVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] - 
				0.34414*pSrcMCU[1][cPos] - 0.71414*pSrcMCU[2][cPos]+ 128.5);
			BVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos++] + 
				1.772*pSrcMCU[1][cPos] + 128.5);
			pDstRGB[dstPos++] = RVal; 
			pDstRGB[dstPos++] = GVal; 
			pDstRGB[dstPos++] = BVal; 
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr422ToRGBLS_MCU_16s8u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw8u *pDstRGB, int dstStep)
{
	if (pDstRGB==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	// Y:			    	Cb:				Cr:					RGB:
	//		  ----8----	     ---4-----4---	---4-----4---	==>	  --8---8--
	//		8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|	==>	8 | A | B |
	//		8 |   By   |	-------------	-------------	==>	  ---------
	//		  ----------
	// Proces each block independently
	fwiYCbCr4xxToRGBLS_MCU_16s8u_P3C3R_8x8Block (pSrcMCU, pDstRGB, dstStep, 0, 1, 0);
	fwiYCbCr4xxToRGBLS_MCU_16s8u_P3C3R_8x8Block (pSrcMCU, pDstRGB, dstStep, 1, 1, 0);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Creates a YCbCr Image and then converts to BGR format
//-----------------------------------------------------------------------
static SYS_INLINE void fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block (
	const Fw16s *pSrcMCU[3], Fw8u *pDstBGR, int dstStep, int blocknum, 
	int xshift, int yshift)
{
	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	unsigned char RVal, GVal, BVal;
	int ysrcPos, csrcPos, coffset, cPos, doffset, dstPos;

	ysrcPos = blocknum<<6;//*64
	coffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	doffset = ((blocknum&0x1)<<3) * 3 + ((blocknum&0x2)<<2)* dstStep;

	for (y=0; y<8; y++) {
		csrcPos = (y>>yshift)*8 + coffset;
		dstPos = y*dstStep + doffset;
		for (x=0;x<8;x++) {
			//add 128 for each element of YCC
			cPos = csrcPos + (x>>xshift);
			RVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] + 
				1.402*pSrcMCU[2][cPos] + 128.5);
			GVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] - 
				0.34414*pSrcMCU[1][cPos] - 0.71414*pSrcMCU[2][cPos]+ 128.5);
			BVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos++] + 
				1.772*pSrcMCU[1][cPos] + 128.5);
			pDstBGR[dstPos++] = BVal; 
			pDstBGR[dstPos++] = GVal; 
			pDstBGR[dstPos++] = RVal; 
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr444ToBGRLS_MCU_16s8u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw8u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	// Y:			    	Cb:			Cr:				RGB:
	//		  ----8----	     ---8---	---8----	==>	  --8--
	//		8 |   Ay   |	8 |A_cb ||	8 |A_cr |	==>	8 | A |
	//		  ---------	    ---------	--------	==>	  -----

	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 0, 0);

	return fwStsNoErr;
}

static SYS_INLINE void fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block (
	const Fw16s *pSrcMCU[3], Fw16u *pDstBGR, int dstStep, int blocknum,
	int xshift, int yshift)
{
	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	unsigned short RVal, GVal, BVal;
	int ysrcPos, csrcPos, coffset, cPos, doffset, dstPos;
	double val;

	ysrcPos = blocknum<<6;//*64
	coffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	doffset = ((blocknum&0x1)<<3) + ((blocknum&0x2)<<2)* dstStep;

	for (y=0; y<8; y++) {
		csrcPos = (y>>yshift)*8 + coffset;
		dstPos = y*dstStep + doffset;
		for (x=0;x<8;x++) {
			//add 128 for each element of YCC
			cPos = csrcPos + (x>>xshift);
			val = (pSrcMCU[0][ysrcPos] + 1.402*pSrcMCU[2][cPos] + 128.5)/8;
			RVal = FWJ_LIMIT5(val) << 11;
			val = (pSrcMCU[0][ysrcPos] - 0.34414*pSrcMCU[1][cPos] - 
				0.71414*pSrcMCU[2][cPos]+ 128.5)/4;
			GVal = FWJ_LIMIT6(val) << 5;
			val = (pSrcMCU[0][ysrcPos] + 1.772*pSrcMCU[1][cPos] + 128.5)/8;
			BVal = FWJ_LIMIT5(val);

			ysrcPos++;
			pDstBGR[dstPos++]=RVal|GVal|BVal;
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr444ToBGR565LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);

	// Y:			    	Cb:			Cr:				RGB:
	//		  ----8----	     ---8---	---8----	==>	  --8--
	//		8 |   Ay   |	8 |A_cb ||	8 |A_cr |	==>	8 | A |
	//		  ---------	    ---------	--------	==>	  -----

	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 0, 0);

	return fwStsNoErr;
}


static SYS_INLINE void fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block (
	const Fw16s *pSrcMCU[3], Fw16u *pDstBGR, int dstStep, int blocknum,
	int xshift, int yshift)
{
	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	unsigned short RVal, GVal, BVal;
	int ysrcPos, csrcPos, coffset, cPos, doffset, dstPos;
	double val;

	ysrcPos = blocknum<<6;//*64
	coffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	doffset = ((blocknum&0x1)<<3) + ((blocknum&0x2)<<2)* dstStep;

	for (y=0; y<8; y++) {
		csrcPos = (y>>yshift)*8 + coffset;
		dstPos = y*dstStep + doffset;
		for (x=0;x<8;x++) {
			//add 128 for each element of YCC
			cPos = csrcPos + (x>>xshift);
			val = (pSrcMCU[0][ysrcPos] + 1.402*pSrcMCU[2][cPos] + 128.5)/8;
			RVal = FWJ_LIMIT5(val) << 10;
			val = (pSrcMCU[0][ysrcPos] - 0.34414*pSrcMCU[1][cPos] - 
				0.71414*pSrcMCU[2][cPos]+ 128.5)/8;
			GVal = FWJ_LIMIT5(val) << 5;
			val = (pSrcMCU[0][ysrcPos] + 1.772*pSrcMCU[1][cPos] + 128.5)/8;
			BVal = FWJ_LIMIT5(val);

			ysrcPos++;
			pDstBGR[dstPos++]=RVal|GVal|BVal;
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr444ToBGR555LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);

	// Y:			    	Cb:			Cr:				RGB:
	//		  ----8----	     ---8---	---8----	==>	  --8--
	//		8 |   Ay   |	8 |A_cb ||	8 |A_cr |	==>	8 | A |
	//		  ---------	    ---------	--------	==>	  -----

	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 0, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr422ToBGRLS_MCU_16s8u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw8u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	// Y:			    	Cb:				Cr:					RGB:
	//		  ----8----	     ---4-----4---	---4-----4---	==>	  --8---8--
	//		8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|	==>	8 | A | B |
	//		8 |   By   |	-------------	-------------	==>	  ---------
	//		  ----------
	// Proces each block independently
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 1, 0);
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 1, 1, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr422ToBGR565LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);

	// Y:			    	Cb:				Cr:					RGB:
	//		  ----8----	     ---4-----4---	---4-----4---	==>	  --8---8--
	//		8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|	==>	8 | A | B |
	//		8 |   By   |	-------------	-------------	==>	  ---------
	//		  ----------
	// Proces each block independently
	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 1, 0);
	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 1, 1, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr422ToBGR555LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);

	// Y:			    	Cb:				Cr:					RGB:
	//		  ----8----	     ---4-----4---	---4-----4---	==>	  --8---8--
	//		8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|	==>	8 | A | B |
	//		8 |   By   |	-------------	-------------	==>	  ---------
	//		  ----------
	// Proces each block independently
	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 0, 1, 0);
	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block (pSrcMCU, pDstBGR, dstStep, 1, 1, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr411ToBGRLS_MCU_16s8u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw8u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);
		
	// Y:			    	CbCr:								RGB:
	//		  ----8----		  ---4-----4---		  	==>			  --8---8--
	//		8 |   Ay   |		4 |A_c | B_c|		==>			8 | A | B |
	//		8 |   By   |		  -------------		==>			  ---------
	//		8 |   Cy   |		4 |C_c | D_c|		==>			8 | C | D |
	//		8 |   Dy   |		  -------------		==>			  ---------
	//		  ----------
	// Proces each block independently
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 0, 1, 1);
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 1, 1, 1);
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 2, 1, 1);
	fwiYCbCr4xxToBGRLS_MCU_16s8u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 3, 1, 1);


	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr411ToBGR565LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);		

	// Y:   				CbCr:								RGB:
	//		  ----8----		  ---4-----4---		  	==>			  --8---8--
	//		8 |   Ay   |		4 |A_c | B_c|		==>			8 | A | B |
	//		8 |   By   |		  -------------		==>			  ---------
	//		8 |   Cy   |		4 |C_c | D_c|		==>			8 | C | D |
	//		8 |   Dy   |		  -------------		==>			  ---------
	//		  ----------
	// Proces each block independently

	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 0, 1, 1);
	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 1, 1, 1);
	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 2, 1, 1);
	fwiYCbCr4xxToBGR565LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 3, 1, 1);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCbCr411ToBGR555LS_MCU_16s16u_P3C3R)(const Fw16s *pSrcMCU[3],
						Fw16u *pDstBGR, int dstStep)
{

	if (pDstBGR==0 || pSrcMCU==0) return fwStsNullPtrErr;
	if (pSrcMCU[0]==0 ||pSrcMCU[1]==0 ||pSrcMCU[2]==0)
		return fwStsNullPtrErr;
	STEPCHECK1(dstStep);

	//dstStep is byte size
	dstStep = dstStep / sizeof (Fw16u);

	// Y:    				CbCr:								RGB:
	//		  ----8----		  ---4-----4---		  	==>			  --8---8--
	//		8 |   Ay   |		4 |A_c | B_c|		==>			8 | A | B |
	//		8 |   By   |		  -------------		==>			  ---------
	//		8 |   Cy   |		4 |C_c | D_c|		==>			8 | C | D |
	//		8 |   Dy   |		  -------------		==>			  ---------
	//		  ----------
	// Proces each block independently

	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 0, 1, 1);
	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 1, 1, 1);
	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 2, 1, 1);
	fwiYCbCr4xxToBGR555LS_MCU_16s16u_P3C3R_8x8Block(pSrcMCU, pDstBGR, dstStep, 3, 1, 1);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Creates a YCCK Image and then converts to CMYK format
//-----------------------------------------------------------------------
static SYS_INLINE void fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (
	const Fw16s *pSrcMCU[4], Fw8u *pDstCMYK, int dstStep, int blocknum,
	int xshift, int yshift)
{
	//Reference code only.
	//SSE2 code need to shift 16 bit 
	int x, y;
	unsigned char RVal, GVal, BVal;
	int ysrcPos, csrcPos, coffset, cPos, doffset, dstPos;

	ysrcPos = blocknum<<6;//*64
	coffset = ((blocknum&0x1)<<2) + ((blocknum&0x2)<<4);
	doffset = ((blocknum&0x1)<<5) + ((blocknum&0x2)<<2)* dstStep;

	for (y=0; y<8; y++) {
		csrcPos = (y>>yshift)*8 + coffset;
		dstPos = y*dstStep + doffset;
		for (x=0;x<8;x++) {
			//add 128 for each element of YCCK
			cPos = csrcPos + (x>>xshift);
			RVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] + 
				1.402*pSrcMCU[2][cPos] + 128.5);
			GVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] - 
				0.34414*pSrcMCU[1][cPos] - 0.71414*pSrcMCU[2][cPos]+ 128.5);
			BVal = FW_REF::Limits<U8>::Sat(pSrcMCU[0][ysrcPos] + 
				1.772*pSrcMCU[1][cPos] + 128.5);
			pDstCMYK[dstPos++] = ~RVal; //C=255-R
			pDstCMYK[dstPos++] = ~GVal; //M=255-G
			pDstCMYK[dstPos++] = ~BVal; //Y=255-B
			pDstCMYK[dstPos++] = (Fw8u)(pSrcMCU[3][ysrcPos++]+128);
		}
	}

	return;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCCK444ToCMYKLS_MCU_16s8u_P4C4R)(const Fw16s *pSrcMCU[4],
			Fw8u *pDstCMYK, int dstStep)
{
	if (pSrcMCU == 0 || pDstCMYK == 0) return fwStsNullPtrErr;
	if (pSrcMCU[0] == 0 || pSrcMCU[1] == 0 || pSrcMCU[2] == 0 || 
		pSrcMCU[3] == 0)
		return fwStsNullPtrErr;

	STEPCHECK1(dstStep);

	// Y:			    	Cb:			Cr:				RGB:
	//		  ----8----	     ---8---	---8----	==>	  --8--
	//		8 |   Ay   |	8 |A_cb ||	8 |A_cr |	==>	8 | A |
	//		  ---------	    ---------	--------	==>	  -----
	
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 0, 0, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCCK422ToCMYKLS_MCU_16s8u_P4C4R)(const Fw16s *pSrcMCU[4],
			Fw8u *pDstCMYK, int dstStep)
{
	if (pSrcMCU == 0 || pDstCMYK == 0) return fwStsNullPtrErr;
	if (pSrcMCU[0] == 0 || pSrcMCU[1] == 0 || pSrcMCU[2] == 0 || 
		pSrcMCU[3] == 0)
		return fwStsNullPtrErr;

	STEPCHECK1(dstStep);

	// Y, K:				Cb:				Cr:					RGB:
	//		  ----8----	     ---4-----4---	---4-----4---	==>	  --8---8--
	//		8 |   Ay   |	8 |A_cb | B_cb|	8 |A_cr | B_cr|	==>	8 | A | B |
	//		8 |   By   |	-------------	-------------	==>	  ---------
	//		  ----------
	// Proces each block independently
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 0, 1, 0);
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 1, 1, 0);

	return fwStsNoErr;
}

FwStatus PREFIX_OPT(OPT_PREFIX, fwiYCCK411ToCMYKLS_MCU_16s8u_P4C4R)(const Fw16s *pSrcMCU[4],
			Fw8u *pDstCMYK, int dstStep)
{
	if (pSrcMCU == 0 || pDstCMYK == 0) return fwStsNullPtrErr;
	if (pSrcMCU[0] == 0 || pSrcMCU[1] == 0 || pSrcMCU[2] == 0 || 
		pSrcMCU[3] == 0)
		return fwStsNullPtrErr;

	STEPCHECK1(dstStep);
		
	// Y, K:				CbCr:								RGB:
	//		  ----8----		  ---4-----4---		  	==>			  --8---8--
	//		8 |   Ay   |		4 |A_c | B_c|		==>			8 | A | B |
	//		8 |   By   |		  -------------		==>			  ---------
	//		8 |   Cy   |		4 |C_c | D_c|		==>			8 | C | D |
	//		8 |   Dy   |		  -------------		==>			  ---------
	//		  ----------
	// Proces each block independently
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 0, 1, 1);
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 1, 1, 1);
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 2, 1, 1);
	fwiYCCK4xxToCMYKLS_MCU_16s8u_P4C4R_8x8Block (pSrcMCU, pDstCMYK, dstStep, 3, 1, 1);

	return fwStsNoErr;
}

#endif //BUILD_NUM_AT_LEAST

// Please do NOT remove the above line for CPP files that need to be multipass compiled
// OREFR OSSE2 
