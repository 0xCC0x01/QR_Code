/* Reference (PNG Specification):
 * 1. http://www.w3.org/TR/PNG
 * 2. https://www.ietf.org/rfc/rfc1950.txt
 * 3. https://www.ietf.org/rfc/rfc1951.txt
 */

#ifndef _PNG_H_
#define _PNG_H_

#include <vector>
using std::vector;

typedef enum
{
    BIT_DEPTH_1 = 1,
    BIT_DEPTH_2 = 2,
    BIT_DEPTH_4 = 4,
    BIT_DEPTH_8 = 8,
    BIT_DEPTH_16 = 16
}BIT_DEPTH;

typedef enum
{
    /* Greyscale */
    GREYSCALE = 0,
    /* Truecolor */
    TRUECOLOR = 2,
    /* Indexed-color */
    INDEXED_COLOR = 3,
    /* Greyscale with alpha */
    ALPHA_GREYSCALE = 4,
    /* Truecolor with alpha */
    ALPHA_TRUECOLOR = 6
}COLOR_TYPE;

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
}PIXEL;

/* Non-compressed blocks (BTYPE=00) */
typedef struct
{
    /* Each block of compressed data begins with 3 header bits
     * bit 0: BFINAL (set if and only if this is the last block of the data set)
     * bit 1~2: BTYPE (set to 00 if no compression) */
    unsigned char header;
    /* LEN is the number of data bytes in the block */
    unsigned short LEN;
    /* NLEN is the complement of LEN */
    unsigned short NLEN;
}NCB;


class Chunk
{
public:
    /* 4-byte chunk length
     * Counts only chunk data field, not including itself, chunk type code or the CRC. */
    unsigned long length;
    /* 4-byte chunk type code. */
    unsigned long type;
    /* 4-byte CRC calculated on the preceding bytes in the chunk
     * including the chunk type code and chunk data fields, but not including the length field.
     * CRC is always present, even for chunks containing no data, eg: IEND. */
    unsigned long CRC;
};

/* Image Header Chunk
 * contains basic image info, MUST APPEAR FIRST. */
class PNG_IHDR : public Chunk
{
public:
    unsigned long width;
    unsigned long height;
    unsigned char bit_depth;
    unsigned char color_type;
    unsigned char compress_method;
    unsigned char filter_method;
    unsigned char interlace_method;

    PNG_IHDR();
};

/* Image Data Chunk
 * contains the actual image data. */
class PNG_IDAT : public Chunk
{
public:
    /* Compression Method and flags
     * bits 0~3: CM = 8 (Compression method)
     * bits 4~7: CINFO = 7 indicates a 32K window size (Compression info) */
    unsigned char CMF;
    /* Flags
     * bits 0~4: FCHECK (check bits for CMF and FLG)
     * bits 5:   FDICT (preset dictionary) 
     * bits 6~7: FLEVEL (compression level) */
    unsigned char FLG;
    /* image data block */
    union
    {
        unsigned char *idata;
        PIXEL *pdata;
    };
    /* 4-byte Adler32 check value calculated on the uncompressed data */
    unsigned long ADLER32;

    PNG_IDAT();
};

/* Palette Chunk
 * contains from 1 to 256 palette entries, each a 3-byte data (1-byte red, 1-byte green, 1-byte blue) */
class PNG_PLTE : public Chunk
{
public:
    vector<PIXEL> palette;

    PNG_PLTE();
};

/* Image Trailer Chunk
 * contains no data, marks the end of the png datastream. MUST APPEAR LAST. */
class PNG_IEND : public Chunk
{
public:
    PNG_IEND();
};


/* A valid PNG image must contain an IHDR chunk, one or more IDAT chunks, and an IEND chunk. */
class PNG
{
private:
    PNG_IHDR IHDR;
    PNG_PLTE PLTE;
    PNG_IDAT IDAT;
    PNG_IEND IEND;

public:
    /* Set IHDR: png image width, height, bit depth and color type */
    bool set_ihdr(unsigned long width, unsigned long height,
                  BIT_DEPTH bit_depth = BIT_DEPTH_4, COLOR_TYPE color_type = INDEXED_COLOR);

    /* Set PLTE */
    bool set_plte(PIXEL *pixels, unsigned int count);

    /* Set IDAT: pixel color data */
    bool set_idat(PIXEL *data);

    /* Set IDAT: indexed color data, for INDEXED_COLOR */
    bool set_idat(unsigned char *data);

    /* Save .png file to the path */
    bool write(char *path);
};

#endif /* _PNG_H_ */