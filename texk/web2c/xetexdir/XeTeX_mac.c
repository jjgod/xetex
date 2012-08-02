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

/* XeTeX_mac.c
 * additional plain C extensions for XeTeX - MacOS-specific routines
 */

#ifdef __POWERPC__
#define MAC_OS_X_VERSION_MIN_REQUIRED	1030
#else
#define MAC_OS_X_VERSION_MIN_REQUIRED	1040
#endif

#define EXTERN extern
#include "xetexd.h"

#undef input /* this is defined in texmfmp.h, but we don't need it and it confuses the carbon headers */
#include <Carbon/Carbon.h>

#include <teckit/TECkit_Engine.h>
#include "XeTeX_ext.h"
#include "XeTeXLayoutInterface.h"

#include "XeTeXswap.h"

int GetGlyphIDFromCTFont(CTFontRef ctFontRef, const char* glyphName)
{
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
	if (&CGFontGetGlyphWithGlyphName == NULL)
		return 0;

	int rval = 0;
    CGFontRef cgfont = CTFontCopyGraphicsFont(ctFontRef, 0);
    if (cgfont) {
        CFStringRef glyphname = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                                                glyphName,
                                                                kCFStringEncodingUTF8,
                                                                kCFAllocatorNull);
        rval = CGFontGetGlyphWithGlyphName(cgfont, glyphname);
        CFRelease(glyphname);
        CGFontRelease(cgfont);
    }
	return rval;
#else
    return 0;
#endif
}

char*
GetGlyphNameFromCTFont(CTFontRef ctFontRef, UInt16 gid, int* len)
{
	*len = 0;
	static char buffer[256];
	buffer[0] = 0;

#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
	if (&CGFontCopyGlyphNameForGlyph == NULL)
		return &buffer[0];

    CGFontRef cgfont = CTFontCopyGraphicsFont(ctFontRef, 0);
    if (cgfont && gid < CGFontGetNumberOfGlyphs(cgfont)) {
        CFStringRef glyphname = CGFontCopyGlyphNameForGlyph(cgfont, gid);
        if (glyphname) {
            if (CFStringGetCString(glyphname, buffer, 256, kCFStringEncodingUTF8)) {
                *len = strlen(buffer);
            }
            CFRelease(glyphname);
        }
        CGFontRelease(cgfont);
    }
#endif

	return &buffer[0];
}

int
countpdffilepages()
{
	int	rval = 0;

    char*		pic_path = kpse_find_file((char*)nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;
	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)pic_path, strlen(pic_path), false);
		if (picFileURL != NULL) {
			FSRef	picFileRef;
			CFURLGetFSRef(picFileURL, &picFileRef);
			CGPDFDocumentRef	document = CGPDFDocumentCreateWithURL(picFileURL);
			if (document != NULL) {
				rval = CGPDFDocumentGetNumberOfPages(document);
				CGPDFDocumentRelease(document);
			}
			CFRelease(picFileURL);
		}
		free(pic_path);
	}

	return rval;
}

int
find_pic_file(char** path, realrect* bounds, int pdfBoxType, int page)
	/* returns bounds in TeX points */
{
	*path = NULL;

	OSStatus	result = fnfErr;
    char*		pic_path = kpse_find_file((char*)nameoffile + 1, kpse_pict_format, 1);
	CFURLRef	picFileURL = NULL;
	if (pic_path) {
		picFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)pic_path, strlen(pic_path), false);

		if (picFileURL != NULL) {
			if (pdfBoxType > 0) {
				CGPDFDocumentRef	document = CGPDFDocumentCreateWithURL(picFileURL);
				if (document != NULL) {
					int	nPages = CGPDFDocumentGetNumberOfPages(document);
					if (page < 0)
						page = nPages + 1 + page;
					if (page < 1)
						page = 1;
					if (page > nPages)
						page = nPages;

					CGRect	r;
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
					if (&CGPDFDocumentGetPage == NULL) {
						switch (pdfBoxType) {
							case pdfbox_crop:
							default:
								r = CGPDFDocumentGetCropBox(document, page);
								break;
							case pdfbox_media:
								r = CGPDFDocumentGetMediaBox(document, page);
								break;
							case pdfbox_bleed:
								r = CGPDFDocumentGetBleedBox(document, page);
								break;
							case pdfbox_trim:
								r = CGPDFDocumentGetTrimBox(document, page);
								break;
							case pdfbox_art:
								r = CGPDFDocumentGetArtBox(document, page);
								break;
						}
					}
					else
#endif
					{
						CGPDFPageRef	pageRef = CGPDFDocumentGetPage(document, page);
						CGPDFBox	boxType;
						switch (pdfBoxType) {
							case pdfbox_crop:
							default:
								boxType = kCGPDFCropBox;
								break;
							case pdfbox_media:
								boxType = kCGPDFMediaBox;
								break;
							case pdfbox_bleed:
								boxType = kCGPDFBleedBox;
								break;
							case pdfbox_trim:
								boxType = kCGPDFTrimBox;
								break;
							case pdfbox_art:
								boxType = kCGPDFArtBox;
								break;
						}
						r = CGPDFPageGetBoxRect(pageRef, boxType);
					}

					bounds->x = r.origin.x * 72.27 / 72.0;
					bounds->y = r.origin.y * 72.27 / 72.0;
					bounds->wd = r.size.width * 72.27 / 72.0;
					bounds->ht = r.size.height * 72.27 / 72.0;
					CGPDFDocumentRelease(document);
					result = noErr;
				}
			}
			else {
                CGImageSourceRef picFileSource = CGImageSourceCreateWithURL(picFileURL, NULL);
                if (picFileSource) {
                    CFDictionaryRef properties = CGImageSourceCopyPropertiesAtIndex(picFileSource, 0, NULL);

                    CFNumberRef imageWidth  = CFDictionaryGetValue(properties, kCGImagePropertyPixelWidth);
                    CFNumberRef imageHeight = CFDictionaryGetValue(properties, kCGImagePropertyPixelHeight);
                    CFNumberRef DPIWidth    = CFDictionaryGetValue(properties, kCGImagePropertyDPIWidth);
                    CFNumberRef DPIHeight   = CFDictionaryGetValue(properties, kCGImagePropertyDPIHeight);
                    int w = 0, h = 0, hRes = 72, vRes = 72;
                    CFNumberGetValue(imageWidth,  kCFNumberIntType, &w);
                    CFNumberGetValue(imageHeight, kCFNumberIntType, &h);
                    CFNumberGetValue(DPIWidth,    kCFNumberIntType, &hRes);
                    CFNumberGetValue(DPIHeight,   kCFNumberIntType, &vRes);

                    bounds->x = 0;
                    bounds->y = 0;
                    bounds->wd = w * 72.27 / hRes;
                    bounds->ht = h * 72.27 / vRes;
                    CFRelease(properties);
                    CFRelease(picFileSource);
                }
			}
			
			CFRelease(picFileURL);
		}

		/* if we couldn't import it, toss the pathname as we'll be returning an error */
		if (result != noErr)
			free(pic_path);
		else
			*path = (char*)pic_path;
	}
	
	return result;
}
