/*
 * ngtcp2
 *
 * Copyright (c) 2020 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // HAVE_CONFIG_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <string_view>
#include <functional>

#include <ngtcp2/ngtcp2.h>

#include "tls_server_session.h"
#include "network.h"
#include "shared.h"

using namespace ngtcp2;

struct Config {
  Address preferred_ipv4_addr;
  Address preferred_ipv6_addr;
  // tx_loss_prob is probability of losing outgoing packet.
  double tx_loss_prob;
  // rx_loss_prob is probability of losing incoming packet.
  double rx_loss_prob;
  // ciphers is the list of enabled ciphers.
  const char *ciphers;
  // groups is the list of supported groups.
  const char *groups;
  // htdocs is a root directory to serve documents.
  std::string htdocs;
  // mime_types_file is a path to "MIME media types and the
  // extensions" file.  Ubuntu mime-support package includes it in
  // /etc/mime/types.
  std::string_view mime_types_file;
  // mime_types maps file extension to MIME media type.
  std::unordered_map<std::string, std::string> mime_types;
  // port is the port number which server listens on for incoming
  // connections.
  uint16_t port;
  // quiet suppresses the output normally shown except for the error
  // messages.
  bool quiet;
  // timeout is an idle timeout for QUIC connection.
  ngtcp2_duration timeout;
  // show_secret is true if transport secrets should be printed out.
  bool show_secret;
  // validate_addr is true if server requires address validation.
  bool validate_addr;
  // early_response is true if server starts sending response when it
  // receives HTTP header fields without waiting for request body.  If
  // HTTP response data is written before receiving request body,
  // STOP_SENDING is sent.
  bool early_response;
  // verify_client is true if server verifies client with X.509
  // certificate based authentication.
  bool verify_client;
  // qlog_dir is the path to directory where qlog is stored.
  std::string_view qlog_dir;
  // no_quic_dump is true if hexdump of QUIC STREAM and CRYPTO data
  // should be disabled.
  bool no_quic_dump;
  // no_http_dump is true if hexdump of HTTP response body should be
  // disabled.
  bool no_http_dump;
  // max_data is the initial connection-level flow control window.
  uint64_t max_data;
  // max_stream_data_bidi_local is the initial stream-level flow
  // control window for a bidirectional stream that the local endpoint
  // initiates.
  uint64_t max_stream_data_bidi_local;
  // max_stream_data_bidi_remote is the initial stream-level flow
  // control window for a bidirectional stream that the remote
  // endpoint initiates.
  uint64_t max_stream_data_bidi_remote;
  // max_stream_data_uni is the initial stream-level flow control
  // window for a unidirectional stream.
  uint64_t max_stream_data_uni;
  // max_streams_bidi is the number of the concurrent bidirectional
  // streams.
  uint64_t max_streams_bidi;
  // max_streams_uni is the number of the concurrent unidirectional
  // streams.
  uint64_t max_streams_uni;
  // max_window is the maximum connection-level flow control window
  // size if auto-tuning is enabled.
  uint64_t max_window;
  // max_stream_window is the maximum stream-level flow control window
  // size if auto-tuning is enabled.
  uint64_t max_stream_window;
  // max_dyn_length is the maximum length of dynamically generated
  // response.
  uint64_t max_dyn_length;
  // static_secret is used to derive keying materials for Retry and
  // Stateless Retry token.
  std::array<uint8_t, 32> static_secret;
  // cc_algo is the congestion controller algorithm.
  ngtcp2_cc_algo cc_algo;
  // initial_rtt is an initial RTT.
  ngtcp2_duration initial_rtt;
  // max_udp_payload_size is the maximum UDP payload size that server
  // transmits.
  size_t max_udp_payload_size;
  // send_trailers controls whether server sends trailer fields or
  // not.
  bool send_trailers;
  // max_gso_dgrams is the maximum number of UDP datagrams in one GSO
  // sendmsg call.
  size_t max_gso_dgrams;
};

struct Buffer {
  Buffer(const uint8_t *data, size_t datalen);
  explicit Buffer(size_t datalen);

  size_t size() const { return tail - begin; }
  size_t left() const { return buf.data() + buf.size() - tail; }
  uint8_t *const wpos() { return tail; }
  const uint8_t *rpos() const { return begin; }
  void push(size_t len) { tail += len; }
  void reset() { tail = begin; }

  std::vector<uint8_t> buf;
  // begin points to the beginning of the buffer.  This might point to
  // buf.data() if a buffer space is allocated by this object.  It is
  // also allowed to point to the external shared buffer.
  uint8_t *begin;
  // tail points to the position of the buffer where write should
  // occur.
  uint8_t *tail;
};

class HandlerBase {
public:
  HandlerBase();
  ~HandlerBase();

  ngtcp2_conn *conn() const;

  int on_rx_key(ngtcp2_crypto_level level, const uint8_t *secret,
                size_t secretlen);
  int on_tx_key(ngtcp2_crypto_level level, const uint8_t *secret,
                size_t secretlen);

  void write_server_handshake(ngtcp2_crypto_level crypto_level,
                              const uint8_t *data, size_t datalen);

  void set_tls_alert(uint8_t alert);

  int call_application_tx_key_cb() const;

protected:
  TLSServerSession tls_session_;
  ngtcp2_conn *conn_;
  QUICError last_error_;
  std::function<int()> application_tx_key_cb_;
};

#endif // SERVER_BASE_H
