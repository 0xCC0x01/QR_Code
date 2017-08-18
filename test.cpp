#include <stdio.h>
#include "QR.h"

static char error_correction_char(int ec_level)
{
    char ec_char[] = {'L', 'M', 'Q', 'H'};
    return ec_char[ec_level];
}

void test_numeric(int v, int e)
{
    char path[100] = {0};
    sprintf_s(path, "D:\\QR\\QR_NUM_%d_%c.png", v, error_correction_char(e));
    QR qr = QR("0123456789", path, e, v);
}

void test_alphanumeric(int v, int e)
{
    char path[100] = {0};
    sprintf_s(path, "D:\\QR\\QR_ALPHA_%d_%c.png", v, error_correction_char(e));
    QR qr = QR("TEST QR", path, e, v);
}

void test_byte(int v, int e)
{
    char path[100] = {0};
    sprintf_s(path, "D:\\QR\\QR_BYTE_%d_%c.png", v, error_correction_char(e));
    QR qr = QR("TEST QR!?@", path, e, v);
}

void test_kanji(int v, int e)
{
    char path[100] = {0};
    sprintf_s(path, "D:\\QR\\QR_KANJI_%d_%c.png", v, error_correction_char(e));
    /*  Shift JIS value */
    QR qr = QR("\x83\x65\x83\x58\x83\x67", path, e, v);
}

void test_chinese(int v, int e)
{
    char path[100] = {0};
    sprintf_s(path, "D:\\QR\\QR_CHINESE_%d_%c.png", v, error_correction_char(e));
    QR qr = QR("测试QR", path, e, v);
}

int main()
{
    for (int v = 1; v <= 40; v++)
    {
        for (int e = LEVEL_L; e <= LEVEL_H; e++)
        {
            test_numeric(v, e);
            test_alphanumeric(v, e);
            test_byte(v, e);
            test_kanji(v, e);
            test_chinese(v, e);
        }
    }
}