#ifndef __ANS_ENCODE_H__
#define __ANS_ENCODE_H__

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>
#include "bits.h"

namespace ans {

  // rANS encode.
  template<uint32_t b, uint32_t k>
  class Encoder {
  public:
    // The constructor initializes M to be the sum of all Fs. It uses k
    // to determine the proper normalization interval for encoding. It uses
    // b to know how many bytes/bits etc to emit at a time.
    Encoder(const std::vector<uint32_t> &Fs)
      : _F(Fs)
      , _B([&Fs] {
          std::vector<uint32_t> t(Fs.size(), 0);
          std::partial_sum(Fs.begin(), Fs.end() - 1, t.begin() + 1);
          return t;
        }())
      , _M(_B.back() + _F.back())
      , _L(k * _M)
      , _log_b([] {
          uint32_t bb = b;
          if (bb == 0) {
            return 0;
          }

          int l = 0;
          while(bb >>= 1) {
            l++;
          }
          return l;
        }())
      , _state(_L)
    {
      static_assert((b & (b - 1)) == 0, "rANS encoder may only emit powers-of-two for renormalization!");
      static_assert((k & (k - 1)) == 0, "rANS encoder must have power-of-two multiple of precision!");
      assert(_L < (1ULL << 32));
      assert((b * _L) < (1ULL << 32));
    }

    void Encode(size_t symbol, BitWriter &w) {
      assert(_L <= _state && _state < (b * _L));
      assert(symbol < _F.size());

      // Renormalize
      uint32_t upper_bound = b * k * _F[symbol];
      while (_state >= upper_bound) {
        w.WriteBits(_state & (b - 1), _log_b);
        _state /= b;
      }

      // Encode
      _state = ((_state / _F[symbol]) * _M) + _B[symbol] + (_state % _F[symbol]);
    }

    uint32_t GetState() const { return _state; }

  private:
    const std::vector<uint32_t> _F;
    const std::vector<uint32_t> _B;

    const uint32_t _M;
    const uint32_t _L;
    const int _log_b;

    uint32_t _state;
  };

}  // namespace ans

#endif  // __ANS_ENCODE_H__