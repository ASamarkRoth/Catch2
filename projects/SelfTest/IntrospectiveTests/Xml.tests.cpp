#include "catch.hpp"
#include "internal/catch_xmlwriter.h"

#include <sstream>

inline std::string encode( std::string const& str, Catch::XmlEncode::ForWhat forWhat = Catch::XmlEncode::ForTextNodes ) {
    std::ostringstream oss;
    oss << Catch::XmlEncode( str, forWhat );
    return oss.str();
}

TEST_CASE( "XmlEncode", "[XML]" ) {
    SECTION( "normal string" ) {
        REQUIRE( encode( "normal string" ) == "normal string" );
    }
    SECTION( "empty string" ) {
        REQUIRE( encode( "" ) == "" );
    }
    SECTION( "string with ampersand" ) {
        REQUIRE( encode( "smith & jones" ) == "smith &amp; jones" );
    }
    SECTION( "string with less-than" ) {
        REQUIRE( encode( "smith < jones" ) == "smith &lt; jones" );
    }
    SECTION( "string with greater-than" ) {
        REQUIRE( encode( "smith > jones" ) == "smith > jones" );
        REQUIRE( encode( "smith ]]> jones" ) == "smith ]]&gt; jones" );
    }
    SECTION( "string with quotes" ) {
        std::string stringWithQuotes = "don't \"quote\" me on that";
        REQUIRE( encode( stringWithQuotes ) == stringWithQuotes );
        REQUIRE( encode( stringWithQuotes, Catch::XmlEncode::ForAttributes ) == "don't &quot;quote&quot; me on that" );
    }
    SECTION( "string with control char (1)" ) {
        REQUIRE( encode( "[\x01]" ) == "[\\x01]" );
    }
    SECTION( "string with control char (x7F)" ) {
        REQUIRE( encode( "[\x7F]" ) == "[\\x7F]" );
    }
}

// Thanks to Peter Bindels (dascandy) for some of the tests
TEST_CASE("XmlEncode: UTF-8", "[xml][utf-8]") {
    SECTION("Valid utf-8 strings") {
        CHECK(encode(u8"Here be 👾") == u8"Here be 👾");
        CHECK(encode(u8"šš") == u8"šš");

        CHECK(encode(u8"\xDF\xBF")         == u8"\xDF\xBF"); // 0x7FF
        CHECK(encode(u8"\xE0\xA0\x80")     == u8"\xE0\xA0\x80"); // 0x800
        CHECK(encode(u8"\xED\x9F\xBF")     == u8"\xED\x9F\xBF"); // 0xD7FF
        CHECK(encode(u8"\xEE\x80\x80")     == u8"\xEE\x80\x80"); // 0xE000
        CHECK(encode(u8"\xEF\xBF\xBF")     == u8"\xEF\xBF\xBF"); // 0xFFFF
        CHECK(encode(u8"\xF0\x90\x80\x80") == u8"\xF0\x90\x80\x80"); // 0x10000
        CHECK(encode(u8"\xF4\x8F\xBF\xBF") == u8"\xF4\x8F\xBF\xBF"); // 0x10FFFF
    }
    SECTION("Invalid utf-8 strings") {

        SECTION("Various broken strings") {
            CHECK(encode(u8"Here \xFF be 👾") == u8"Here \\xFF be 👾");
            CHECK(encode(u8"\xFF") == "\\xFF");
            CHECK(encode(u8"\xF4\x90\x80\x80") == u8"\\xF4\\x90\\x80\\x80");
            CHECK(encode(u8"\xC5\xC5\xA0") == u8"\\xC5Š");
            CHECK(encode(u8"\xF4\x90\x80\x80") == u8"\\xF4\\x90\\x80\\x80"); // 0x110000 -- out of unicode range
        }

        SECTION("Overlong encodings") {
            CHECK(encode(u8"\xC0\x80") == u8"\\xC0\\x80"); // \0
            CHECK(encode(u8"\xF0\x80\x80\x80") == u8"\\xF0\\x80\\x80\\x80"); // Super-over-long \0
            CHECK(encode(u8"\xC1\xBF") == u8"\\xC1\\xBF"); // ASCII char as UTF-8 (0x7F)
            CHECK(encode(u8"\xE0\x9F\xBF") == u8"\\xE0\\x9F\\xBF"); // 0x7FF
            CHECK(encode(u8"\xF0\x8F\xBF\xBF") == u8"\\xF0\\x8F\\xBF\\xBF"); // 0xFFFF
        }

        // Note that we actually don't modify surrogate pairs, as we do not do strict checking
        SECTION("Surrogate pairs") {
            CHECK(encode(u8"\xED\xA0\x80") == u8"\xED\xA0\x80"); // Invalid surrogate half 0xD800
            CHECK(encode(u8"\xED\xAF\xBF") == u8"\xED\xAF\xBF"); // Invalid surrogate half 0xDBFF
            CHECK(encode(u8"\xED\xB0\x80") == u8"\xED\xB0\x80"); // Invalid surrogate half 0xDC00
            CHECK(encode(u8"\xED\xBF\xBF") == u8"\xED\xBF\xBF"); // Invalid surrogate half 0xDFFF        }
        }

        SECTION("Invalid start byte") {
            CHECK(encode(u8"\x80") == u8"\\x80");
            CHECK(encode(u8"\x81") == u8"\\x81");
            CHECK(encode(u8"\xBC") == u8"\\xBC");
            CHECK(encode(u8"\xBF") == u8"\\xBF");
            // Out of range
            CHECK(encode(u8"\xF5\x80\x80\x80") == u8"\\xF5\\x80\\x80\\x80");
            CHECK(encode(u8"\xF6\x80\x80\x80") == u8"\\xF6\\x80\\x80\\x80");
            CHECK(encode(u8"\xF7\x80\x80\x80") == u8"\\xF7\\x80\\x80\\x80");
        }

        SECTION("Missing continuation byte(s)") {
            // Missing first continuation byte
            CHECK(encode(u8"\xDE") == u8"\\xDE");
            CHECK(encode(u8"\xDF") == u8"\\xDF");
            CHECK(encode(u8"\xE0") == u8"\\xE0");
            CHECK(encode(u8"\xEF") == u8"\\xEF");
            CHECK(encode(u8"\xF0") == u8"\\xF0");
            CHECK(encode(u8"\xF4") == u8"\\xF4");

            // Missing second continuation byte
            CHECK(encode(u8"\xE0\x80") == u8"\\xE0\\x80");
            CHECK(encode(u8"\xE0\xBF") == u8"\\xE0\\xBF");
            CHECK(encode(u8"\xE1\x80") == u8"\\xE1\\x80");
            CHECK(encode(u8"\xF0\x80") == u8"\\xF0\\x80");
            CHECK(encode(u8"\xF4\x80") == u8"\\xF4\\x80");

            // Missing third continuation byte
            CHECK(encode(u8"\xF0\x80\x80") == u8"\\xF0\\x80\\x80");
            CHECK(encode(u8"\xF4\x80\x80") == u8"\\xF4\\x80\\x80");
        }
    }
}
