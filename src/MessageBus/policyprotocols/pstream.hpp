#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// ============================================================================
// 1. THE STREAM BUFFER (Manages FIFO File Descriptors & Buffering)
// ============================================================================
template <typename CharT, typename Traits = std::char_traits<CharT>>
class BasicPipeBuf : public std::basic_streambuf<CharT, Traits> {
public:
    using Base = std::basic_streambuf<CharT, Traits>;
    using int_type = typename Traits::int_type;

    BasicPipeBuf() : fd_(-1) {
        // Allocate internal buffers for reading and writing
        buffer_in_.resize(buffer_size_);
        buffer_out_.resize(buffer_size_);
        
        // Initialize the write buffer pointers (pbase, pptr, epptr)
        this->setp(buffer_out_.data(), buffer_out_.data() + buffer_out_.size());
    }

    ~BasicPipeBuf() override {
        close_pipe();
    }

    bool open_pipe(const std::string& path, std::ios_base::openmode mode) {
        close_pipe();

        // 1. Create the FIFO if it doesn't already exist
        mkfifo(path.c_str(), 0666);

        // 2. Map standard stream flags to POSIX file flags
        int flags = 0;
        if ((mode & std::ios_base::in) && (mode & std::ios_base::out)) {
            flags = O_RDWR;
        } else if (mode & std::ios_base::in) {
            flags = O_RDONLY;
        } else if (mode & std::ios_base::out) {
            flags = O_WRONLY;
        }

        // 3. Open the file descriptor
        fd_ = open(path.c_str(), flags);
        return fd_ != -1;
    }

    void close_pipe() {
        if (fd_ != -1) {
            sync(); // Flush remaining write buffer data
            close(fd_);
            fd_ = -1;
        }
    }

    bool is_open() const { return fd_ != -1; }

protected:
    // Triggered when the read buffer is empty (underflow)
    int_type underflow() override {
        if (fd_ == -1) return Traits::eof();

        // If the read pointers are still valid, return current character
        if (this->gptr() < this->egptr()) {
            return Traits::to_int_type(*this->gptr());
        }

        // Read fresh bytes from the POSIX named pipe
        ssize_t bytes_read = read(fd_, buffer_in_.data(), buffer_in_.size() * sizeof(CharT));
        if (bytes_read <= 0) {
            return Traits::eof(); // End of file or pipe closed
        }

        // Update read pointers (eback, gptr, egptr)
        size_t elements_read = bytes_read / sizeof(CharT);
        this->setg(buffer_in_.data(), buffer_in_.data(), buffer_in_.data() + elements_read);

        return Traits::to_int_type(*this->gptr());
    }

    // Triggered when the write buffer is full, or when flushed (overflow)
    int_type overflow(int_type ch) override {
        if (fd_ == -1) return Traits::eof();

        if (sync() == -1) {
            return Traits::eof();
        }

        if (!Traits::eq_int_type(ch, Traits::eof())) {
            *this->pptr() = Traits::to_char_type(ch);
            this->pbump(1);
        }
        return Traits::not_eof(ch);
    }

    // Flushes the write buffer out to the operating system
    int sync() override {
        if (fd_ == -1) return -1;

        ptrdiff_t count = this->pptr() - this->pbase();
        if (count > 0) {
            ssize_t bytes_written = write(fd_, this->pbase(), count * sizeof(CharT));
            if (bytes_written <= 0) {
                return -1; // Write failed
            }
            // Reset write pointers to the beginning of the buffer
            this->setp(buffer_out_.data(), buffer_out_.data() + buffer_out_.size());
        }
        return 0;
    }

private:
    int fd_;
    static constexpr size_t buffer_size_ = 1024;
    std::vector<CharT> buffer_in_;
    std::vector<CharT> buffer_out_;
};

// ============================================================================
// 2. THE PIPE STREAM (Inherits std::basic_iostream)
// ============================================================================
template <typename CharT, typename Traits = std::char_traits<CharT>>
class BasicPipeStream : public std::basic_iostream<CharT, Traits> {
public:
    // Initialize stream with our custom pipe buffer
    BasicPipeStream() : std::basic_iostream<CharT, Traits>(&buf_), buf_() {}

    BasicPipeStream(const std::string& path, std::ios_base::openmode mode) 
        : std::basic_iostream<CharT, Traits>(&buf_), buf_() {
        open(path, mode);
    }

    void open(const std::string& path, std::ios_base::openmode mode) {
        if (!buf_.open_pipe(path, mode)) {
            // Set the stream error state if the file descriptor fails to open
            this->setstate(std::ios_base::failbit);
        } else {
            this->clear();
        }
    }

    void close() {
        buf_.close_pipe();
    }

    bool is_open() const {
        return buf_.is_open();
    }

private:
    BasicPipeBuf<CharT, Traits> buf_; // Must be declared below stream initialization
};

// Convenience alias for standard char streams
using PipeStream = BasicPipeStream<char>;
