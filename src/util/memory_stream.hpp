#include <cstddef>
#include <ios>
#include <istream>
#include <streambuf>


/* std::spanstream is not supported everywhere so we have to provide ourselves. */
class MemoryStreamBuf : public std::streambuf {
public:
    MemoryStreamBuf(const char* data, std::size_t size) {
        auto* begin = const_cast<char*>(data);
        setg(begin, begin, begin + size);
    }

protected:
    pos_type seekoff(
        off_type off,
        std::ios_base::seekdir dir,
        std::ios_base::openmode which = std::ios_base::in
    ) override {
        if (!(which & std::ios_base::in)) {
            return pos_type(off_type(-1));
        }

        char* base = eback();
        char* current = gptr();
        char* end = egptr();
        char* next = nullptr;

        switch (dir) {
            case std::ios_base::beg:
                next = base + off;
                break;
            case std::ios_base::cur:
                next = current + off;
                break;
            case std::ios_base::end:
                next = end + off;
                break;
            default:
                return pos_type(off_type(-1));
        }

        if (next < base || next > end) {
            return pos_type(off_type(-1));
        }

        setg(base, next, end);
        return pos_type(next - base);
    }
};

class MemoryIStream : public std::istream {
public:
    MemoryIStream(const char* data, std::size_t size)
        : std::istream(nullptr), buf_(data, size) {
        rdbuf(&buf_);
        clear();
    }

private:
    MemoryStreamBuf buf_;
};