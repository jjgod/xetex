/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2008 by SIL International
 copyright (c) 2009 by Jonathan Kew

 Written by Jonathan Kew

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the copyright holders
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written
authorization from the copyright holders.
\****************************************************************************/

/*
 *   file name:  XeTeXFontInst_Mac.cpp
 *
 *   created on: 2005-10-22
 *   created by: Jonathan Kew
 */


#include "XeTeXFontInst_Mac.h"
#include "XeTeX_ext.h"

XeTeXFontInst_Mac::XeTeXFontInst_Mac(CTFontDescriptorRef descriptor, float pointSize, LEErrorCode &status)
    : XeTeXFontInst(pointSize, status)
    , fDescriptor(descriptor)
    , fFontRef(0)
{
    if (LE_FAILURE(status)) {
        return;
    }

	initialize(status);
}

XeTeXFontInst_Mac::~XeTeXFontInst_Mac()
{
	if (fFontRef != 0)
		CFRelease(fFontRef);
}

void XeTeXFontInst_Mac::initialize(LEErrorCode &status)
{
    if (fDescriptor == 0) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

	XeTeXFontInst::initialize(status);

	if (status != LE_NO_ERROR)
		fDescriptor = 0;

    fFontRef = CTFontCreateWithFontDescriptor(fDescriptor, fPointSize * 72.0 / 72.27, NULL);
	if (!fFontRef) {
		status = LE_FONT_FILE_NOT_FOUND_ERROR;
		fDescriptor = 0;
	}
	
    return;
}

const void *XeTeXFontInst_Mac::readTable(LETag tag, le_uint32 *length) const
{
    CFDataRef tableData = CTFontCopyTable(fFontRef, tag, 0);
	if (!tableData) {
		*length = 0;
		return NULL;
	}
    *length = CFDataGetLength(tableData);
	UInt8* table = LE_NEW_ARRAY(UInt8, *length);
	if (table != NULL)
        CFDataGetBytes(tableData, CFRangeMake(0, *length), table);

    return table;
}

void XeTeXFontInst_Mac::getGlyphBounds(LEGlyphID gid, GlyphBBox* bbox)
{
    bbox->xMin = 65536.0;
    bbox->yMin = 65536.0;
    bbox->xMax = -65536.0;
    bbox->yMax = -65536.0;

    CGRect rect = CTFontGetBoundingRectsForGlyphs(fFontRef,
        0, /* Use default orientation for now, handle vertical later */
        (const CGGlyph *) &gid, NULL, 1);

    if (CGRectIsNull(rect))
        bbox->xMin = bbox->yMin = bbox->xMax = bbox->yMax = 0;
    else {
        // convert PS to TeX points and flip y-axis
        // TODO: do we need to flip for bounding rect returned by Core Text?
        bbox->yMin = -(rect.origin.y + rect.size.height) * 72.27 / 72.0;
        bbox->yMax = -(rect.origin.y) * 72.27 / 72.0;
        bbox->xMin = rect.origin.x * 72.27 / 72.0;
        bbox->xMax = (rect.origin.x + rect.size.width) * 72.27 / 72.0;
    }
}

LEGlyphID
XeTeXFontInst_Mac::mapGlyphToIndex(const char* glyphName) const
{
	LEGlyphID rval = XeTeXFontInst::mapGlyphToIndex(glyphName);
	if (rval)
		return rval;
	return GetGlyphIDFromCTFont(fFontRef, glyphName);
}

const char*
XeTeXFontInst_Mac::getGlyphName(LEGlyphID gid, int& nameLen)
{
	const char* rval = XeTeXFontInst::getGlyphName(gid, nameLen);
	if (rval)
		return rval;
	return GetGlyphNameFromCTFont(fFontRef, gid, &nameLen);
}
