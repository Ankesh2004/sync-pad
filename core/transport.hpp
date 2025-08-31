#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <optional>
#include <cstdint>

struct Frame {
    uint8_t type;        // 1=HELLO,2=ACK,3=PING,4=PONG
    std::string payload; // raw payload (UTF-8)
};

class Transport {
public:
    // listen_port: if >0 the transport will listen and accept incoming connections (server mode)
    // peer_host/peer_port: if non-empty, transport will attempt to connect (client mode)
    Transport(int listen_port, const std::string& peer_host, int peer_port);
    ~Transport();

    // start background threads (accept/connect/io)
    void start();

    // stop threads and close sockets
    void stop();

    // send a frame if connected (thread-safe). returns true on success
    bool send_frame(const Frame& f);

    // pop a received frame (thread-safe). returns true if frame was popped into out.
    bool pop_frame(Frame &out);

    // query connected state
    bool is_connected() const;

private:
    // internal helpers
    void accept_thread_fn();
    void connect_thread_fn();
    void io_thread_fn(int fd);

    // low-level read/write
    bool write_all(int fd, const void* buf, size_t len);
    bool read_all(int fd, void* buf, size_t len);

    // create/close connection
    void set_connected_fd(int fd);
    void clear_connected_fd();

private:
    int listen_port_;
    std::string peer_host_;
    int peer_port_;

    std::thread accept_thread_;
    std::thread connect_thread_;
    std::thread io_thread_;

    int listen_fd_ = -1;
    mutable std::mutex conn_mutex_;
    int conn_fd_ = -1;

    // incoming queue
    mutable std::mutex q_mutex_;
    std::condition_variable q_cv_;
    std::queue<Frame> q_;

    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};

    // write lock to serialize writes on socket
    std::mutex write_mutex_;
};
