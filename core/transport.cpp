#include "transport.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

// ---------- utility helpers ----------
static void set_reuseaddr(int fd) {
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}

// ---------- constructor / destructor ----------
Transport::Transport(int listen_port, const std::string& peer_host, int peer_port)
: listen_port_(listen_port), peer_host_(peer_host), peer_port_(peer_port) {}

Transport::~Transport() { stop(); }

// ---------- start / stop ----------
void Transport::start() {
    running_ = true;

    // start accept thread if needed
    if (listen_port_ > 0) {
        accept_thread_ = std::thread(&Transport::accept_thread_fn, this);
    }

    // start connect thread if peer_host is set
    if (!peer_host_.empty() && peer_port_ > 0) {
        connect_thread_ = std::thread(&Transport::connect_thread_fn, this);
    }
}

void Transport::stop() {
    running_ = false;

    // close listening socket to unblock accept
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

    // close connection socket
    {
        std::lock_guard<std::mutex> lk(conn_mutex_);
        if (conn_fd_ >= 0) {
            close(conn_fd_);
            conn_fd_ = -1;
        }
    }

    // join threads
    if (accept_thread_.joinable()) accept_thread_.join();
    if (connect_thread_.joinable()) connect_thread_.join();
    if (io_thread_.joinable()) io_thread_.join();

    // notify any waiting pop
    q_cv_.notify_all();
}

// ---------- accept thread (server) ----------
void Transport::accept_thread_fn() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        perror("socket");
        return;
    }
    set_reuseaddr(listen_fd_);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }
    if (listen(listen_fd_, 1) < 0) {
        perror("listen");
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }

    while (running_) {
        sockaddr_in peer{};
        socklen_t plen = sizeof(peer);
        int fd = accept(listen_fd_, (struct sockaddr*)&peer, &plen);
        if (fd < 0) {
            if (!running_) break;
            perror("accept");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        char peerhost[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, peerhost, sizeof(peerhost));
        std::cout << "[transport] accepted connection from " << peerhost << ":" << ntohs(peer.sin_port) << "\n";

        // set non-blocking? we keep blocking for simplicity in io thread
        set_connected_fd(fd);
    }
}

// ---------- connect thread (client, with exponential backoff capped ~3s) ----------
void Transport::connect_thread_fn() {
    double delay = 0.1; // start 100ms
    const double max_delay = 3.0;

    while (running_) {
        // if already connected, sleep and continue
        if (connected_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        // resolve host
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int rc = getaddrinfo(peer_host_.c_str(), nullptr, &hints, &res);
        if (rc != 0 || !res) {
            std::cerr << "[connect] getaddrinfo failed: " << gai_strerror(rc) << "\n";
            std::this_thread::sleep_for(std::chrono::duration<double>(delay));
            delay = std::min(delay * 2, max_delay);
            continue;
        }

        sockaddr_in serv{}; serv.sin_family = AF_INET;
        serv.sin_port = htons(peer_port_);
        serv.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;
        freeaddrinfo(res);

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("socket");
            std::this_thread::sleep_for(std::chrono::duration<double>(delay));
            delay = std::min(delay * 2, max_delay);
            continue;
        }

        std::cout << "[connect] trying " << peer_host_ << ":" << peer_port_ << "\n";
        if (connect(fd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
            close(fd);
            // failed
            std::cout << "[connect] failed, backing off " << delay << "s\n";
            std::this_thread::sleep_for(std::chrono::duration<double>(delay));
            delay = std::min(delay * 2, max_delay);
            continue;
        }

        // success
        std::cout << "[connect] connected to " << peer_host_ << ":" << peer_port_ << "\n";
        delay = 0.1;
        set_connected_fd(fd);
    }
}

// ---------- set/clear connection ----------
void Transport::set_connected_fd(int fd) {
    {
        std::lock_guard<std::mutex> lk(conn_mutex_);
        if (conn_fd_ >= 0) {
            close(conn_fd_);
            conn_fd_ = -1;
        }
        conn_fd_ = fd;
        connected_ = true;
    }

    // start io thread to read frames from this fd
    if (io_thread_.joinable()) {
        // if previous io_thread exists, join it (should usually be joined when cleared)
        io_thread_.join();
    }
    io_thread_ = std::thread(&Transport::io_thread_fn, this, fd);
}

void Transport::clear_connected_fd() {
    std::lock_guard<std::mutex> lk(conn_mutex_);
    if (conn_fd_ >= 0) {
        close(conn_fd_);
        conn_fd_ = -1;
    }
    connected_ = false;
}

// ---------- low-level io helpers ----------
bool Transport::write_all(int fd, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t w = ::send(fd, p, remaining, 0);
        if (w <= 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += w;
        remaining -= (size_t)w;
    }
    return true;
}

bool Transport::read_all(int fd, void* buf, size_t len) {
    char* p = (char*)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t r = ::recv(fd, p, remaining, 0);
        if (r == 0) return false; // closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += r;
        remaining -= (size_t)r;
    }
    return true;
}

// ---------- IO thread: read frames, push to queue ----------
void Transport::io_thread_fn(int fd) {
    while (running_) {
        // read 4-byte length (big-endian)
        uint32_t len_be = 0;
        if (!read_all(fd, &len_be, sizeof(len_be))) break;
        uint32_t len = ntohl(len_be);
        if (len == 0 || len > 10*1024*1024) { // sanity limit 10MB
            std::cerr << "[io] invalid frame length: " << len << "\n";
            break;
        }

        // read 1-byte type
        uint8_t type = 0;
        if (!read_all(fd, &type, 1)) break;
        size_t payload_len = len - 1;
        std::string payload;
        if (payload_len > 0) {
            payload.resize(payload_len);
            if (!read_all(fd, payload.data(), payload_len)) break;
        }

        // push frame to queue
        {
            std::lock_guard<std::mutex> lk(q_mutex_);
            q_.push(Frame{type, payload});
        }
        q_cv_.notify_one();
    }

    // connection closed or error: clear connection
    std::cout << "[io] connection closed or error\n";
    clear_connected_fd();
}

// ---------- public send_frame (thread-safe) ----------
bool Transport::send_frame(const Frame& f) {
    std::lock_guard<std::mutex> lk(conn_mutex_);
    if (conn_fd_ < 0) return false;
    int fd = conn_fd_;

    std::lock_guard<std::mutex> wlk(write_mutex_);
    uint32_t len = 1 + (uint32_t)f.payload.size();
    uint32_t len_be = htonl(len);
    if (!write_all(fd, &len_be, sizeof(len_be))) return false;
    if (!write_all(fd, &f.type, 1)) return false;
    if (len > 1) {
        if (!write_all(fd, f.payload.data(), f.payload.size())) return false;
    }
    return true;
}

bool Transport::pop_frame(Frame &out) {
    std::unique_lock<std::mutex> lk(q_mutex_);
    if (q_.empty()) return false;
    out = q_.front();
    q_.pop();
    return true;
}

bool Transport::is_connected() const {
    return connected_.load();
}
