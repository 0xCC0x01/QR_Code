#include "png.h"
#include "util.h"
#include <cstring>
#include <fstream>
using namespace std;

#define PNG_FILE_SIGNATURE "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"

#define CHUNK_TYPE_IHDR 0x49484452
#define CHUNK_TYPE_PLTE 0x504C5445
#define CHUNK_TYPE_IDAT 0x49444154
#define CHUNK_TYPE_IEND 0x49454e44


#define MIN(a, b) ((a) <= (b) ? (a) : (b))

static union {char c[4]; unsigned long x;} endian_test = {{'l', '?', '?', 'b'}};
#define ENDIANNESS ((char)endian_test.x)


/* Converts the unsigned integer from host byte order to network byte order. */
unsigned long H2NL(unsigned long value)
{
    /* Byte order: little endian */
    if ('l' == ENDIANNESS)
    {
        unsigned char *s = (unsigned char *)&value;
        return (unsigned long)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
    }

    return value;
}

template <typename T>
static void CONCAT(char *&buff, T value, int len = 0)
{
    if (len <= 0)
    {
        memcpy(buff, &value, sizeof(value));
        buff += sizeof(value);
    }
    else
    {
        memcpy(buff, &value, len);
        buff += len;
    }    
}


PNG_IHDR::PNG_IHDR()
{
    length = 0x0D;
    type = CHUNK_TYPE_IHDR;
    width = 0;
    height = 0;
    bit_depth = BIT_DEPTH_4;
    color_type = INDEXED_COLOR;
    /* Only compression method 0 (deflate/inflate compression) is defined */
    compress_method = 0;
    /* Only filter method 0 (adaptive filtering with five basic filter types) is defined */
    filter_method = 0;
    /* Two values are defined: 0 (no interlace) or 1 (Adam7 interlace) */
    interlace_method = 0;
}

PNG_IDAT::PNG_IDAT()
{
    length = 0;
    type = CHUNK_TYPE_IDAT;

    CMF = 0x78;

    /* (CMF*256 + FLG) is multiple of 31 */
    int FDICT, FLEVEL;

    FDICT = 0;
    FLEVEL = 3;
    /* FCHECK = ((CMF*256 + (FLEVEL<<6) + (FDICT<<5) + 30)/31)*31 - CMF*256 - (FLEVEL<<6);
     * FLG = (FLEVEL<<6) + (FDICT<<5) + FCHECK; */
    FLG = ((CMF*256 + (FLEVEL<<6) + (FDICT<<5) + 30)/31)*31 - CMF*256 + (FDICT<<5);

    idata = NULL;
    pdata = NULL;
}

PNG_PLTE::PNG_PLTE()
{
    length = 0;
    type = CHUNK_TYPE_PLTE;
}

PNG_IEND::PNG_IEND()
{
    length = 0;
    type = CHUNK_TYPE_IEND;
    CRC = 0xAE426082;
}


int pixel_size(unsigned char color_type)
{
    int bytes = 0;

    switch(color_type)
    {
        case GREYSCALE:
        case INDEXED_COLOR:
            bytes = 1;
            break;

        case ALPHA_GREYSCALE:
            bytes = 2;
            break;

        case TRUECOLOR:
            bytes = 3;
            break;

        case ALPHA_TRUECOLOR:
            bytes = 4;
            break;
    }
    return bytes;
}

bool PNG::set_ihdr(unsigned long width, unsigned long height, BIT_DEPTH bit_depth, COLOR_TYPE color_type)
{
    if (width == 0 || height == 0)
    {
        return false;
    }
    
    /* Indexed-color: allowed bit depths: 1/2/4/8 */
    if ((INDEXED_COLOR == color_type) && (BIT_DEPTH_16 == bit_depth))
    {
        return false;
    }

    /* Truecolor, Greyscale with alpha, Truecolor with alpha: allowed bit depths: 8/16 */
    if ((TRUECOLOR == color_type || ALPHA_GREYSCALE == color_type || ALPHA_TRUECOLOR == color_type)
        && (BIT_DEPTH_1 == bit_depth || BIT_DEPTH_2 == bit_depth || BIT_DEPTH_4 == bit_depth))
    {
        return false;
    }

    IHDR.width = width;
    IHDR.height = height;
    IHDR.bit_depth = bit_depth;
    IHDR.color_type = color_type;

    return true;
}

bool PNG::set_plte(PIXEL *pixels, unsigned int count)
{
    if (NULL != pixels && count <= (1U << IHDR.bit_depth))
    {
        /* Each a 3-byte series of the form: R/G/B */
        PLTE.length = count*3;

        for (unsigned int i = 0; i < count; i++)
        {
            PLTE.palette.push_back(pixels[i]);
        }

        return true;
    }

    return false;
}

bool PNG::set_idat(PIXEL *data)
{
    if (NULL != data)
    {
        IDAT.pdata = data;
        return true;
    }

    return false;
}

bool PNG::set_idat(unsigned char *data)
{
    if (NULL != data && INDEXED_COLOR == IHDR.color_type)
    {
        IDAT.idata = data;
        return true;
    }

    return false;
}

void write_ihdr(ofstream &file, PNG_IHDR &IHDR)
{
    int ihdr_len = IHDR.length + sizeof(Chunk);
    char *ihdr = new char[ihdr_len];
    char *buff = ihdr;

    CONCAT(buff, H2NL(IHDR.length));
    CONCAT(buff, H2NL(IHDR.type));
    CONCAT(buff, H2NL(IHDR.width));
    CONCAT(buff, H2NL(IHDR.height));
    CONCAT(buff, IHDR.bit_depth);
    CONCAT(buff, IHDR.color_type);
    CONCAT(buff, IHDR.compress_method);
    CONCAT(buff, IHDR.filter_method);
    CONCAT(buff, IHDR.interlace_method);

    IHDR.CRC = CRC32(ihdr + 4, IHDR.length + 4);
    CONCAT(buff, H2NL(IHDR.CRC));

    file.write(ihdr, ihdr_len);
    delete [] ihdr;
}

void write_plte(ofstream &file, PNG_PLTE &PLTE)
{
    if (PLTE.length)
    {
        int plte_len = PLTE.length + sizeof(Chunk);
        char *plte = new char[plte_len];
        char *buff = plte;

        CONCAT(buff, H2NL(PLTE.length));
        CONCAT(buff, H2NL(PLTE.type));
        for (unsigned int i = 0; i < PLTE.palette.size(); i++)
        {
            CONCAT(buff, PLTE.palette[i].red);
            CONCAT(buff, PLTE.palette[i].green);
            CONCAT(buff, PLTE.palette[i].blue);
        }

        PLTE.CRC = CRC32(plte + 4, PLTE.length + 4);
        CONCAT(buff, H2NL(PLTE.CRC));

        file.write(plte, plte_len);
        delete [] plte;
    }
}

void write_idat(ofstream &file, PNG_IHDR &IHDR, PNG_IDAT &IDAT)
{
    int size = pixel_size(IHDR.color_type);
    unsigned short len = IHDR.height*((IHDR.width*IHDR.bit_depth + 7)/8*size + 1);

    IDAT.length = 7 + len + 4;
    int idat_len = IDAT.length + sizeof(Chunk);
    char *idat = new char[idat_len];
    char *buff = idat;

    CONCAT(buff, H2NL(IDAT.length));
    CONCAT(buff, H2NL(IDAT.type));

    CONCAT(buff, IDAT.CMF);
    CONCAT(buff, IDAT.FLG);

    NCB ucb;
    ucb.header = 0x01;
    ucb.LEN = len;
    ucb.NLEN = len ^ 0xFFFF;

    CONCAT(buff, ucb.header);
    CONCAT(buff, ucb.LEN);
    CONCAT(buff, ucb.NLEN);
    
    unsigned int groups = MIN(IHDR.width, (IHDR.width*IHDR.bit_depth + 7)/8);
    unsigned int each_group = MIN(IHDR.width, 8/IHDR.bit_depth);

    each_group = (each_group < 1) ? 1 : each_group;

    for (unsigned int h = 0; h < IHDR.height; h++)
    {
        CONCAT(buff, 0x00, 1);

        if (INDEXED_COLOR == IHDR.color_type)
        {
            for (unsigned int w = 0; w < groups; w++)
            {
                unsigned char value = 0;
                
                for (unsigned int i = 0; i < each_group; i++)
                {
                    if (w*each_group + i < IHDR.width)
                    {
                        value |= IDAT.idata[IHDR.width*h + w*each_group + i] << (8 - IHDR.bit_depth*(i + 1));
                    }       
                }
                CONCAT(buff, value);
            }
        }
        else if (GREYSCALE == IHDR.color_type || ALPHA_GREYSCALE == IHDR.color_type)
        {
            for (unsigned int w = 0; w < groups; w++)
            {
                PIXEL pixel;
                unsigned char value = 0;
                
                for (unsigned int i = 0; i < each_group; i++)
                {
                    if (w*each_group + i < IHDR.width)
                    {
                        pixel = IDAT.pdata[IHDR.width*h + w*each_group + i];

                        /* Gray = R*0.299 + G*0.587 + B*0.114 */
                        unsigned char grey = (pixel.red*19595 + pixel.green*38469 + pixel.blue*7472) >> 16;

                        if (IHDR.bit_depth < 8)
                        {
                            grey = ((1 << IHDR.bit_depth) - 1)*grey/255;
                            value |= grey << (8 - IHDR.bit_depth*(i + 1));
                        }
                        else
                        {
                            value = grey;
                        }
                    }
                }
                CONCAT(buff, value, IHDR.bit_depth/8);

                if (ALPHA_GREYSCALE == IHDR.color_type)
                {
                    CONCAT(buff, pixel.alpha, IHDR.bit_depth/8);
                }
            }
        }
        else if (TRUECOLOR == IHDR.color_type || ALPHA_TRUECOLOR == IHDR.color_type)
        {
            for (unsigned int w = 0; w < IHDR.width; w++)
            {
                CONCAT(buff, IDAT.pdata[IHDR.width*h + w].red, IHDR.bit_depth/8);
                CONCAT(buff, IDAT.pdata[IHDR.width*h + w].green, IHDR.bit_depth/8);
                CONCAT(buff, IDAT.pdata[IHDR.width*h + w].blue, IHDR.bit_depth/8);
                if (ALPHA_TRUECOLOR == IHDR.color_type)
                {
                    CONCAT(buff, IDAT.pdata[IHDR.width*h + w].alpha, IHDR.bit_depth/8);
                }
            }
        }
    }

    IDAT.ADLER32 = Adler32((unsigned char *)idat + 15, IDAT.length - 11);
    CONCAT(buff, H2NL(IDAT.ADLER32));

    IDAT.CRC = CRC32(idat + 4, IDAT.length + 4);
    CONCAT(buff, H2NL(IDAT.CRC));

    file.write(idat, idat_len);
    delete [] idat;
}

void write_iend(ofstream &file, PNG_IEND png_iend)
{
    int iend_len = png_iend.length + sizeof(Chunk);
    char *iend = new char[iend_len];
    char *buff = iend;

    CONCAT(buff, H2NL(png_iend.length));
    CONCAT(buff, H2NL(png_iend.type));
    CONCAT(buff, H2NL(png_iend.CRC));

    file.write(iend, iend_len);
    delete [] iend;
}

bool PNG::write(char *path)
{
    ofstream png_file(path, ios::out | ios::binary);
    if (png_file.is_open())
    {
        /* png file signature */
        png_file.write(PNG_FILE_SIGNATURE, 8);

        /* IHDR */
        write_ihdr(png_file, IHDR);

        /* PLTE */
        write_plte(png_file, PLTE);

        /* IDAT */
        write_idat(png_file, IHDR, IDAT);

        /* IEND */
        write_iend(png_file, IEND);

        png_file.close();

        return true;
    }

    return false;
}