/*
Copyright (c) 2006-2009 Advanced Micro Devices, Inc. All Rights Reserved.
This software is subject to the Apache v2.0 License.
*/

/*#DataBlock

#Parameters

pSrc - Pointer to a source buffer.
pSrc1 - Pointer to source buffer one.
pSrc2 - Pointer to source buffer two.
pDst - Pointer to a destination buffer.
pDst1 - Pointer to destination buffer one.
pDst2 - Pointer to destination buffer two.
len - Specifies the number of elements in a buffer.
pDp - Pointer to the destination buffer that contains the dot product of the source buffers.
pIndx - Pointer to the destination buffer that contains the index of the maximum or minimum value in the source buffer.
pMax - Pointer to the destination buffer that contains the maximum value in the source buffer.
pMaxAbs - Pointer to the destination buffer that contains the maximum absolute value in the source buffer.
pMaxIndx - Pointer to the destination buffer that contains the index of the maximum value in the source vector.
pMean - Pointer to the destination buffer that contains the mean of the values in the source buffer.
pMin - Pointer to the destination buffer that contains the minimum value in the source buffer.
pMinAbs - Pointer to the destination buffer that contains the minimum absolute value in the source buffer.
pMinIndx - Pointer to the destination buffer that contains the index of the minimum value in the source buffer.
pNorm - Pointer to the destination buffer that contains the NormC, NormL1, or NormL2 of the values in the source buffer, depending on the function call. 
pStdDev - Pointer to the destination buffer that contains the standard deviation of the values in the source buffer.
pSum - Pointer to the destination buffer that contains the sum of all the elements in the source buffer.
hint - Hints whether to choose a fast or accurate computation algorithm.
level - Specifies a threshold level.
levelGT - Specifies a threshold level for greater-than comparison.
levelLT - Specifies a threshold level for less-than comparison.
pSrcDst - Pointer to a buffer that is both the source and destination.
pSrcRe - Pointer to a source buffer that contains the real component values of an input signal.
pSrcIm - Pointer to a source buffer that contains the imaginary component values of an input signal.
relOp - Constant that indicates which relational operation to perform.
rndMode - Constant that indicates which rounding mode to use when converting from a scalar to an integer.
scaleFactor - Specifies an integer scaling factor for the Sfs operation. The returned result is multiplied by 2^(-scaleFactor).
val - Specified value.
value - Specified value.
valueGT - Specifies a value to set when a greater-than comparison evaluates as true.
valueLT - Specifies a value to set when a less-than comparison evaluates as true.

vdiv - Specifies the denominator.
vsub - Specifies the subtrahend.
lenFind - Length of the substring.
pFind - Pointer to the substring.
pIndex - Pointer to the index of the result.
valFind - Value of the element to be found.
dstLen - Specifies the number of elements in destination buffer(this will be the length of auto-correlation)
srcLen - Specifies the number of elements in source buffer


*/

/*#Documentation

<SectionHeading>Signal Processing Library</SectionHeading>

<Paragraph>Signal Processing<SectDesc> functions perform a variety of tasks related to signal processing.</SectDesc></Paragraph>

<Paragraph>This section is organized as follows.</Paragraph>

<SectLinks></SectLinks>

<Paragraph>Within the section, the <Bold>Basic Concepts</Bold> chapter provides overview information related to the functions in the library, 
and subsequent chapters provide detailed descriptions of library functions that perform operations of the same kind.</Paragraph>
<Paragraph>Each detailed description consists of a <Bold>Function Name</Bold> followed by a short description, a <Bold>Synopsis</Bold> of the function syntax, 
a list of function <Bold>Parameters</Bold>, a detailed <Bold>Description</Bold> of the function, and a list of <Bold>Return Values.</Bold></Paragraph> 
<Paragraph>When a function is optimized for one or more technologies, a list of <Bold>Supported Technologies</Bold> is provided after the short description.</Paragraph>

<RefLink></RefLink>

<Chapter>
<ChapterHeading>Basic Concepts</ChapterHeading>

<Paragraph>This <ChaptDesc> chapter provides an overview of the information contained in the functional descriptions.</ChaptDesc></Paragraph>

<Paragraph>Framewave functional descriptions include the following types of information.</Paragraph>

<Heading2>
<H2Name>Parameter Glossary</H2Name>
<H2Link>signal_parameters</H2Link>
</Heading2>

<Paragraph>Signal processing library function definitions use the following parameters.</Paragraph>

#PrintAllParameters

</Chapter>

*/
