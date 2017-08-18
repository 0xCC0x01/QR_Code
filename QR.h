/* Reference (QR Specification): ISO/IEC 18004:2006
 */

#ifndef _QR_H_
#define _QR_H_

#include <string>
using std::string;

/* Determine version automatically */
#define AUTO_VERSION    (-1)

/* Error Correction Level */
enum
{
    /* Recovery Capacity: 7% */
    LEVEL_L = 0,
    /* Recovery Capacity: 15% */
    LEVEL_M = 1,
    /* Recovery Capacity: 25% */
    LEVEL_Q = 2,
    /* Recovery Capacity: 30% */
    LEVEL_H = 3
};

/* Data Mode */
enum
{
    /* Decimal digit (0 - 9) */
    NUMERIC = 0,
    /* Set of 45 characters */
    ALPHA_NUMERIC = 1,
    /* 8 bits per character */
    BYTE = 2,
    /* Kanji characters */
    KANJI = 3,
    /* Total supported modes */
    MAX_MODE
};

/* Data Encoding */
enum
{
    /* Default */
    ISO_8859_1 = 0,
    /* ISO/IEC 10646 UTF-8. ECI Id:"000026" */
    UTF_8 = 1
};

class QR
{
private:
    int VERSION;
    int MODE;
    int SIZE;
    int EC_LEVEL;
    int ENCODING;
    unsigned char *QR_DATA;

private:
    void finder_pattern(int x, int y);
    void separator();
    void timing_pattern();
    void align_pattern();

    string encode_data(string content);
    string construct_data(string qr_str, string ec_str);

    void format_info(int data_mask);
    void version_info();
    void data_pattern(string qr_str);

    int data_mask_score();
    int data_mask_evaluation();
    void data_mask_pattern(int data_mask);

    bool is_light_row(int row, int start_col);
    bool is_light_col(int col, int start_row);

public:
    QR(string content, char *path, int ec_level = LEVEL_M, int version = AUTO_VERSION);
};

#endif /* _QR_H_ */