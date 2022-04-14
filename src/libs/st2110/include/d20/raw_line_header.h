/**
 * st2110-20 header struct
 *
 */
#pragma once

#include <cinttypes>

namespace seeder { namespace d20 {

    // from https://tools.ietf.org/html/rfc4175
    //
    // 0                               16                             32
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |   Extended Sequence Number    |            Length             |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |F|          Line No            |C|           Offset            |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |            Length             |F|          Line No            |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |C|           Offset            |                               .
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               .
    // .                                                               .
    // .                 Two (partial) lines of video data             .
    // .                                                               .
    // +---------------------------------------------------------------+
    struct raw_extended_sequence_number
    {
        uint16_t esn;
    };

    struct raw_line_header
    {
        uint16_t length;

        uint8_t line_no_1 : 7;
        uint8_t field_identification : 1;

        uint8_t line_no_0;

        uint8_t offset_1 : 7;
        uint8_t continuation : 1;

        uint8_t offset_0;
    };

}} // namespace seeder::d20
