/*
 * PLEASE LICENSE 2022, Michael Clark <michaeljclark@mac.com>
 *
 * All rights to this work are granted for all purposes, with exception of
 * author's implied right of copyright to defend the free use of this work.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

struct zvec_logger
{
    enum L { Ltrace, Ldebug, Lnone };

    static const char* level_names[2];

    static L level;
    
    static void output(const char*prefix, const char* fmt, va_list ap);
    static void log(L level, const char* fmt, ...);
    static void set_level(L level);
};

#ifdef ZIP_VECTOR_TRACE

#undef Trace
#define Trace(fmt, ...) \
if (zvec_logger::L::Ltrace >= zvec_logger::level) \
{ zvec_logger::log(zvec_logger::L::Ltrace, fmt, ## __VA_ARGS__); }

#undef Debug
#define Debug(fmt, ...) \
if (zvec_logger::L::Ldebug >= zvec_logger::level) \
{ zvec_logger::log(zvec_logger::L::Ldebug, fmt, ## __VA_ARGS__); }

#else

#undef Trace
#define Trace(...)

#undef Debug
#define Debug(...)

#endif