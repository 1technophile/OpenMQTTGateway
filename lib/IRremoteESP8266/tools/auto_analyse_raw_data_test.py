#!/usr/bin/python
"""Unit tests for auto_analyse_raw_data.py"""
import StringIO
import unittest
import auto_analyse_raw_data as analyse


class TestRawIRMessage(unittest.TestCase):
  """Unit tests for the RawIRMessage class."""

  def test_display_binary(self):
    """Test the display_binary() method."""
    output = StringIO.StringIO()
    message = analyse.RawIRMessage(100, [8000, 4000, 500, 500, 500], output,
                                   False)
    self.assertEqual(output.getvalue(), '')
    message.display_binary("10101010")
    message.display_binary("0000000000000000")
    message.display_binary("00010010001101000101011001111000")
    self.assertEqual(output.getvalue(), '\n'
                     '  Bits: 8\n'
                     '  Hex:  0xAA (MSB first)\n'
                     '        0x55 (LSB first)\n'
                     '  Dec:  170 (MSB first)\n'
                     '        85 (LSB first)\n'
                     '  Bin:  0b10101010 (MSB first)\n'
                     '        0b01010101 (LSB first)\n'
                     '\n'
                     '  Bits: 16\n'
                     '  Hex:  0x0000 (MSB first)\n'
                     '        0x0000 (LSB first)\n'
                     '  Dec:  0 (MSB first)\n'
                     '        0 (LSB first)\n'
                     '  Bin:  0b0000000000000000 (MSB first)\n'
                     '        0b0000000000000000 (LSB first)\n'
                     '\n'
                     '  Bits: 32\n'
                     '  Hex:  0x12345678 (MSB first)\n'
                     '        0x1E6A2C48 (LSB first)\n'
                     '  Dec:  305419896 (MSB first)\n'
                     '        510274632 (LSB first)\n'
                     '  Bin:  0b00010010001101000101011001111000 (MSB first)\n'
                     '        0b00011110011010100010110001001000 (LSB first)\n')


class TestAutoAnalyseRawData(unittest.TestCase):
  """Unit tests for the functions in AutoAnalyseRawData."""

  def test_dump_constants_simple(self):
    """Simple tests for the dump_constants() function."""
    ignore = StringIO.StringIO()
    output = StringIO.StringIO()
    defs = []
    message = analyse.RawIRMessage(200, [
        7930, 3952, 494, 1482, 520, 1482, 494, 1508, 494, 520, 494, 1482, 494,
        520, 494, 1482, 494, 1482, 494, 3978, 494, 520, 494, 520, 494, 520, 494,
        520, 520, 520, 494, 520, 494, 520, 494, 1482, 494
    ], ignore)
    analyse.dump_constants(message, defs, output)
    self.assertEqual(defs, [
        '#define HDR_MARK 7930U', '#define BIT_MARK 520U',
        '#define HDR_SPACE 3978U', '#define ONE_SPACE 1508U',
        '#define ZERO_SPACE 520U'
    ])
    self.assertEqual(output.getvalue(), 'Guessing key value:\n'
                     'HDR_MARK   = 7930\n'
                     'HDR_SPACE  = 3978\n'
                     'BIT_MARK   = 520\n'
                     'ONE_SPACE  = 1508\n'
                     'ZERO_SPACE = 520\n')

  def test_dump_constants_aircon(self):
    """More complex tests for the dump_constants() function."""
    ignore = StringIO.StringIO()
    output = StringIO.StringIO()
    defs = []
    message = analyse.RawIRMessage(200, [
        9008, 4496, 644, 1660, 676, 530, 648, 558, 672, 1636, 646, 1660, 644,
        556, 650, 584, 626, 560, 644, 580, 628, 1680, 624, 560, 648, 1662, 644,
        582, 648, 536, 674, 530, 646, 580, 628, 560, 670, 532, 646, 562, 644,
        556, 672, 536, 648, 1662, 646, 1660, 652, 554, 644, 558, 672, 538, 644,
        560, 668, 560, 648, 1638, 668, 536, 644, 1660, 668, 532, 648, 560, 648,
        1660, 674, 554, 622, 19990, 646, 580, 624, 1660, 648, 556, 648, 558,
        674, 556, 622, 560, 644, 564, 668, 536, 646, 1662, 646, 1658, 672, 534,
        648, 558, 644, 562, 648, 1662, 644, 584, 622, 558, 648, 562, 668, 534,
        670, 536, 670, 532, 672, 536, 646, 560, 646, 558, 648, 558, 670, 534,
        650, 558, 646, 560, 646, 560, 668, 1638, 646, 1662, 646, 1660, 646,
        1660, 648
    ], ignore)
    analyse.dump_constants(message, defs, output)
    self.assertEqual(defs, [
        '#define HDR_MARK 9008U', '#define BIT_MARK 676U',
        '#define HDR_SPACE 4496U', '#define ONE_SPACE 1680U',
        '#define ZERO_SPACE 584U', '#define SPACE_GAP = 19990U'
    ])
    self.assertEqual(output.getvalue(), 'Guessing key value:\n'
                     'HDR_MARK   = 9008\n'
                     'HDR_SPACE  = 4496\n'
                     'BIT_MARK   = 676\n'
                     'ONE_SPACE  = 1680\n'
                     'ZERO_SPACE = 584\n'
                     'SPACE_GAP = 19990\n')

  def test_convert_rawdata(self):
    """Tests for the convert_rawdata() function."""
    # trivial cases
    self.assertEqual(analyse.convert_rawdata("0"), [0])
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("")
    self.assertEqual(context.exception.message,
                     "Raw Data contains a non-numeric value of ''.")

    # Single parenthesis
    self.assertEqual(analyse.convert_rawdata("foo {10"), [10])
    self.assertEqual(analyse.convert_rawdata("20} bar"), [20])

    # No parentheses
    self.assertEqual(analyse.convert_rawdata("10,20 ,  30"), [10, 20, 30])

    # Dual parentheses
    self.assertEqual(analyse.convert_rawdata("{10,20 ,  30}"), [10, 20, 30])
    self.assertEqual(analyse.convert_rawdata("foo{10,20}bar"), [10, 20])

    # Many parentheses
    self.assertEqual(analyse.convert_rawdata("foo{10,20}{bar}"), [10, 20])
    self.assertEqual(analyse.convert_rawdata("foo{10,20}{bar}}{"), [10, 20])

    # Bad parentheses
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("}10{")
    self.assertEqual(context.exception.message,
                     "Raw Data not parsible due to parentheses placement.")

    # Non base-10 values
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("10, 20, foo, bar, 30")
    self.assertEqual(context.exception.message,
                     "Raw Data contains a non-numeric value of 'foo'.")

    # A messy usual "good" case.
    input_str = """uint16_t rawbuf[6] = {
    9008, 4496, 644,
    1660, 676,

    530}
    ;"""
    self.assertEqual(
        analyse.convert_rawdata(input_str), [9008, 4496, 644, 1660, 676, 530])

  def test_parse_and_report(self):
    """Tests for the parse_and_report() function."""

    # Without code generation.
    output = StringIO.StringIO()
    input_str = """
        uint16_t rawbuf[139] = {9008, 4496, 644, 1660, 676, 530, 648, 558, 672,
            1636, 646, 1660, 644, 556, 650, 584, 626, 560, 644, 580, 628, 1680,
            624, 560, 648, 1662, 644, 582, 648, 536, 674, 530, 646, 580, 628,
            560, 670, 532, 646, 562, 644, 556, 672, 536, 648, 1662, 646, 1660,
            652, 554, 644, 558, 672, 538, 644, 560, 668, 560, 648, 1638, 668,
            536, 644, 1660, 668, 532, 648, 560, 648, 1660, 674, 554, 622, 19990,
            646, 580, 624, 1660, 648, 556, 648, 558, 674, 556, 622, 560, 644,
            564, 668, 536, 646, 1662, 646, 1658, 672, 534, 648, 558, 644, 562,
            648, 1662, 644, 584, 622, 558, 648, 562, 668, 534, 670, 536, 670,
            532, 672, 536, 646, 560, 646, 558, 648, 558, 670, 534, 650, 558,
            646, 560, 646, 560, 668, 1638, 646, 1662, 646, 1660, 646, 1660,
            648};"""
    analyse.parse_and_report(input_str, 200, False, output)
    self.assertEqual(
        output.getvalue(), 'Found 139 timing entries.\n'
        'Potential Mark Candidates:\n'
        '[9008, 676]\n'
        'Potential Space Candidates:\n'
        '[19990, 4496, 1680, 584]\n'
        '\n'
        'Guessing encoding type:\n'
        'Looks like it uses space encoding. Yay!\n'
        '\n'
        'Guessing key value:\n'
        'HDR_MARK   = 9008\n'
        'HDR_SPACE  = 4496\n'
        'BIT_MARK   = 676\n'
        'ONE_SPACE  = 1680\n'
        'ZERO_SPACE = 584\n'
        'SPACE_GAP = 19990\n'
        '\n'
        'Decoding protocol based on analysis so far:\n'
        '\n'
        'HDR_MARK+HDR_SPACE+10011000010100000000011000001010010 GAP(19990)\n'
        '  Bits: 35\n'
        '  Hex:  0x4C2803052 (MSB first)\n'
        '        0x250600A19 (LSB first)\n'
        '  Dec:  20443050066 (MSB first)\n'
        '        9938405913 (LSB first)\n'
        '  Bin:  0b10011000010100000000011000001010010 (MSB first)\n'
        '        0b01001010000011000000000101000011001 (LSB first)\n'
        'BIT_MARK(UNEXPECTED)01000000110001000000000000001111\n'
        '  Bits: 32\n'
        '  Hex:  0x40C4000F (MSB first)\n'
        '        0xF0002302 (LSB first)\n'
        '  Dec:  1086586895 (MSB first)\n'
        '        4026540802 (LSB first)\n'
        '  Bin:  0b01000000110001000000000000001111 (MSB first)\n'
        '        0b11110000000000000010001100000010 (LSB first)\n'
        'Total Nr. of suspected bits: 67\n')

    # With code generation.
    output = StringIO.StringIO()
    input_str = """
        uint16_t rawbuf[37] = {7930, 3952, 494, 1482, 520, 1482, 494,
            1508, 494, 520, 494, 1482, 494, 520, 494, 1482, 494, 1482, 494,
            3978, 494, 520, 494, 520, 494, 520, 494, 520, 520, 520, 494, 520,
            494, 520, 494, 1482, 494};"""
    analyse.parse_and_report(input_str, 200, True, output)
    self.assertEqual(
        output.getvalue(), 'Found 37 timing entries.\n'
        'Potential Mark Candidates:\n'
        '[7930, 520]\n'
        'Potential Space Candidates:\n'
        '[3978, 1508, 520]\n'
        '\n'
        'Guessing encoding type:\n'
        'Looks like it uses space encoding. Yay!\n'
        '\n'
        'Guessing key value:\n'
        'HDR_MARK   = 7930\n'
        'HDR_SPACE  = 3978\n'
        'BIT_MARK   = 520\n'
        'ONE_SPACE  = 1508\n'
        'ZERO_SPACE = 520\n'
        '\n'
        'Decoding protocol based on analysis so far:\n'
        '\n'
        'HDR_MARK+HDR_SPACE+11101011\n'
        '  Bits: 8\n'
        '  Hex:  0xEB (MSB first)\n'
        '        0xD7 (LSB first)\n'
        '  Dec:  235 (MSB first)\n'
        '        215 (LSB first)\n'
        '  Bin:  0b11101011 (MSB first)\n'
        '        0b11010111 (LSB first)\n'
        'UNEXPECTED->HDR_SPACE+00000001\n'
        '  Bits: 8\n  Hex:  0x01 (MSB first)\n'
        '        0x80 (LSB first)\n'
        '  Dec:  1 (MSB first)\n'
        '        128 (LSB first)\n'
        '  Bin:  0b00000001 (MSB first)\n'
        '        0b10000000 (LSB first)\n'
        'Total Nr. of suspected bits: 16\n'
        '\n'
        'Generating a VERY rough code outline:\n'
        '\n'
        "// WARNING: This probably isn't directly usable. It's a guide only.\n"
        '#define HDR_MARK 7930U\n'
        '#define BIT_MARK 520U\n'
        '#define HDR_SPACE 3978U\n'
        '#define ONE_SPACE 1508U\n'
        '#define ZERO_SPACE 520U\n'
        '#define XYZ_BITS 16U\n'
        '// Function should be safe up to 64 bits.\n'
        'void IRsend::sendXYZ(const uint64_t data, const uint16_t nbits,'
        ' const uint16_t repeat) {\n'
        '  enableIROut(38);  // A guess. Most common frequency.\n'
        '  for (uint16_t r = 0; r <= repeat; r++) {\n'
        '    // Header\n'
        '    mark(HDR_MARK);\n'
        '    space(HDR_SPACE);\n'
        '    // Data\n'
        '    // e.g. data = 0xEB, nbits = 8\n'
        '    sendData(BIT_MARK, ONE_SPACE, BIT_MARK, ZERO_SPACE, data, nbits,'
        ' true);\n'
        '    // Footer\n'
        '    mark(BIT_MARK);\n'
        '    space(HDR_SPACE);\n'
        '    // Data\n'
        '    // e.g. data = 0x1, nbits = 8\n'
        '    sendData(BIT_MARK, ONE_SPACE, BIT_MARK, ZERO_SPACE, data, nbits,'
        ' true);\n'
        '    // Footer\n'
        '    mark(BIT_MARK);\n'
        '    space(100000);  // A 100% made up guess of the gap between'
        ' messages.\n'
        '  }\n'
        '}\n')


if __name__ == '__main__':
  unittest.main(verbosity=2)
