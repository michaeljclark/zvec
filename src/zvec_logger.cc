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

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <cstddef>
#include <cassert>

#include <vector>

#include <zvec_logger.h>

#ifdef _WIN32
#include <Windows.h>
#endif

zvec_logger::L zvec_logger::level = zvec_logger::L::Lnone;

const char* zvec_logger::level_names[2] = { "trace", "debug" };

void zvec_logger::output(const char *prefix, const char* fmt, va_list args1)
{
    std::vector<char> buf;
    va_list args2;
    size_t plen, pout, len, ret;

    va_copy(args2, args1);

    plen = strlen(prefix);
    len = (size_t)vsnprintf(NULL, 0, fmt, args1);
    assert(len >= 0);
    buf.resize(plen + len + 4); // prefix + ": " + message + CR + zero-terminator
    pout = (size_t)snprintf(buf.data(), buf.capacity(), "%s: ", prefix);
    ret = (size_t)vsnprintf(buf.data() + pout, buf.capacity(), fmt, args2);
    assert(len == ret);
    if (buf[buf.size()-3] != '\n') buf[buf.size()-2] = '\n';

#if defined (_WIN32) && !defined (_CONSOLE)
    OutputDebugStringA(buf.data());
#else
    printf("%s", buf.data());
#endif
}

void zvec_logger::log(L level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (level >= zvec_logger::level) {
        output(zvec_logger::level_names[level], fmt, ap);
    }
    va_end(ap);
}

void zvec_logger::set_level(L level)
{
    zvec_logger::level = level;
}