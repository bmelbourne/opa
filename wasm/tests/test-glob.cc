#include <unordered_map>

#include "glob-compiler.h"
#include "glob-lexer.h"
#include "glob-parser.h"
#include "malloc.h"
#include "test.h"
#include "re2/re2.h"
#include "std.h"
#include "glob.h"

WASM_EXPORT(test_glob_cache)
extern "C"
void test_glob_cache()
{
	// Reset the cache
	opa_builtin_cache_set(0, NULL);

	for (int i = 0; i < 100; i++)
	{
		char pattern[20];
		snprintf(pattern, sizeof(pattern), "foo/%d/*", i);
		opa_glob_match(opa_string_terminated(pattern), opa_null(), opa_string_terminated("foo/bar"));
	}

	std::unordered_map<void*, void*>* c = static_cast<std::unordered_map<void*, void*>*>(opa_builtin_cache_get(1));

	test("glob cache size", c->size() == 100);

	opa_glob_match(opa_string_terminated("bar/*"), opa_null(), opa_string_terminated("bar/baz"));

	test("glob cache size doesn't surpass max", c->size() == 100);
}

// The following is a re-implementation of tests in
// https://github.com/gobwas/glob/blob.
//
// The MIT License (MIT)
//
// Copyright (c) 2016 Sergey Kamardin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

WASM_EXPORT(test_glob_lexer)
extern "C"
void test_glob_lexer()
{
#define TEST(test_case, pattern, ...) {                                 \
        token expected[] = {__VA_ARGS__};                               \
        lexer *l = new lexer(pattern, strlen(pattern));                 \
        for (int i = 0; i < sizeof(expected)/sizeof(expected[0]); i++)  \
        {                                                               \
            token token(glob_lexer_token_eof, "", 0);                   \
            l->next(&token);                                            \
            test(test_case, token.kind == expected[i].kind &&           \
                 token.s == expected[i].s);                             \
        }                                                               \
        delete l;                                                       \
    }

    TEST("glob/lexer", "", {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hello", {glob_lexer_token_text, "hello", 5}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "/{rate,[0-9]]}*",
         {glob_lexer_token_text, "/", 1},
         {glob_lexer_token_terms_open, "{", 1},
         {glob_lexer_token_text, "rate", 4},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_range_open, "[", 1},
         {glob_lexer_token_range_lo, "0", 1},
         {glob_lexer_token_range_between, "-", 1},
         {glob_lexer_token_range_hi, "9", 1},
         {glob_lexer_token_range_close, "]", 1},
         {glob_lexer_token_text, "]", 1},
         {glob_lexer_token_terms_close, "}", 1},
         {glob_lexer_token_any, "*", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "hello,world", {glob_lexer_token_text, "hello,world", 11}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hello\\,world", {glob_lexer_token_text, "hello,world", 11}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hello\\{world", {glob_lexer_token_text, "hello{world", 11}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hello?", {glob_lexer_token_text, "hello", 5}, {glob_lexer_token_single, "?", 1}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hellof*", {glob_lexer_token_text, "hellof", 6}, {glob_lexer_token_any, "*", 1}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "hello**", {glob_lexer_token_text, "hello", 5}, {glob_lexer_token_super, "**", 2}, {glob_lexer_token_eof, "", 0});
    TEST("glob/lexer", "[日-語]",
         {glob_lexer_token_range_open, "[", 1},
         {glob_lexer_token_range_lo, "日", 3},
         {glob_lexer_token_range_between, "-", 1},
         {glob_lexer_token_range_hi, "語", 3},
         {glob_lexer_token_range_close, "]", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "[!日-語]",
         {glob_lexer_token_range_open, "[", 1},
         {glob_lexer_token_not, "!", 1},
         {glob_lexer_token_range_lo, "日", 3},
         {glob_lexer_token_range_between, "-", 1},
         {glob_lexer_token_range_hi, "語", 3},
         {glob_lexer_token_range_close, "]", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "[!日本語]",
         {glob_lexer_token_range_open, "[", 1},
         {glob_lexer_token_not, "!", 1},
         {glob_lexer_token_text, "日本語", 9},
         {glob_lexer_token_range_close, "]", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "{a,b}",
         {glob_lexer_token_terms_open, "{", 1},
         {glob_lexer_token_text, "a", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_text, "b", 1},
         {glob_lexer_token_terms_close, "}", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "/{z,ab}*",
         {glob_lexer_token_text, "/", 1},
         {glob_lexer_token_terms_open, "{", 1},
         {glob_lexer_token_text, "z", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_text, "ab", 2},
         {glob_lexer_token_terms_close, "}", 1},
         {glob_lexer_token_any, "*", 1},
         {glob_lexer_token_eof, "", 0},
         );
    TEST("glob/lexer", "{[!日-語],*,?,{a,b,\\c}}",
         {glob_lexer_token_terms_open, "{", 1},
         {glob_lexer_token_range_open, "[", 1},
         {glob_lexer_token_not, "!", 1},
         {glob_lexer_token_range_lo, "日", 3},
         {glob_lexer_token_range_between, "-", 1},
         {glob_lexer_token_range_hi, "語", 3},
         {glob_lexer_token_range_close, "]", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_any, "*", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_single, "?", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_terms_open, "{", 1},
         {glob_lexer_token_text, "a", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_text, "b", 1},
         {glob_lexer_token_separator, ",", 1},
         {glob_lexer_token_text, "c", 1},
         {glob_lexer_token_terms_close, "}", 1},
         {glob_lexer_token_terms_close, "}", 1},
         {glob_lexer_token_eof, "", 0},
         );
#undef TEST
}

WASM_EXPORT(test_glob_parser)
extern "C"
void test_glob_parser()
{
#define TEST(test_case, pattern, expected) {                            \
        node *e = expected;                                             \
        lexer *l = new lexer(pattern, strlen(pattern));                 \
        node *n = NULL;                                                 \
        std::string error = glob_parse(l, &n);                          \
        test(test_case, e->equal(n));                                   \
        delete n;                                                       \
        delete l;                                                       \
        delete e;                                                       \
    }

    TEST("glob/parser", "abc", (new node(kind_pattern))->
         insert(new node(kind_text, "abc")));
    TEST("glob/parser", "a*c",
         (new node(kind_pattern))->
         insert(new node(kind_text, "a"))->
         insert(new node(kind_any))->
         insert(new node(kind_text, "c")));
    TEST("glob/parser", "a**c",
         (new node(kind_pattern))->
         insert(new node(kind_text, "a"))->
         insert(new node(kind_super))->
         insert(new node(kind_text, "c")));
    TEST("glob/parser", "a?c",
         (new node(kind_pattern))->
         insert(new node(kind_text, "a"))->
         insert(new node(kind_single))->
         insert(new node(kind_text, "c")));
    TEST("glob/parser", "[!a-z]",
         (new node(kind_pattern))->
         insert(new node(kind_range, "a", "z", true)));
    TEST("glob/parser", "[az]",
         (new node(kind_pattern))->
         insert(new node(kind_list, "az", false)));
    TEST("glob/parser", "{a,z}",
         (new node(kind_pattern))->
         insert((new node(kind_any_of))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_text, "a")))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_text, "z")))));
    TEST("glob/parser", "/{z,ab}*",
         (new node(kind_pattern))->
         insert(new node(kind_text, "/"))->
         insert((new node(kind_any_of))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_text, "z")))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_text, "ab"))))->
         insert(new node(kind_any)));
    TEST("glob/parser", "{a,{x,y},?,[a-z],[!qwe]}",
         (new node(kind_pattern))->
         insert((new node(kind_any_of))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_text, "a")))->
                insert((new node(kind_pattern))->
                       insert((new node(kind_any_of))->
                              insert((new node(kind_pattern))->
                                     insert(new node(kind_text, "x")))->
                              insert((new node(kind_pattern))->
                                     insert(new node(kind_text, "y")))))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_single)))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_range, "a", "z", false)))->
                insert((new node(kind_pattern))->
                       insert(new node(kind_list, "qwe", true)))));
#undef TEST
}

WASM_EXPORT(test_glob_translate)
extern "C"
void test_glob_translate()
{
#define TEST(test_case, pattern, expected, ...) {                       \
        std::string re2;                                                \
        const char *delimiters[] = {__VA_ARGS__};                       \
        std::vector<std::string> v;                                     \
        for (int i = 0; i < sizeof(delimiters)/sizeof(const char*); i++) { \
            v.push_back(delimiters[i]);                                 \
        }                                                               \
        if (v.empty()) { v.push_back(std::string(".")); }               \
        glob_translate(pattern, strlen(pattern), v, &re2);              \
        test_str_eq(test_case, expected, re2.c_str());                  \
        re2::RE2::Options options;                                      \
        options.set_log_errors(false);                                  \
        re2::RE2 compiled(std::string(re2.c_str(),strlen(re2.c_str())), options); \
        test(test_case, compiled.ok());                                 \
    }

    TEST("glob/translate", "[a-z][!a-x]*cat*[h][!b]*eyes*", "^[a-z][^a-x][^\\.]*cat[^\\.]*[h][^b][^\\.]*eyes[^\\.]*$");
    TEST("glob/translate", "https://*.google.*", "^https\\:\\/\\/[^\\.]*\\.google\\.[^\\.]*$");
    TEST("glob/translate", "https://*.google.*", "^https\\:\\/\\/[^\\.]*\\.google\\.[^\\.]*$", "."); // "." is the default
    TEST("glob/translate", "{https://*.google.*,*yandex.*,*yahoo.*,*mail.ru}", "^(https\\:\\/\\/[^\\.]*\\.google\\.[^\\.]*|[^\\.]*yandex\\.[^\\.]*|[^\\.]*yahoo\\.[^\\.]*|[^\\.]*mail\\.ru)$");
    TEST("glob/translate", "{https://*gobwas.com,http://exclude.gobwas.com}", "^(https\\:\\/\\/[^\\.]*gobwas\\.com|http\\:\\/\\/exclude\\.gobwas\\.com)$");
    TEST("glob/translate", "abc*", "^abc[^\\.]*$");
    TEST("glob/translate", "*def", "^[^\\.]*def$");
    TEST("glob/translate", "*def", "^[^\\.]*def$", "."); // "." is the default
    TEST("glob/translate", "ab*ef", "^ab[^\\.]*ef$");
    TEST("glob/translate", "api.*.com", "^api\\.[^\\.\\,]*\\.com$", ".", ",");
    TEST("glob/translate", "api.**.com", "^api\\..*\\.com$");
    TEST("glob/translate", "api.**.com", "^api\\..*\\.com$", "."); // "." is the default...
    TEST("glob/translate", "api.**.com", "^api\\..*\\.com$", ".", ","); // and "," does not matter here
#undef TEST
}
