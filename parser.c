
#include "parser.h"

#include "hw_layout.h"
#include "gcode.h"

#include "strings.h"
#include "asm.h"

#include <string.h> // for memset


#define LOG_PARSE_ERROR(MSG)  {LOG_STRING("P: ERROR: " MSG " in " __FILE__ ":");LOG_U16(__LINE__);LOG_NEWLINE;state = ERROR_STATE;return FALSE;}



/* Parses a stream of characters
 * and split into (whitespace separated) words starting with a capital Letter
 * if there is a number after the letter, store this as well
 * if an end-of-line is found, process_command is called
 *
 * comments are handled correctly. (both () and ;)
 * checksums are ingored. Nxxxxx Tokens (line number) is ignored here as well.
 *
 * Extensions:
 * - all letters are supported (NIST requires fewer than 26)
 * - after $ a base64 encoded string is accepted (for laser_raster_data)
 * - fixed-point numbers (scaled integers) are used to parse the numbers
 *   numbers need to be given in normal notation, no exponent-handling!
 */

typedef enum {
    EXPECT_FIRST_LETTER = 0,
    EXPECT_LETTER,
    EXPECT_NUMBER_OR_SIGN,
    EXPECT_FIRST_DIGIT,    // must be a digit
    EXPECT_ANOTHERDIGIT,        // digit, '.' or  whitespace
    EXPECT_SUBDIGITS,        // digit, or whitespace
    EXPECT_BASE64_1,    // expect first char of a base64-string
    EXPECT_BASE64_2,    // expect second char of a base64-string
    EXPECT_BASE64_3,    // expect third char of a base64-string (may be '=')
    EXPECT_BASE64_4,    // expect fourth char of a base64-string (may be '=')
    ERROR_STATE,    // some error occured. ignore untile EOL and do not precess_command()
    COMMENT_MODE,    // inside comment mode ()
    IGNORE_REST,    // after a ; (comment until end of line)
    PARSE_CHECKSUM, // after a '*': parse digits of the checksum
    PARSE_FILENAME, // Characters which must be a filename
    PARSE_FILENAME_TICKS, // parse filename inside single ticks ''
    PARSE_FILENAME_DOUBLETICKS, // parse filename inside double ticks ""

    NUM_PARSE_STATES
} parse_state;


static uint8_t last_letter;
uint32_t codes_seen; // A is at bit 0
uint32_t numbers_got; // A is at bit 0
// result storage
number numbers[26]; // scaled integers
int32_t integers[26]; // integer part of ^^
// intermediate results
static uint32_t current_int;


// new style
static parse_state state = EXPECT_FIRST_LETTER;
static bool isNegative = FALSE;
static uint8_t digits = 0; // how many digits (before any '.') are processed
static uint8_t subdigits = 0; // how many digits (after any '.') are processed
uint8_t base64_bytes[LASER_MAX_RASTER_LINE];
uint8_t base64_len = 0; // amount of valid bytes pushed into LASER_RASTERDATA BUFFER
#define MAX_FILENAME_LEN 16
char filename[MAX_FILENAME_LEN]; // filename for M20..M33 commands
uint8_t filename_len = 0; // amount of valid characters in filename


// Base64 decoding helper (alphabet see https://de.wikipedia.org/wiki/Base64 )
// returns index into
// "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
// instead of '+' also '-' may occur
// instead of '/' also '_' may occur
//
// 000000 A
// 000001 B
// 000010 C
// 000011 D
// 000100 E
// 000101 F
// 000110 G
// 000111 H
// 001000 I
// 001001 J
// 001010 K
// 001011 L
// 001100 M
// 001101 N
// 001110 O
// 001111 P
// 010000 Q
// 010001 R
// 010010 S
// 010011 T
// 010100 U
// 010101 V
// 010110 W
// 010111 X
// 011000 Y
// 011001 Z
// 011010 a
// 011011 b
// 011100 c
// 011101 d
// 011110 e
// 011111 f
// 100000 g
// 100001 h
// 100010 i
// 100011 j
// 100100 k
// 100101 l
// 100110 m
// 100111 n
// 101000 o
// 101001 p
// 101010 q
// 101011 r
// 101100 s
// 101101 t
// 101110 u
// 101111 v
// 110000 w
// 110001 x
// 110010 y
// 110011 z
// 110100 0
// 110101 1
// 110110 2
// 110111 3
// 111000 4
// 111001 5
// 111010 6
// 111011 7
// 111100 8
// 111101 9
// 111110 +-
// 111111 /_



uint8_t base64_value(char c) {
    if (('A' <= c) && (c <= 'Z')) return (uint8_t) c - 'A';
    if (('a' <= c) && (c <= 'z')) return (uint8_t) c - 'a' + 26;
    if (('0' <= c) && (c <= '9')) return (uint8_t) c - '0' + 52;
    if ('+' == c) return (uint8_t) 62;
    if ('-' == c) return (uint8_t) 62;
    if ('/' == c) return (uint8_t) 63;
    if ('_' == c) return (uint8_t) 63;
    return 255; // mark error
}

// parses a line and calls process_cmd if end-of-line found
// atm no checksums are checked or properly ignored!
// return TRUE if OK, FALSE if parse error

bool process_char(char c) {
    bool isDigit = (('0'<=c) && (c<='9'));
    bool isWhite = ((' ' == c)||('\t'==c));
    uint8_t b = 0;

    // ignore '\r'
    if (('\r' == c))
        return TRUE;
    // at end-of-line, command is complete => call process_command
    if (('\n' == c)) {
#ifdef DEBUG
        uint8_t i;
        for(i=0;i<31;i++) {
            if (codes_seen & ((uint32_t)1 << i)) {
                if (numbers_got & ((uint32_t)1 << i)) {
                    LOG_STRING("P: TOKEN ");LOG_CHAR('A'+i);LOG_S32(numbers[i]);LOG_NEWLINE;
                } else {
                    LOG_STRING("P: TOKEN ");LOG_CHAR('A'+i);LOG_NEWLINE;
                }
            }
        }
#endif
        if (base64_len) {
            LOG_STRING("P: TOKEN $ (");LOG_U8(base64_len);LOG_STRING("Bytes)\n");
            // if Stepper queue is empty, transfer modulation data now, else later
            if (STEPPER_QUEUE_is_empty()) {
                for(b=0;b<base64_len;b++)
                    LASER_RASTERDATA_put(base64_bytes[b]);
                base64_len=b=0;
            }
        }
        if (state != ERROR_STATE) {
            LOG_STRING("P: PROCESS COMMAND\n");
            process_command();
            // if stepper queue was not empty before, transfer modulation now
            if(base64_len) {
                for(b=0;b<base64_len;b++)
                    LASER_RASTERDATA_put(base64_bytes[b]);
                base64_len=0;
            }
        } else
            LOG_STRING("P: ERROR-STATE: IGNORE COMMAND!\n");
        // re-init parser
        codes_seen = 0;
        numbers_got = 0;
        state = EXPECT_FIRST_LETTER;
        return state != ERROR_STATE;
    }
    // XXX update checksum

    // state dependent interpretation of characters
    switch(state) {
        case EXPECT_FIRST_LETTER:
            codes_seen = 0;
            numbers_got = 0;
            memset(numbers, 0, sizeof(numbers));
            memset(integers, 0, sizeof(integers));
            memset(base64_bytes, 0, sizeof(base64_bytes));
            memset(filename, 0, sizeof(filename));
            filename_len = 0;
            base64_len = 0;
            state = EXPECT_LETTER;
            // intentionally no break !
        case EXPECT_LETTER:
            if ((('A'<=c) && (c<='Z'))) {
                last_letter = c-'A';
                codes_seen |= ((uint32_t)1) << last_letter;
                state = EXPECT_NUMBER_OR_SIGN;
                return TRUE;
            } else if (isWhite) { // ignore whitespace
                return TRUE;
            } else if ('*' == c) {
                state = PARSE_CHECKSUM;
                return TRUE;
            } else if (';' == c) {
                state = IGNORE_REST;
                return TRUE;
            } else if ('(' == c) {
                state = COMMENT_MODE;
                return TRUE;
            } else if ('$' == c) {
                state = EXPECT_BASE64_1;
                return TRUE;
            } else if ('\'' == c) {
                state = PARSE_FILENAME_TICKS;
                return TRUE;
            } else if ('"' == c) {
                state = PARSE_FILENAME_DOUBLETICKS;
                return TRUE;
            } else if (!filename_len) {
                state = PARSE_FILENAME;
                filename[filename_len++] = c;
                filename[filename_len] = 0;
                return TRUE;
            }
            LOG_PARSE_ERROR("unexpected character and filename already set!");
            break;
        case EXPECT_NUMBER_OR_SIGN:
            if (isWhite) {
                state = EXPECT_LETTER;
                return TRUE;
            } else if (!(isDigit || (c == '+') || (c=='-'))) {
                if (filename_len) {
                    LOG_PARSE_ERROR("filename already set: unexpected character found");
                }
                state = PARSE_FILENAME;
                filename[filename_len++] = last_letter + 'A';
                filename[filename_len++] = c;
                filename[filename_len] = 0;
                return TRUE;
            }
            state = EXPECT_FIRST_DIGIT;
            current_int = 0;
            digits = 0;
            subdigits = 0;
            if (c == '-') {
                isNegative = TRUE;
                return TRUE;
            }
            isNegative = FALSE;
            if (c == '+')    // needless, but valid
                return TRUE;
            // intentionally no break!
        case EXPECT_FIRST_DIGIT:    // first digit of a number
            if (isWhite) {
                state = EXPECT_LETTER;
                return TRUE;
            } else if (!isDigit)
                LOG_PARSE_ERROR("Expected [0-9\\w]");
            current_int = (uint8_t) c - '0';
            digits++;
            state = EXPECT_ANOTHERDIGIT;
            // fall through to number storage
            break;
        case EXPECT_ANOTHERDIGIT: // digits of a number before '.' or 'eE'
            if (isDigit) {
                if (digits>9)
                    LOG_PARSE_ERROR("Too many leading digits!");
                times_ten(current_int);
                current_int += (uint8_t) (c - '0');
                digits++;
                // fall through to number storage
                break;
            } else if ('.' == c) {
                state = EXPECT_SUBDIGITS;
                break;
            } else if (isWhite) {
                state = EXPECT_LETTER;
                break;
            } else if ('*' == c) {
                state = PARSE_CHECKSUM;
                break;
            } else if (';' == c) {
                state = IGNORE_REST;
                break;
            } else if ('(' == c) {
                state = COMMENT_MODE;
                break;
            } else if ('$' == c) {
                state = EXPECT_BASE64_1;
                break;
            } else if ('\'' == c) {
                state = PARSE_FILENAME_TICKS;
                break;
            } else if ('"' == c) {
                state = PARSE_FILENAME_DOUBLETICKS;
                break;
            } else
                LOG_PARSE_ERROR("Expected [0-9.\\w]");

        case EXPECT_SUBDIGITS:    // digits of a number after '.'
            if (isDigit) {
                if (subdigits >= SCALE_DIGITS) // ignore further digits
                    return TRUE;
                if (digits+subdigits > 9) //capacity exceeded!
                    //~ LOG_PARSE_ERROR("Too many total digits!");
                    return TRUE; // ignore further digits
                times_ten(current_int);
                current_int += (uint8_t) (c - '0');
                subdigits++;
                // fall through to number storage
                break;
            } else if (isWhite) {
                state = EXPECT_LETTER;
                return TRUE;
            } else
                LOG_PARSE_ERROR("Expected [0-9\\w]");
        case EXPECT_BASE64_1:    // expect first char of a base64-string
            if (isWhite) {
                state = EXPECT_LETTER;
                return TRUE;
            }
            b = base64_value(c);
            if (b > 63) {
                state = IGNORE_REST;
                LOG_PARSE_ERROR("Expected a BASE64 character");
            }
            base64_bytes[base64_len] = b<<2;
            state = EXPECT_BASE64_2;
            return TRUE;
        case EXPECT_BASE64_2:    // expect second char of a base64-string
            b = base64_value(c);
            if (b > 63) {
                state = IGNORE_REST;
                LOG_PARSE_ERROR("Expected a BASE64 character");
            }
            base64_bytes[base64_len++] |= (b >> 4);
            if (base64_len >= sizeof(base64_bytes)) {
                state = IGNORE_REST;
                LOG_PARSE_ERROR("Too many Base64 Bytes (Buffer full)");
            }
            base64_bytes[base64_len] = (b << 4);
            state = EXPECT_BASE64_3;
            return TRUE;
        case EXPECT_BASE64_3:    // expect third char of a base64-string (may be '=')
            if ('=' != c) {
                b = base64_value(c);
                if (b > 63) {
                    state = IGNORE_REST;
                    LOG_PARSE_ERROR("Expected a BASE64 character");
                }
                base64_bytes[base64_len++] |= (b >> 2);
                if (base64_len >= sizeof(base64_bytes)) {
                    state = IGNORE_REST;
                    LOG_PARSE_ERROR("Too many Base64 Bytes (Buffer full)");
                }
                base64_bytes[base64_len] = (b << 6);
            }
            state = EXPECT_BASE64_4;
            return TRUE;
        case EXPECT_BASE64_4:    // expect fourth char of a base64-string (may be '=')
            if ('=' != c) {
                b = base64_value(c);
                if (b > 63) {
                    state = IGNORE_REST;
                    LOG_PARSE_ERROR("Expected a BASE64 character");
                }
                base64_bytes[base64_len++] |= b;
                if (base64_len >= sizeof(base64_bytes)) {
                    state = IGNORE_REST;
                    LOG_PARSE_ERROR("Too many Base64 Bytes (Buffer full)");
                }
            }
            state = EXPECT_BASE64_1;
            return TRUE;
        case COMMENT_MODE:    // inside comment mode ()
            // just eat everything between '(' and ')'
            if (c == ')') {
                state = EXPECT_LETTER;
            }
            return TRUE;
        case ERROR_STATE: // after an error, ignore until end of line and do not process_command at eol!
            return FALSE;
        case PARSE_CHECKSUM: // after a '*': parse digits of the checksum (untile end of line)
            // ignored.
        case IGNORE_REST:    // after a ; (comment until end of line)
            // just eat everything after a ';'
            return TRUE;
        case PARSE_FILENAME_DOUBLETICKS: // parse filename inside double ticks ""
            if ('"' == c) {
                state = EXPECT_LETTER;
                return TRUE;
            }
            b = filename_len;
            filename[b++] = c;
            filename[b] = 0;
            filename_len = b;
            return (filename_len < sizeof(filename));
        case PARSE_FILENAME_TICKS: // parse filename inside single ticks ''
            if ('\'' == c) {
                state = EXPECT_LETTER;
                return TRUE;
            }
            b = filename_len;
            filename[b++] = c;
            filename[b] = 0;
            filename_len = b;
            return (filename_len < sizeof(filename));
        case PARSE_FILENAME: // Characters which must be a filename
            if (isWhite) {
                state = EXPECT_LETTER;
                return TRUE;
            }
            b = filename_len;
            filename[b++] = c;
            filename[b] = 0;
            filename_len = b;
            return (filename_len < sizeof(filename));

        default:
            LOG_PARSE_ERROR("Unknown or undefined State");
    }
    //~ DEBUG("have %d digits and %d subdigits", digits, subdigits);
    //~ LOG_STRING("have digits:subdigits = ");LOG_U8(digits);
    //~ LOG_COMMA;LOG_U8(subdigits);LOG_NEWLINE;

    {
        int32_t myint = current_int;
        uint8_t subdigs = subdigits;
        // goal is an integer in units of 10**-SCALE_DIGITS
        while (subdigs < SCALE_DIGITS) {
            // append '0' subdigits until right exponent
            if (myint > 429496729) // would overflow!
                LOG_PARSE_ERROR("Number overflow!");
            //~ times_ten(myint);
            myint *= 10;
            subdigs++;
        }
        while (subdigs > SCALE_DIGITS) {
            // remove subdigits until right exponent
            myint +=5; // rounding!
            myint /= 10;
            subdigs--;
        }

        //~ DEBUG("current_int is now %ld", myint);
        if (myint > 2147483647)
            LOG_PARSE_ERROR("Number overflow!"); // would overflow when utilising sign!
        if (isNegative)
            myint = -myint;
        // store at right location
        numbers[last_letter] = myint;
        integers[last_letter] = myint / ONE;
        numbers_got |= ((uint32_t)1) << last_letter;
        //~ DEBUG("%c_Number= %f, %u, %u", (int) ('A'+last_letter), numbers[last_letter], integers[last_letter]);
        //~ LOG_STRING("NUMBER");avrtest_putchar('A'+last_letter);
        //~ LOG_S32(numbers[last_letter]);
        //~ LOG_S32(integers[last_letter]);avrtest_putchar('\n');

    }
    return TRUE;
}

// set up initial conditions with a G28 command (homing)
void parser_init(void) {
    state = EXPECT_FIRST_LETTER;
    process_char('F');
    process_char('3');
    process_char('0');
    process_char('0');
    process_char('\n');
    if (!SIM_ACTIVE) {
        process_char('G');
        process_char('2');
        process_char('8');
        process_char('\n');
    }
    LOG_STRING("P: init done\n");
}

#if 0
void ClearToSend(void)
{
  previous_millis_cmd = millis();
  #ifdef SDSUPPORT
  if(fromsd[bufindr])
    return;
  #endif //SDSUPPORT
  //~ SERIAL_PROTOCOLLNPGM(MSG_OK);
}

void FlushSerialRequestResend(void)
{
  //uint8_t cmdbuffer[bufindr][100]="Resend:";
  //~ MYSERIAL.flush();
  //~ SERIAL_PROTOCOLPGM(MSG_RESEND);
  //~ SERIAL_PROTOCOLLN(gcode_LastN + 1);
  ClearToSend();
}
#endif
