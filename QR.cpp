#include "QR.h"
#include "png/png.h"
#include <stdlib.h>
#include <string.h>

/* Min QR version: 1 */
#define QR_MIN_VERSION           (1)
/* Max QR version: 40 */
#define QR_MAX_VERSION           (40)
/* Version 1 size: 21*21 */
#define QR_MIN_SIZE              (21)
/* Total QR error correction levels */
#define QR_EC_LEVEL              (4)

/* Finder pattern size: 7*7 */
#define FINDER_PATTERN_SIZE      (7)
/* Alignment pattern size: 5*5 */
#define ALIGN_PATTERN_SIZE       (5)
/* Total alphanumeric characters */
#define ALPHA_NUMERIC_COUNT      (45)

/* Module data:
 * 1. Not set: 0x80 (light only)
 * 2. Function pattern:  0x10 (light); 0x11 (dark)
 * 3. Data pattern DARK: 0x00 (light); 0x01 (dark) */
#define MODULE_NOT_SET           (0x80)
#define MODULE_FUNCION_PATTERN   (0x10)


typedef struct
{
    int blocks;
    int block_bytes;
    int block_data_bytes;
}EC_INFO;

typedef struct
{
    int total_bytes;
    int data_bytes[QR_EC_LEVEL];
    int align_coords;
    int align_coord[7];
    EC_INFO ec_info[2][QR_EC_LEVEL];
}QR_INFO;

static const QR_INFO QR_info[] =
{
    { /* Version 1 */
        26,  19,   16,   13,    9,
         0,  0,  0,  0,  0,  0,  0,  0,
         1,  26,  19,  1,  26,  16,  1,  26,  13,  1,  26,   9,
         0,   0,   0,  0,   0,   0,  0,   0,   0,  0,   0,   0
    },
    { /* Version 2 */
        44,  34,   28,   22,   16,
         2,  6,  18,  0,  0,  0,  0,  0,
         1,  44,  34,  1,  44,  28,  1,  44,  22,  1,  44,  16,
         0,   0,   0,  0,   0,   0,  0,   0,   0,  0,   0,   0
    },
    { /* Version 3 */
        70,  55,   44,   34,   26,
         2,  6,  22,  0,  0,  0,  0,  0,
         1,  70,  55,  1,  70,  44,  2,  35,  17,  2,  35,  13,
         0,   0,   0,  0,   0,   0,  0,   0,   0,  0,   0,   0
    },
    { /* Version 4 */
        100,  80,   64,   48,   36,
         2,  6,  26,  0,  0,  0,  0,  0,
         1, 100,  80,  2,  50,  32,  2,  50,  24,  4,  25,   9,
         0,   0,   0,  0,   0,   0,  0,   0,   0,  0,   0,   0
    },
    { /* Version 5 */
        134,  108,   86,   62,   46,
         2,  6,  30,  0,  0,  0,  0,  0,
         1, 134, 108,  2,  67,  43,  2,  33,  15,  2,  33,  11,
         0,   0,   0,  0,   0,   0,  2,  34,  16,  2,  34,  12
    },
    { /* Version 6 */
        172,  136,  108,   76,   60,
         2,  6,  34,  0,  0,  0,  0,  0,
         2,  86,  68,  4,  43,  27,  4,  43,  19,  4,  43,  15,
         0,   0,   0,  0,   0,   0,  0,   0,   0,  0,   0,   0
    },
    { /* Version 7 */
        196,  156,  124,   88,   66,
         3,  6,  22,  38,  0,  0,  0,  0,
         2,  98,  78,  4,  49,  31,  2,  32,  14,  4,  39,  13,
         0,   0,   0,  0,   0,   0,  4,  33,  15,  1,  40,  14
    },
    { /* Version 8 */
        242,  194,  154,  110,   86,
         3,  6,  24,  42,  0,  0,  0,  0,
         2, 121,  97,  2,  60,  38,  4,  40,  18,  4,  40,  14,
         0,   0,   0,  2,  61,  39,  2,  41,  19,  2,  41,  15
    },
    { /* Version 9 */
        292,  232,  182,  132,  100,
         3,  6,  26,  46,  0,  0,  0,  0,
         2, 146, 116,  3,  58,  36,  4,  36,  16,  4,  36,  12,
         0,   0,   0,  2,  59,  37,  4,  37,  17,  4,  37,  13
    },
    { /* Version 10 */
        346,  274,  216,  154,  122,
         3,  6,  28,  50,  0,  0,  0,  0,
         2,  86,  68,  4,  69,  43,  6,  43,  19,  6,  43,  15,
         2,  87,  69,  1,  70,  44,  2,  44,  20,  2,  44,  16
    },
    { /* Version 11 */
        404,  324,  254,  180,  140,
         3,  6,  30,  54,  0,  0,  0,  0,
         4, 101,  81,  1,  80,  50,  4,  50,  22,  3,  36,  12,
         0,   0,   0,  4,  81,  51,  4,  51,  23,  8,  37,  13
    },
    { /* Version 12 */
        466,  370,  290,  206,  158,
         3,  6,  32,  58,  0,  0,  0,  0,
         2, 116,  92,  6,  58,  36,  4,  46,  20,  7,  42,  14,
         2, 117,  93,  2,  59,  37,  6,  47,  21,  4,  43,  15
    },
    { /* Version 13 */
        532,  428,  334,  244,  180,
         3,  6,  34,  62,  0,  0,  0,  0,
         4, 133, 107,  8,  59,  37,  8,  44,  20, 12,  33,  11,
         0,   0,   0,  1,  60,  38,  4,  45,  21,  4,  34,  12
    },
    { /* Version 14 */
        581,  461,  365,  261,  197,
         4,  6,  26,  46,  66,  0,  0,  0,
         3, 145, 115,  4,  64,  40, 11,  36,  16, 11,  36,  12,
         1, 146, 116,  5,  65,  41,  5,  37,  17,  5,  37,  13
    },
    { /* Version 15 */
        655,  523,  415,  295,  223,
         4,  6,  26,  48,  70,  0,  0,  0,
         5, 109,  87,  5,  65,  41,  5,  54,  24, 11,  36,  12,
         1, 110,  88,  5,  66,  42,  7,  55,  25,  7,  37,  13
    },
    { /* Version 16 */
        733,  589,  453,  325,  253,
         4,  6,  26,  50,  74,  0,  0,  0,
         5, 122,  98,  7,  73,  45, 15,  43,  19,  3,  45,  15,
         1, 123,  99,  3,  74,  46,  2,  44,  20, 13,  46,  16
    },
    { /* Version 17 */
        815,  647,  507,  367,  283,
         4,  6,  30,  54,  78,  0,  0,  0,
         1, 135, 107, 10,  74,  46,  1,  50,  22,  2,  42,  14,
         5, 136, 108,  1,  75,  47, 15,  51,  23, 17,  43,  15
    },
    { /* Version 18 */
        901,  721,  563,  397,  313,
         4,  6,  30,  56,  82,  0,  0,  0,
         5, 150, 120,  9,  69,  43, 17,  50,  22,  2,  42,  14,
         1, 151, 121,  4,  70,  44,  1,  51,  23, 19,  43,  15
    },
    { /* Version 19 */
        991,  795,  627,  445,  341,
         4,  6,  30,  58,  86,  0,  0,  0,
         3, 141, 113,  3,  70,  44, 17,  47,  21,  9,  39,  13,
         4, 142, 114, 11,  71,  45,  4,  48,  22, 16,  40,  14
    },
    { /* Version 20 */
        1085,  861,  669,  485,  385,
         4,  6,  30,  62,  90,  0,  0,  0,
         3, 135, 107,  3,  67,  41, 15,  54,  24, 15,  43,  15,
         5, 136, 108, 13,  68,  42,  5,  55,  25, 10,  44,  16
    },
    { /* Version 21 */
        1156,  932,  714,  512,  406,
         5,  6,  28,  50,  72,  94,  0,  0,
         4, 144, 116, 17,  68,  42, 17,  50,  22, 19,  46,  16,
         4, 145, 117,  0,   0,   0,  6,  51,  23,  6,  47,  17
    },
    { /* Version 22 */
        1258, 1006,  782,  568,  442,
         5,  6,  26,  50,  74,  98,  0,  0,
         2, 139, 111, 17,  74,  46,  7,  54,  24, 34,  37,  13,
         7, 140, 112,  0,   0,   0, 16,  55,  25,  0,   0,   0
    },
    { /* Version 23 */
        1364, 1094,  860,  614,  464,
         5,  6,  30,  54,  78,  102,  0,  0,
         4, 151, 121,  4,  75,  47, 11,  54,  24, 16,  45,  15,
         5, 152, 122, 14,  76,  48, 14,  55,  25, 14,  46,  16
    },
    { /* Version 24 */
        1474, 1174,  914,  664,  514,
         5,  6,  28,  54,  80,  106,  0,  0,
         6, 147, 117,  6,  73,  45, 11,  54,  24, 30,  46,  16,
         4, 148, 118, 14,  74,  46, 16,  55,  25,  2,  47,  17
    },
    { /* Version 25 */
        1588, 1276, 1000,  718,  538,
         5,  6,  32,  58,  84,  110,  0,  0,
         8, 132, 106,  8,  75,  47,  7,  54,  24, 22,  45,  15,
         4, 133, 107, 13,  76,  48, 22,  55,  25, 13,  46,  16
    },
    { /* Version 26 */
        1706, 1370, 1062,  754,  596,
         5,  6,  30,  58,  86,  114,  0,  0,
        10, 142, 114, 19,  74,  46, 28,  50,  22, 33,  46,  16,
         2, 143, 115,  4,  75,  47,  6,  51,  23,  4,  47,  17
    },
    { /* Version 27 */
        1828, 1468, 1128,  808,  628,
         5,  6,  34,  62,  90,  118,  0,  0,
         8, 152, 122, 22,  73,  45,  8,  53,  23, 12,  45,  15,
         4, 153, 123,  3,  74,  46, 26,  54,  24, 28,  46,  16
    },
    { /* Version 28 */
        1921, 1531, 1193,  871,  661,
         6,  6,  26,  50,  74,  98,  122,  0,
         3, 147, 117,  3,  73,  45,  4,  54,  24, 11,  45,  15,
        10, 148, 118, 23,  74,  46, 31,  55,  25, 31,  46,  16
    },
    { /* Version 29 */
        2051, 1631, 1267,  911,  701,
         6,  6,  30,  54,  78,  102,  126,  0,
         7, 146, 116, 21,  73,  45,  1,  53,  23, 19,  45,  15,
         7, 147, 117,  7,  74,  46, 37,  54,  24, 26,  46,  16
    },
    { /* Version 30 */
        2185, 1735, 1373,  985,  745,
         6,  6,  26,  52,  78,  104,  130,  0,
         5, 145, 115, 19,  75,  47, 15,  54,  24, 23,  45,  15,
        10, 146, 116, 10,  76,  48, 25,  55,  25, 25,  46,  16
    },
    { /* Version 31 */
        2323, 1843, 1455, 1033,  793,
         6,  6,  30,  56,  80,  108,  134,  0,
        13, 145, 115,  2,  74,  46, 42,  54,  24, 23,  45,  15,
         3, 146, 116, 29,  75,  47,  1,  55,  25, 28,  46,  16
    },
    { /* Version 32 */
        2465, 1955, 1541, 1115,  845,
         6,  6,  34,  60,  86,  112,  138,  0,
        17, 145, 115, 10,  74,  46, 10,  54,  24, 19,  45,  15,
         0,   0,   0, 23,  75,  47, 35,  55,  25, 35,  46,  16
    },
    { /* Version 33 */
        2611, 2071, 1631, 1171,  901,
         6,  6,  30,  58,  86,  114,  142,  0,
        17, 145, 115, 14,  74,  46, 29,  54,  24, 11,  45,  15,
         1, 146, 116, 21,  75,  47, 19,  55,  25, 46,  46,  16
    },
    { /* Version 34 */
        2761, 2191, 1725, 1231,  961,
         6,  6,  34,  62,  90,  118,  146,  0,
        13, 145, 115, 14,  74,  46, 44,  54,  24, 59,  46,  16,
         6, 146, 116, 23,  75,  47,  7,  55,  25,  1,  47,  17
    },
    { /* Version 35 */
        2876, 2306, 1812, 1286,  986,
         7,  6,  30,  54,  78,  102,  126,  150,
        12, 151, 121, 12,  75,  47, 39,  54,  24, 22,  45,  15,
         7, 152, 122, 26,  76,  48, 14,  55,  25, 41,  46,  16
    },
    { /* Version 36 */
        3034, 2434, 1914, 1354, 1054,
         7,  6,  24,  50,  76,  102,  128,  154,
         6, 151, 121,  6,  75,  47, 46,  54,  24,  2,  45,  15,
        14, 152, 122, 34,  76,  48, 10,  55,  25, 64,  46,  16
    },
    { /* Version 37 */
        3196, 2566, 1992, 1426, 1096,
         7,  6,  28,  54,  80,  106,  132,  158,
        17, 152, 122, 29,  74,  46, 49,  54,  24, 24,  45,  15,
         4, 153, 123, 14,  75,  47, 10,  55,  25, 46,  46,  16
    },
    { /* Version 38 */
        3362, 2702, 2102, 1502, 1142,
         7,  6,  32,  58,  84,  110,  136,  162,
         4, 152, 122, 13,  74,  46, 48,  54,  24, 42,  45,  15,
        18, 153, 123, 32,  75,  47, 14,  55,  25, 32,  46,  16
    },
    { /* Version 39 */
        3532, 2812, 2216, 1582, 1222,
         7,  6,  26,  54,  82,  110,  138,  166,
        20, 147, 117, 40,  75,  47, 43,  54,  24, 10,  45,  15,
         4, 148, 118,  7,  76,  48, 22,  55,  25, 67,  46,  16
    },
    { /* Version 40 */
        3706, 2956, 2334, 1666, 1276,
         7,  6,  30,  58,  86,  114,  142,  170,
        19, 148, 118, 18,  75,  47, 34,  54,  24, 20,  45,  15,
         6, 149, 119, 31,  76,  48, 34,  55,  25, 61,  46,  16
    }
};

/* GF(2^8): Galois field
 * Prime modulus polynomical, P(α) = α^8+α^4+α^3+α^2+1 = 0;
 * => α^8 = α^4+α^3+α^2+1 = 29 (00011101)
 */

/* Y = GF_INT[X]: Y = α^X */
static const unsigned char GF_INT[256] =
{
      1,   2,   4,   8,  16,  32,  64, 128,  29,  58, 116, 232, 205, 135,  19,  38,
     76, 152,  45,  90, 180, 117, 234, 201, 143,   3,   6,  12,  24,  48,  96, 192,
    157,  39,  78, 156,  37,  74, 148,  53, 106, 212, 181, 119, 238, 193, 159,  35,
     70, 140,   5,  10,  20,  40,  80, 160,  93, 186, 105, 210, 185, 111, 222, 161,
     95, 190,  97, 194, 153,  47,  94, 188, 101, 202, 137,  15,  30,  60, 120, 240,
    253, 231, 211, 187, 107, 214, 177, 127, 254, 225, 223, 163,  91, 182, 113, 226,
    217, 175,  67, 134,  17,  34,  68, 136,  13,  26,  52, 104, 208, 189, 103, 206,
    129,  31,  62, 124, 248, 237, 199, 147,  59, 118, 236, 197, 151,  51, 102, 204,
    133,  23,  46,  92, 184, 109, 218, 169,  79, 158,  33,  66, 132,  21,  42,  84,
    168,  77, 154,  41,  82, 164,  85, 170,  73, 146,  57, 114, 228, 213, 183, 115,
    230, 209, 191,  99, 198, 145,  63, 126, 252, 229, 215, 179, 123, 246, 241, 255,
    227, 219, 171,  75, 150,  49,  98, 196, 149,  55, 110, 220, 165,  87, 174,  65,
    130,  25,  50, 100, 200, 141,   7,  14,  28,  56, 112, 224, 221, 167,  83, 166,
     81, 162,  89, 178, 121, 242, 249, 239, 195, 155,  43,  86, 172,  69, 138,   9,
     18,  36,  72, 144,  61, 122, 244, 245, 247, 243, 251, 235, 203, 139,  11,  22,
     44,  88, 176, 125, 250, 233, 207, 131,  27,  54, 108, 216, 173,  71, 142,   1
};

/* Y = GF_EXP[X]: X = α^Y */
static const unsigned char GF_EXP[256] =
{
      0,   0,   1,  25,   2,  50,  26, 198,   3, 223,  51, 238,  27, 104, 199,  75,
      4, 100, 224,  14,  52, 141, 239, 129,  28, 193, 105, 248, 200,   8,  76, 113,
      5, 138, 101,  47, 225,  36,  15,  33,  53, 147, 142, 218, 240,  18, 130,  69,
     29, 181, 194, 125, 106,  39, 249, 185, 201, 154,   9, 120,  77, 228, 114, 166,
      6, 191, 139,  98, 102, 221,  48, 253, 226, 152,  37, 179,  16, 145,  34, 136,
     54, 208, 148, 206, 143, 150, 219, 189, 241, 210,  19,  92, 131,  56,  70,  64,
     30,  66, 182, 163, 195,  72, 126, 110, 107,  58,  40,  84, 250, 133, 186,  61,
    202,  94, 155, 159,  10,  21, 121,  43,  78, 212, 229, 172, 115, 243, 167,  87,
      7, 112, 192, 247, 140, 128,  99,  13, 103,  74, 222, 237,  49, 197, 254,  24,
    227, 165, 153, 119,  38, 184, 180, 124,  17,  68, 146, 217,  35,  32, 137,  46,
     55,  63, 209,  91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190,  97,
    242,  86, 211, 171,  20,  42,  93, 158, 132,  60,  57,  83,  71, 109,  65, 162,
     31,  45,  67, 216, 183, 123, 164, 118, 196,  23,  73, 236, 127,  12, 111, 246,
    108, 161,  59,  82,  41, 157,  85, 170, 251,  96, 134, 177, 187, 204,  62,  90,
    203,  89,  95, 176, 156, 169, 160,  81,  11, 245,  22, 235, 122, 117,  44, 215,
     79, 174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168,  80,  88, 175
};

/* Generator polynomials for Reed-Solomon error correction codewords */
static unsigned char GP_7[]  = { 87, 229, 146, 149, 238, 102,  21};
static unsigned char GP_10[] = {251,  67,  46,  61, 118,  70,  64,  94,  32,  45};
static unsigned char GP_13[] = { 74, 152, 176, 100,  86, 100, 106, 104, 130, 218, 206, 140,  78};
static unsigned char GP_15[] = {  8, 183,  61,  91, 202,  37,  51,  58,  58, 237, 140, 124,   5,  99, 105};
static unsigned char GP_16[] = {120, 104, 107, 109, 102, 161,  76,   3,  91, 191, 147, 169, 182, 194, 225, 120};
static unsigned char GP_17[] = { 43, 139, 206,  78,  43, 239, 123, 206, 214, 147,  24,  99, 150,  39, 243, 163, 136};
static unsigned char GP_18[] = {215, 234, 158,  94, 184,  97, 118, 170,  79, 187, 152, 148, 252, 179,   5,  98,  96, 153};
static unsigned char GP_20[] = { 17,  60,  79,  50,  61, 163,  26, 187, 202, 180, 221, 225,  83, 239, 156, 164, 212, 212, 188, 190};
static unsigned char GP_22[] = {210, 171, 247, 242,  93, 230,  14, 109, 221,  53, 200,  74,   8, 172,  98,  80, 219, 134, 160, 105, 165, 231};
static unsigned char GP_24[] = {229, 121, 135,  48, 211, 117, 251, 126, 159, 180, 169, 152, 192, 226, 228, 218, 111,   0, 117, 232,  87,  96, 227,  21};
static unsigned char GP_26[] = {173, 125, 158,   2, 103, 182, 118,  17, 145, 201, 111,  28, 165,  53, 161,  21, 245, 142,  13, 102,  48, 227, 153, 145, 218,  70};
static unsigned char GP_28[] = {168, 223, 200, 104, 224, 234, 108, 180, 110, 190, 195, 147, 205,  27, 232, 201,  21,  43, 245,  87,  42, 195, 212, 119, 242,  37,   9, 123};
static unsigned char GP_30[] = { 41, 173, 145, 152, 216,  31, 179, 182,  50,  48, 110,  86, 239,  96, 222, 125,  42, 173, 226, 193, 224, 130, 156,  37, 251, 216, 238,  40, 192, 180};

static unsigned char* GP_LIST[] =
{
     NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  GP_7,  NULL,  NULL,
    GP_10,  NULL,  NULL, GP_13,  NULL, GP_15, GP_16, GP_17, GP_18,  NULL,
    GP_20,  NULL, GP_22,  NULL, GP_24,  NULL, GP_26,  NULL, GP_28,  NULL,
    GP_30
};

/* Alphanumeric characters */
static const char ALPHA_NUMERIC_TABLE[ALPHA_NUMERIC_COUNT] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '$', '%', '*',
    '+', '-', '.', '/', ':'
};

/* Number of bits in character count indicator */
static const unsigned char BITS_OF_CHARACTER_COUNT[3][MAX_MODE] = 
{
    /* Version 1~9 */
    {10,  9,  8,  8},
    /* Version 10~26 */
    {12, 11, 16, 10},
    /* Version 27~40 */
    {14, 13, 16, 12}
};

/* Convert character to alphanumeric value */
int alpha_numeric_value(char c)
{
    for (int i = 0; i < ALPHA_NUMERIC_COUNT; i++)
    {
        if (ALPHA_NUMERIC_TABLE[i] == c)
        {
            return i;
        }
    }

    return -1;
}

/* Get number of bits in character count indicator */
int character_count_len(int mode, int version)
{
    int bits_count = 0;

    if (version >= 1 && version <= 9)
    {
        bits_count = BITS_OF_CHARACTER_COUNT[0][mode];
    }
    else if (version >= 10 && version <= 26)
    {
        bits_count = BITS_OF_CHARACTER_COUNT[1][mode];
    }
    else if (version >= 27 && version <= 40)
    {
        bits_count = BITS_OF_CHARACTER_COUNT[2][mode];
    }

    return bits_count;
}

/* Convert int to a binary string (len bits) */
string I2BS(int value, int len)
{
    string ret(len, '0');

    for (int i = 0; i < len; i++)
    {
        ret[len - i - 1] = '0' + ((value >> i) & 0x01);
    }

    return ret;
}

/* Convert binary string (8 bits) to unsigned char */
unsigned char BS2I(string s)
{
    unsigned char value = 0;

    for (int i = 0; i < 8; i++)
    {
        value += (s[i] - '0') << (7 - i);
    }

    return value;
}

/* Check Shift JIS characters */
bool is_kanji(string content)
{
    int data_len = content.size();

    if (data_len % 2)
    {
        return false;
    }

    for (int i = 0; i < data_len; i+=2)
    {
        unsigned char c = content[i];
        if ((c < 0x81 || c > 0x9F) && (c < 0xE0 || c > 0xEB))
        {
            return false;
        }
    }

    return true;
}

/* Check UTF-8 characters 
 * 1-byte 0xxxxxxx 
 * 2-byte 110xxxxx 10xxxxxx 
 * 3-byte 1110xxxx 10xxxxxx 10xxxxxx 
 * 4-byte 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * 5-byte 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * 6-byte 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */  
bool is_UTF8(string content)
{
    int bytes = 0;
    bool all_ascii = true;
 
    for (unsigned int i = 0; i < content.size(); i++)
    {
        unsigned char c = content[i];

        if ((c & 0x80) != 0)
        {
            all_ascii = false;
        }
            
        if (bytes == 0)
        {
            if ((c & 0x80) != 0)
            {
                while ((c & 0x80) != 0)
                {
                    c <<= 1;
                    bytes++;
                }

                if (bytes < 2 || bytes > 6)
                {
                    return false;
                }

                bytes--;
            }
        }
        else
        {
            if ((c & 0xC0) != 0x80)
            {
                return false;
            }
                
            bytes--;
        }
    }

    if (all_ascii)
    {
        return false;
    }
        
    return (bytes == 0);
}

/* Determine mode of content */
int mode_check(string content)
{
    int mode = -1;

    if (is_kanji(content))
    {
        return KANJI;
    }

    for (unsigned int i = 0; i < content.size(); i++)
    {
        char c = content[i];
        if (c >= '0' && c <= '9')
        {
            mode = (mode < NUMERIC) ? NUMERIC : mode;
        }
        else if (alpha_numeric_value(c) >= 0)
        {
            mode = (mode < ALPHA_NUMERIC) ? ALPHA_NUMERIC : mode;
        }
        else
        {
            mode = BYTE;
        }
    }

    return mode;
}

/* Determine encoding of content: default or UTF8 */
int encoding_check(string content)
{
    if (is_UTF8(content))
    {
        return UTF_8;
    }

    return ISO_8859_1;
}

/* Determine most appropriate version */
int version_check(string content, int ec_level, int version, int mode, int encoding)
{
    int data_len = content.size();
    int eci_len = (UTF_8 == encoding) ? 12 : 0;
    int min_ver = QR_MIN_VERSION, max_ver = QR_MAX_VERSION;

    if (KANJI == mode)
    {
        data_len /= 2;
    }

    if (AUTO_VERSION != version)
    {
        min_ver = max_ver = version;
    }

    for (int v = min_ver; v <= max_ver; v++)
    {
        int capacity = 4 + character_count_len(mode, v) + eci_len;

        switch(mode)
        {
            case NUMERIC:
            {
                int r = (data_len % 3 == 0) ? 0 : ((data_len % 3 == 1) ? 4 : 7);
                capacity += (10*(data_len/3) + r);
            }         
            break;

            case ALPHA_NUMERIC:
                capacity += (11*(data_len/2) + 6*(data_len % 2));
            break;

            case BYTE:
                capacity += 8*data_len;
            break;

            case KANJI:
                capacity += 13*data_len;
            break;
        }

        if (capacity <= QR_info[v - 1].data_bytes[ec_level]*8)
        {
            return v;
        }
    }
        
    return -1;
}

string eci_header(int mode, int encoding)
{
    string ret = "";

    if ((BYTE == mode) && (UTF_8 == encoding))
    {
        /* ECI mode indicator */
        ret += "0111";
        /* ECI Assignment number (000026) */
        ret += "00011010";
    }

    return ret;
}

string mode_indicator(int mode)
{
    string ret = "";

    switch(mode)
    {
        case NUMERIC:
            ret = "0001";
        break;

        case ALPHA_NUMERIC:
            ret = "0010";
        break;

        case BYTE:
            ret = "0100";
        break;

        case KANJI:
            ret = "1000";
        break;
    }

    return ret;
}

string character_count(int len, int mode, int version)
{
    if (KANJI == mode)
    {
        len /= 2;
    }

    return I2BS(len, character_count_len(mode, version));
}

/* NUMERIC MODE:
 * Input characters are divied into groups of 3 characters
 * For each group, Converted to its 10-bit binary equivalent.
 * If the number of data is NOT a multiple of 3
 * the final 1 or 2 digits I2BSerted to 4 or 7 bits binary number */
string encode_numeric(string content)
{
    string ret = "";

    for (unsigned int i = 0; i < content.size(); i+=3)
    {
        string num = "";

        if ((i + 1 < content.size()) && (i + 2 < content.size()))
        {
            num = content.substr(i, 3);
            ret += I2BS(atoi(num.c_str()), 10);
        }
        else if (i + 1 < content.size())
        {
            num = content.substr(i, 2);
            ret += I2BS(atoi(num.c_str()), 7);
        }
        else
        {
            num = content.substr(i, 1);
            ret += I2BS(atoi(num.c_str()), 4);
        }
    }

    return ret;
}

/* ALPHA_NUMERIC MODE:
 * Input characters are divied into groups of 2 characters
 * For each group, character value of the 1st character multiplied by 45 and add the 2nd
 * the result encoded as 11-bit binary number
 * If the number of data is NOT a multiple of 2, the final character is encoded as a 6-bit binary number */
string encode_alpha_numeric(string content)
{
    string ret = "";

    for (unsigned int i = 0; i < content.size(); i+=2)
    {
        int a = alpha_numeric_value(content[i]);

        if (i + 1 < content.size())
        {
            int b = alpha_numeric_value(content[i + 1]);
            ret += I2BS(a*45 + b, 11);
        }
        else
        {
            ret += I2BS(a, 6);
        }
    }

    return ret;
}

/* BYTE MODE:
 * One 8-bit codeword directly represents the byte value of the input data character*/
string encode_byte(string content)
{
    string ret = "";

    for (unsigned int i = 0; i < content.size(); i++)
    {
        ret += I2BS(content[i], 8);
    }

    return ret;
}

/* KANJI MODE:
 * Input characters are compacted to 13-bit binary
 * characters with Shift JIS values from 0x8140 to 0x9FFC: Subtract 0x8140
 * characters with Shift JIS values from 0xE040 to 0xEBBF: Subtract 0xC140
 * Multiply most significant byte of result by 0xC0, and add least significant byte to product */
string encode_kanji(string content)
{
    string ret = "";

    for (unsigned int i = 0; i < content.size(); i+=2)
    {
        int v = ((content[i] & 0xFF) << 8) | content[i + 1];
        
        if (v >= 0x8140 && v <= 0x9FFC)
        {
            v -= 0x8140;
        }
        else
        {
            v -= 0xC140;
        }

        v = ((v >> 8) * 0xC0) + (v & 0x00FF);
        ret += I2BS(v, 13);
    }

    return ret;
}

string encode_content(string content, int mode)
{
    string ret = "";

    switch(mode)
    {
        case NUMERIC:
            ret = encode_numeric(content);
        break;

        case ALPHA_NUMERIC:
            ret = encode_alpha_numeric(content);
        break;

        case BYTE:
            ret = encode_byte(content);
        break;

        case KANJI:
            ret = encode_kanji(content);
        break;
    }

    return ret;
}

string terminator(int curr_bits_count, int version, int ec_level)
{
    int max_bits = QR_info[version - 1].data_bytes[ec_level]*8;
    return (curr_bits_count <= (max_bits - 4)) ? "0000" : "";
}

string padding_bits(int curr_bits_count)
{
    int need_bits = ((curr_bits_count + 7)/8)*8 - curr_bits_count;
    return (need_bits > 0) ? string(need_bits, '0') : "";
}

string padding_codewords(int curr_bits_count, int version, int ec_level)
{
    string ret = "";
    bool flg = true;

    int max_bits = QR_info[version - 1].data_bytes[ec_level]*8;
    while(curr_bits_count < max_bits)
    {
        ret += (flg ? "11101100" : "00010001");

        flg = !flg;
        curr_bits_count += 8;
    }

    return ret;
}

string Reed_Solomon(string block_str, int data_bytes, int rs_bytes)
{
    string ret = "";

    unsigned char *rs = new unsigned char [data_bytes + rs_bytes];
    memset(rs, 0, data_bytes + rs_bytes);

    for (int i = 0; i < data_bytes; i++)
    {
        rs[i] = BS2I(block_str.substr(i*8, 8));
    }

    for (int i = 0; i < data_bytes; i++)
    {
        if (rs[0] != 0)
        {
            int first = GF_EXP[rs[0]];

            for (int j = 0; j < rs_bytes; j++)
            {
                int ele = GP_LIST[rs_bytes][j] + first;
                ele = ele % 255;
                rs[j] = rs[j+1] ^ GF_INT[ele];
            }

            for (int j = rs_bytes; j < data_bytes + rs_bytes - 1; j++)
            {
                rs[j] = rs[j + 1];
            }
        }
        else
        {
            for (int j = 0; j < data_bytes + rs_bytes - 1; j++)
            {
                rs[j] = rs[j + 1];
            }
        }  
    }

    for (int i = 0; i < rs_bytes; i++)
    {
        ret += I2BS(rs[i], 8);
    }
    delete [] rs;
    return ret;
}

string error_correction(string qr_str, int version, int ec_level)
{
    string ret = "";
    int start_pos = 0;

    for (int i = 0; i <= 1; i++)
    {
        EC_INFO info = QR_info[version - 1].ec_info[i][ec_level];
        int rs_bytes = info.block_bytes - info.block_data_bytes;

        for (int j = 0; j < info.blocks; j++)
        {
            string block_str = qr_str.substr(start_pos, info.block_data_bytes*8);
            ret += Reed_Solomon(block_str, info.block_data_bytes, rs_bytes);

            start_pos += info.block_data_bytes*8;
        }
    }
    
    return ret;
}

void QR::finder_pattern(int x, int y)
{
    static unsigned char pattern[][FINDER_PATTERN_SIZE] =
    {
        1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1
    };

    for (int i = 0; i < FINDER_PATTERN_SIZE; i++)
    {
        for (int j = 0; j < FINDER_PATTERN_SIZE; j++)
        {
            QR_DATA[(x + i)*SIZE + (y + j)] = pattern[i][j] + MODULE_FUNCION_PATTERN;
        }
    }
}

void QR::separator()
{
    for (int i = 0; i < 8; i++)
    {
        QR_DATA[7*SIZE + i] = 0 + MODULE_FUNCION_PATTERN;
        QR_DATA[7*SIZE + SIZE - 8 + i] = 0 + MODULE_FUNCION_PATTERN;
        QR_DATA[(SIZE-8)*SIZE + i] = 0 + MODULE_FUNCION_PATTERN;

        QR_DATA[i*SIZE + 7] = 0 + MODULE_FUNCION_PATTERN;
        QR_DATA[(SIZE - 8 + i)*SIZE + 7] = 0 + MODULE_FUNCION_PATTERN;
        QR_DATA[i*SIZE + SIZE - 8] = 0 + MODULE_FUNCION_PATTERN;
    }
}

void QR::timing_pattern()
{
    for (int i = FINDER_PATTERN_SIZE + 1; i < SIZE - FINDER_PATTERN_SIZE - 1; i++)
    {
        QR_DATA[6*SIZE + i] = QR_DATA[i*SIZE + 6] = ((i + 1)%2) + MODULE_FUNCION_PATTERN;
    }
}

void QR::align_pattern()
{
    static unsigned char pattern[][ALIGN_PATTERN_SIZE] =
    {
        1, 1, 1, 1, 1,
        1, 0, 0, 0, 1,
        1, 0, 1, 0, 1,
        1, 0, 0, 0, 1,
        1, 1, 1, 1, 1
    };

    QR_INFO info = QR_info[VERSION - 1];

    for (int i = 0; i < info.align_coords; i++)
    {
        for (int j = 0; j < info.align_coords; j++)
        {
            int x = info.align_coord[i];
            int y = info.align_coord[j];

            if (x && y && QR_DATA[x*SIZE + y] == MODULE_NOT_SET)
            {
                /* (x, y) is center coordinate, left top is (x-2, y-2) */
                x -= 2;
                y -= 2;

                for (int m = 0; m < ALIGN_PATTERN_SIZE; m++)
                {
                    for (int n = 0; n < ALIGN_PATTERN_SIZE; n++)
                    {
                        QR_DATA[(x + m)*SIZE + (y + n)] = pattern[m][n] + MODULE_FUNCION_PATTERN;
                    }
                }
            }
        }
    }
}

/* The format information is a 15-bit sequence
 * containing 5 data bits, with 10 error correction bits calculated using the (15, 5) BCH code
 * data bits 0~2: data mask pattern
 * data bits 3~4: error correction level (L:01; M:00; Q:11; H:10) */
void QR::format_info(int data_mask)
{
    int format;

    switch (EC_LEVEL)
    {
    case LEVEL_M:
        format = 0x00;
    break;

    case LEVEL_L:
        format = 0x08;
    break;

    case LEVEL_Q:
        format = 0x18;
    break;

    default:
        format = 0x10;
    break;
    }

    format += data_mask;
    int coefficient = (format << 10);

    for (int i = 0; i < 5; i++)
    {
        if (coefficient & (1 << (14 - i)))
        {
            /* Generator polynomial G(x) = x^10+x^8+x^5+x^4+x^2+x+1. 10100110111 (0x0537) */
            coefficient ^= (0x0537 << (4 - i));
        }
    }
    format = (format << 10) + coefficient;
    format ^= 0x5412; /* masking shall be applied by XORing the bit string with 101010000010010 (0x5412) */

    QR_DATA[8] = QR_DATA[8*SIZE + SIZE - 1] = (format & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[SIZE + 8] = QR_DATA[8*SIZE + SIZE - 2] = ((format >> 1) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[2*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 3] = ((format >> 2) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[3*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 4] = ((format >> 3) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[4*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 5] = ((format >> 4) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[5*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 6] = ((format >> 5) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[7*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 7] = ((format >> 6) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 8] = QR_DATA[8*SIZE + SIZE - 8] = ((format >> 7) & 0x01) + MODULE_FUNCION_PATTERN;
    /* Dark module */
    QR_DATA[(SIZE-8)*SIZE + 8] = 1 + MODULE_FUNCION_PATTERN;

    QR_DATA[8*SIZE + 7] = QR_DATA[(SIZE - 7)*SIZE + 8] = ((format >> 8) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 5] = QR_DATA[(SIZE - 6)*SIZE + 8] = ((format >> 9) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 4] = QR_DATA[(SIZE - 5)*SIZE + 8] = ((format >> 10) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 3] = QR_DATA[(SIZE - 4)*SIZE + 8] = ((format >> 11) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 2] = QR_DATA[(SIZE - 3)*SIZE + 8] = ((format >> 12) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 1] = QR_DATA[(SIZE - 2)*SIZE + 8] = ((format >> 13) & 0x01) + MODULE_FUNCION_PATTERN;
    QR_DATA[8*SIZE + 0] = QR_DATA[(SIZE - 1)*SIZE + 8] = ((format >> 14) & 0x01) + MODULE_FUNCION_PATTERN;
}

/* The version information is a 18-bit sequence
 * containing 6 data bits, with 12 error correction bits calculated using the (18, 6) Golay code
 * data bits 0~5: version */
void QR::version_info()
{
    /* version information included in QR of version 7 or larger */
    if (VERSION < 7)
    {
        return;
    }

    int ver_data = (VERSION << 12);

    for (int i = 0; i < 6; i++)
    {
        if (ver_data & (1 << (17 - i)))
        {
            /* Generator polynomial G(x) = x^12+x^11+x^10+x^9+x^8+x^5+x^2+1. 1111100100101 (0x1F25) */
            ver_data ^= (0x1F25 << (5 - i));
        }
    }
    ver_data += (VERSION << 12);

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            QR_DATA[i*SIZE + (SIZE - 11 + j)] = ((ver_data >> (i*3 + j)) & 0x01) + MODULE_FUNCION_PATTERN;
            QR_DATA[(SIZE - 11 + j)*SIZE + i] = ((ver_data >> (i*3 + j)) & 0x01) + MODULE_FUNCION_PATTERN;
        }
    }
}

void QR::data_pattern(string qr_str)
{
    int x = SIZE - 1;
    int y = SIZE - 1;
    bool upwards = true;
    bool left = true;

    for (int i = 0; i < QR_info[VERSION - 1].total_bytes*8;)
    {
        if (QR_DATA[x*SIZE + y] == MODULE_NOT_SET)
        {
            QR_DATA[x*SIZE + y] = qr_str[i] - '0';
            i++;
        }
        
        if (upwards)
        {
            if (left)
            {
                y--;
            }
            else if (x > 0)
            {
                x--;
                y++;
            }
            else
            {
                y--;
                if (y == 6)
                {
                    y = 5;
                }
                upwards = false;
                left = false;
            }      
        }
        else
        {
            if (left)
            {
                y--;
            }
            else if (x + 1 <= SIZE - 1)
            {
                x++;
                y++;
            }
            else
            {
                y--;
                upwards = true;
                left = false;
            }
        }

        left = !left;
    }
}

bool QR::is_light_row(int row, int start_col)
{
    int end_col = start_col + 3;

    start_col = (start_col < 0) ? 0 : start_col;
    end_col = (end_col > SIZE - 1) ? SIZE - 1 : end_col;

    for (int i = start_col; i <= end_col; i++)
    {
        if (QR_DATA[row*SIZE + i] & 0x01)
        {
            return false;
        }
    }

    return true;
}

bool QR::is_light_col(int col, int start_row)
{
    int end_row = start_row + 3;

    start_row = (start_row < 0) ? 0 : start_row;
    end_row = (end_row > SIZE - 1) ? SIZE - 1 : end_row;

    for (int i = start_row; i <= end_row; i++)
    {
        if (QR_DATA[i*SIZE + col] & 0x01)
        {
            return false;
        }
    }

    return true;
}

int QR::data_mask_score()
{
    int score = 0;
    int i, j, k;

    /* Adjacent modules in column in same color: penalty socre N1 = 3 */
    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE - 4; j++)
        {
            int count = 1;

            for (k = j + 1; k < SIZE; k++)
            {
                if ((QR_DATA[i*SIZE + j] & 0x01) == (QR_DATA[i*SIZE + k] & 0x01))
                {
                    count++;
                }
                else
                {
                    break;
                }
            }

            if (count >= 5)
            {
                score += (count - 5) + 3;
            }

            j = k - 1;
        }
    }

    /* Adjacent modules in row in same color: penalty socre N1 = 3 */
    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE - 4; j++)
        {
            int count = 1;

            for (k = j + 1; k < SIZE; k++)
            {
                if ((QR_DATA[j*SIZE + i] & 0x01) == (QR_DATA[k*SIZE + i] & 0x01))
                {
                    count++;
                }
                else
                {
                    break;
                }
            }

            if (count >= 5)
            {
                score += (count - 5) + 3;
            }

            j = k - 1;
        }
    }

    /* Block of modules in same color: penalty socre N2 = 3 */
    for (i = 0; i < SIZE - 1; i++)
    {
        for (j = 0; j < SIZE - 1; j++)
        {
            if ((QR_DATA[i*SIZE + j] & 0x01) == (QR_DATA[i*SIZE + j + 1] & 0x01)
                && (QR_DATA[i*SIZE + j] & 0x01) == (QR_DATA[(i + 1)*SIZE + j] & 0x01)
                && (QR_DATA[i*SIZE + j] & 0x01) == (QR_DATA[(i + 1)*SIZE + j + 1] & 0x01))
            {
                score += 3;
            }
        }
    }

    /* 1:1:3:1:1 ratio pattern: penalty socre N3 = 40 */
    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE - 6; j++)
        {
            /* Row */
            if ((QR_DATA[i*SIZE + j] & 0x01) == 1
                && (QR_DATA[i*SIZE + j + 1] & 0x01) == 0
                && (QR_DATA[i*SIZE + j + 2] & 0x01) == 1
                && (QR_DATA[i*SIZE + j + 3] & 0x01) == 1
                && (QR_DATA[i*SIZE + j + 4] & 0x01) == 1
                && (QR_DATA[i*SIZE + j + 5] & 0x01) == 0
                && (QR_DATA[i*SIZE + j + 6] & 0x01) == 1)
            {
                if (is_light_row(i, j - 4) || is_light_row(i, j + 7))
                {
                    score += 40;
                }
            }
            /* Column */
            if ((QR_DATA[j*SIZE + i] & 0x01) == 1
                && (QR_DATA[(j + 1)*SIZE + i] & 0x01) == 0
                && (QR_DATA[(j + 2)*SIZE + i] & 0x01) == 1
                && (QR_DATA[(j + 3)*SIZE + i] & 0x01) == 1
                && (QR_DATA[(j + 4)*SIZE + i] & 0x01) == 1
                && (QR_DATA[(j + 5)*SIZE + i] & 0x01) == 0
                && (QR_DATA[(j + 6)*SIZE + i] & 0x01) == 1)
            {
                if (is_light_col(i, j - 4) || is_light_col(i, j + 7))
                {
                    score += 40;
                }
            }
        }
    }

    /* Proportion of dark modules in entire symbol: penalty socre N4 = 10 */
    int count = 0;
    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            if (QR_DATA[i*SIZE + j] & 0x01)
            {
                count++;
            }
        }
    }

    int ratio = abs(50 - (count*100)/(SIZE*SIZE))/5;
    score += 10*ratio;

    return score;
}

/* 1. Data masking is not applied to function patterns.
 * 2. Data masking is performed on the encoding region (excluding format information and version information)
 * 3. Although the data masking is performed on encoding region, the area to be evaluated is the complete symbol.
 * 4. Select the pattern with the lowest penalty points score. */
int QR::data_mask_evaluation()
{
    int data_mask = 0;
    int min_penalty = 0xFFFF;

    for (int i = 0; i < 8; i++)
    {
        format_info(i);
        data_mask_pattern(i);

        int penalty = data_mask_score();

        if (penalty < min_penalty)
        {
            min_penalty = penalty;
            data_mask = i;
        }

        data_mask_pattern(i);
    }

    return data_mask;
}

void QR::data_mask_pattern(int data_mask)
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            bool mask;

            if (QR_DATA[i*SIZE + j] & MODULE_FUNCION_PATTERN)
            {
                continue;
            }

            switch(data_mask)
            {
                case 0:
                    mask = ((i + j) % 2 == 0);
                break;

                case 1:
                    mask = (i % 2 == 0);
                break;

                case 2:
                    mask = (j % 3 == 0);
                break;

                case 3:
                    mask = ((i + j) % 3 == 0);
                break;

                case 4:
                    mask = (((i / 2) + (j / 3)) % 2 == 0);
                break;

                case 5:
                    mask = (((i * j) % 2) + ((i * j) % 3) == 0);
                break;

                case 6:
                    mask = ((((i * j) % 2) + ((i * j) % 3)) % 2 == 0);
                break;

                case 7:
                    mask = ((((i + j) % 2) + ((i * j) % 3)) % 2 == 0);
                break;
            }

            QR_DATA[i*SIZE + j] ^= mask;
        }
    }
}

string QR::construct_data(string qr_str, string ec_str)
{
    string ret = "";
    EC_INFO info1 = QR_info[VERSION - 1].ec_info[0][EC_LEVEL];
    EC_INFO info2 = QR_info[VERSION - 1].ec_info[1][EC_LEVEL];
    int data_bytes = (info1.block_data_bytes < info2.block_data_bytes) ? info2.block_data_bytes : info1.block_data_bytes;

    for (int i = 0; i < data_bytes; i++)
    {
        int index = i;
        for (int j = 0; j < info1.blocks + info2.blocks; j++)
        {
            int len = (j < info1.blocks) ? info1.block_data_bytes : info2.block_data_bytes;

            if (j < info1.blocks && i >= info1.block_data_bytes)
            {
                index += len;
            }
            else
            {
                ret += qr_str.substr(index*8, 8);
                index += len;
            }
        }
    }

    for (int i = 0; i < info1.block_bytes - info1.block_data_bytes; i++)
    {
        int index = i;
        for (int j = 0; j < info1.blocks + info2.blocks; j++)
        {
            ret += ec_str.substr(index*8, 8);
            index += (info1.block_bytes - info1.block_data_bytes);
        }
    }

    return ret;
}

string QR::encode_data(string content)
{
    string data_str = "", ec_str;

    data_str += eci_header(MODE, ENCODING);
    data_str += mode_indicator(MODE);
    data_str += character_count(content.size(), MODE, VERSION);
    data_str += encode_content(content, MODE);
    data_str += terminator(data_str.size(), VERSION, EC_LEVEL);
    data_str += padding_bits(data_str.size());
    data_str += padding_codewords(data_str.size(), VERSION, EC_LEVEL);
    ec_str = error_correction(data_str, VERSION, EC_LEVEL);

    return construct_data(data_str, ec_str);
}

QR::QR(string content, char *path, int ec_level, int version)
{
    EC_LEVEL = ec_level;
    MODE = mode_check(content);
    ENCODING = encoding_check(content);
    VERSION = version_check(content, ec_level, version, MODE, ENCODING);

    /* Invalid mode or version */
    if (VERSION < QR_MIN_VERSION || MODE < 0)
    {
        return;
    }

    SIZE = QR_MIN_SIZE + (VERSION - 1)*4;

    QR_DATA = new unsigned char[SIZE*SIZE];
    memset(QR_DATA, MODULE_NOT_SET, SIZE*SIZE);

    /* Function patterns */
    finder_pattern(0, 0);
    finder_pattern(0, SIZE - FINDER_PATTERN_SIZE);
    finder_pattern(SIZE - FINDER_PATTERN_SIZE, 0);
    separator();
    align_pattern();
    timing_pattern();

    /* Encoding region */
    format_info(0);
    version_info();
    string bit_stream = encode_data(content);
    data_pattern(bit_stream);

    int data_mask = data_mask_evaluation();
    data_mask_pattern(data_mask);
    format_info(data_mask);

    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            QR_DATA[i*SIZE + j] &= 0x01;
        }
    }

    /* Create QR code png file */
    PNG qr_png;
    qr_png.set_ihdr(SIZE, SIZE, BIT_DEPTH_1, INDEXED_COLOR);

    PIXEL pixels[] = {{0xFF, 0xFF, 0xFF}, {0, 0, 0}};
    qr_png.set_plte(pixels, 2);

    qr_png.set_idat((unsigned char *)QR_DATA);
    qr_png.write(path);

    delete [] QR_DATA;
}