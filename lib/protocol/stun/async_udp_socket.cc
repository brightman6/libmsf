#include "async_udp_socket.h"

#include <stdint.h>
#include <string>
#include <butil/logging.h>

#include "sent_packet.h"
#include "sigslot.h"
#include "time_utils.h"

namespace MSF {

static const int BUF_SIZE = 64 * 1024;

AsyncUDPSocket* AsyncUDPSocket::Create(AsyncSocket* socket,
                                       const SocketAddress& bind_address) {
  std::unique_ptr<AsyncSocket> owned_socket(socket);
  if (socket->Bind(bind_address) < 0) {
    LOG(ERROR) << "Bind() failed with error " << socket->GetError();
    return nullptr;
  }
  return new AsyncUDPSocket(owned_socket.release());
}

AsyncUDPSocket* AsyncUDPSocket::Create(SocketFactory* factory,
                                       const SocketAddress& bind_address) {
  AsyncSocket* socket =
      factory->CreateAsyncSocket(bind_address.family(), SOCK_DGRAM);
  if (!socket) return nullptr;
  return Create(socket, bind_address);
}

AsyncUDPSocket::AsyncUDPSocket(AsyncSocket* socket) : socket_(socket) {
  size_ = BUF_SIZE;
  buf_ = new char[size_];

  // The socket should start out readable but not writable.
  socket_->SignalReadEvent.connect(this, &AsyncUDPSocket::OnReadEvent);
  socket_->SignalWriteEvent.connect(this, &AsyncUDPSocket::OnWriteEvent);
}

AsyncUDPSocket::~AsyncUDPSocket() { delete[] buf_; }

SocketAddress AsyncUDPSocket::GetLocalAddress() const {
  return socket_->GetLocalAddress();
}

SocketAddress AsyncUDPSocket::GetRemoteAddress() const {
  return socket_->GetRemoteAddress();
}

int AsyncUDPSocket::Send(const void* pv, size_t cb,
                         const PacketOptions& options) {
  SentPacket sent_packet(options.packet_id, TimeMillis(),
                         options.info_signaled_after_sent);
  CopySocketInformationToPacketInfo(cb, *this, false, &sent_packet.info);
  int ret = socket_->Send(pv, cb);
  SignalSentPacket(this, sent_packet);
  return ret;
}

int AsyncUDPSocket::SendTo(const void* pv, size_t cb, const SocketAddress& addr,
                           const PacketOptions& options) {
  SentPacket sent_packet(options.packet_id, TimeMillis(),
                         options.info_signaled_after_sent);
  CopySocketInformationToPacketInfo(cb, *this, true, &sent_packet.info);
  int ret = socket_->SendTo(pv, cb, addr);
  SignalSentPacket(this, sent_packet);
  return ret;
}

int AsyncUDPSocket::Close() { return socket_->Close(); }

AsyncUDPSocket::State AsyncUDPSocket::GetState() const { return STATE_BOUND; }

int AsyncUDPSocket::GetOption(Socket::Option opt, int* value) {
  return socket_->GetOption(opt, value);
}

int AsyncUDPSocket::SetOption(Socket::Option opt, int value) {
  return socket_->SetOption(opt, value);
}

int AsyncUDPSocket::GetError() const { return socket_->GetError(); }

void AsyncUDPSocket::SetError(int error) { return socket_->SetError(error); }

void AsyncUDPSocket::OnReadEvent(AsyncSocket* socket) {
  assert(socket_.get() == socket);

  SocketAddress remote_addr;
  int64_t timestamp;
  int len = socket_->RecvFrom(buf_, size_, &remote_addr, &timestamp);
  if (len < 0) {
    // An error here typically means we got an ICMP error in response to our
    // send datagram, indicating the remote address was unreachable.
    // When doing ICE, this kind of thing will often happen.
    // TODO: Do something better like forwarding the error to the user.
    SocketAddress local_addr = socket_->GetLocalAddress();
    LOG(INFO) << "AsyncUDPSocket[" << local_addr.ToSensitiveString()
              << "] receive failed with error " << socket_->GetError();
    return;
  }

  // TODO: Make sure that we got all of the packet.
  // If we did not, then we should resize our buffer to be large enough.
  SignalReadPacket(this, buf_, static_cast<size_t>(len), remote_addr,
                   (timestamp > -1 ? timestamp : TimeMicros()));
}

void AsyncUDPSocket::OnWriteEvent(AsyncSocket* socket) {
  SignalReadyToSend(this);
}

}  // namespace MSF
