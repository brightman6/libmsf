
#include "connection.h"
#include "sock_utils.h"

#include <butil/logging.h>

namespace MSF {

Connection::Connection(bool thread_safe) {
  mutex_ = thread_safe ? new std::mutex : nullptr;
}

Connection::~Connection() { CloseConn(); }

// https://www.icode9.com/content-4-484721.html
void Connection::SubmitWriteIovecSafe(BufferIovec &queue) noexcept {
  if (queue.size() > 0) {
    LOG(WARNING) << "no write iovec available";
    return;
  }
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    SubmitWriteIovec(queue);
  } else {
    SubmitWriteIovec(queue);
  }
}

void Connection::SubmitWriteIovec(BufferIovec &queue) noexcept {
  if (write_pending_queue_.empty()) {
    write_pending_queue_.swap(queue);
  } else {
    write_pending_queue_.insert(write_pending_queue_.end(),
                                std::make_move_iterator(queue.begin()),
                                std::make_move_iterator(queue.end()));
  }
}

void Connection::UpdateWriteBusyIovec() noexcept {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    if (write_pending_queue_.size() > 0) {
      return;
    }
    if (write_busy_queue_.empty()) {
      write_busy_queue_.swap(write_busy_queue_);
    } else {
      write_busy_queue_.insert(
          write_busy_queue_.end(),
          std::make_move_iterator(write_pending_queue_.begin()),
          std::make_move_iterator(write_pending_queue_.end()));
    }
  }
}

void Connection::UpdateWriteBusyOffset() noexcept {}

bool Connection::HandleReadEvent() {
#if 0
  int bytes = ::recv(fd_, recvBuf_.WriteAddr(), recvBuf_.WritableSize(), 0);
  if (bytes == kError) {
      if (EAGAIN == errno || EWOULDBLOCK == errno)
          return true;

      if (EINTR == errno)
          continue; // restart ::recv

      LOG(WARNING) << fd_ << " HandleReadEvent Error " << errno;
      Shutdown(ShutdownMode::kShutdownBoth);
      state_ = State::kStateError;
      return false;
  }

  if (bytes == 0) {
      LOG(WARNING) << fd_ << " HandleReadEvent EOF ";
      if (sendBuf_.Empty()) {
          Shutdown(ShutdownMode::SHUTDOWN_Both);
          state_ = State::kStatePassiveClose;
      } else {
          state_ = State::kStateCloseWaitWrite;
          Shutdown(ShutdownMode::kShutdownRead);
          // loop_->Modify(eET_Write, shared_from_this()); // disable read
      }

      return false;
  }
#endif
  return true;
}

bool Connection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(ERROR) << fd_ << " HandleWriteEvent wrong state " << state_;
    return false;
  }

  return true;
}

void Connection::HandleErrorEvent() { return; }

void Connection::Shutdown(ShutdownMode mode) {
#if 0
  switch (mode) {
  case ShutdownMode::kShutdownRead:
      Shutdown(fd_, SHUT_RD);
      break;

  case ShutdownMode::kShutdownWrite:
      if (!send_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown write, but still has data to send";
          send_buf_.Clear();
      }

      Shutdown(fd_, SHUT_WR);
      break;

  case ShutdownMode::kShutdownBoth:
      if (!send_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown both, but still has data to send";
          send_buf_.Clear();
      }
      Shutdown(fd_, SHUT_RDWR);
      break;
  }
#endif
}

void Connection::ActiveClose() {
#if 0
   if (fd_ == kInvalidSocket)
      return;

    if (send_buf_.Empty()) {
        Shutdown(ShutdownMode::kShutdownBoth);
        state_ = State::kStateActiveClose;
    } else {
        state_ = State::kStateCloseWaitWrite;
        Shutdown(ShutdownMode::kShutdownRead); // disable read
    }

    // loop_->Modify(eET_Write, shared_from_this());
#endif
}

void Connection::CloseConn() {
  if (fd_ > 0) {
    LOG(INFO) << "Close conn for fd: " << fd_;
    // event_.remove();
    CloseSocket(fd_);
    fd_ = -1;
  }
}

}  // namespace MSF