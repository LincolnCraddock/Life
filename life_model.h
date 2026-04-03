#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

struct settings_t
{
  size_t width = 25;
  size_t height = 25;
  unsigned numFlowSrcs = 1;
  unsigned numFlowSinks = 1;
  unsigned numStartCells = 100;
  long maxStartSediment = 1'000'000'000;
  float freqStartSediment = 0.003;
  unsigned seed = 0;
};

struct point_t
{
  size_t x = 0;
  size_t y = 0;

  bool
  operator== (const point_t& other) const
  {
    return x == other.x && y == other.y;
  }

  bool
  operator< (const point_t& other) const
  {
    if (x < other.x)
      return true;
    else if (x == other.x)
      return y < other.y;
    else
      return false;
  }

  bool
  operator> (const point_t& other) const
  {
    if (x > other.x)
      return true;
    else if (x == other.x)
      return y > other.y;
    else
      return false;
  }
};

struct rect_t
{
  size_t x1 = 0;
  size_t y1 = 0;
  size_t x2 = 0;
  size_t y2 = 0;

  bool
  operator== (const rect_t& other) const
  {
    return x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2;
  }

  bool
  isWellFormed () const
  {
    return (x1 < x2) == (y1 < y2) || x1 == x2 || y1 == y2;
  }
};

struct motion_t
{
  static constexpr float MAX_MAGNITUDE = std::numeric_limits<float>::max ();
  static constexpr float MAX_FLOW = 2;

  float dx = 0;
  float dy = 0;

  motion_t
  operator+ (const motion_t& other) const
  {
    return { dx + other.dx, dy + other.dy };
  }

  motion_t
  operator- (const motion_t& other) const
  {
    return { dx - other.dx, dy - other.dy };
  }

  motion_t&
  operator+= (const motion_t& other)
  {
    dx += other.dx;
    dy += other.dy;
    return *this;
  }

  motion_t&
  operator-= (const motion_t& other)
  {
    dx -= other.dx;
    dy -= other.dy;
    return *this;
  }

  float
  magnitude ()
  {
    return sqrt (dx * dx + dy * dy);
  }
};

struct direction_t
{
  unsigned char val = 0;

  direction_t
  operator+ (const direction_t& other) const
  {
    return { (unsigned char) (val + other.val) };
  }

  direction_t
  operator- (const direction_t& other) const
  {
    return { (unsigned char) (val - other.val) };
  }

  direction_t&
  operator+= (const direction_t& other)
  {
    val += other.val;
    return *this;
  }

  direction_t&
  operator-= (const direction_t& other)
  {
    val -= other.val;
    return *this;
  }

  motion_t
  getMotion ()
  {
    float angle = (float) val / 128.0 * M_PI;
    return { std::cos (angle), std::sin (angle) };
  }

  motion_t
  getMotion (float speed)
  {
    float angle = (float) val / 128.0 * M_PI;
    return { speed * std::cos (angle), speed * std::sin (angle) };
  }
};

struct polymer_shape_t
{
  uint16_t data = 0;
};

struct polymer_t
{
  uint32_t data = 0;

  size_t
  a ()
  {
    return (size_t) (data >> 8);
  }

  size_t
  b ()
  {
    return (size_t) ((data >> 4) & 0x03);
  }

  size_t
  c ()
  {
    return (size_t) (data & 0x03);
  }

  polymer_shape_t
  toShape ()
  {
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
    uint32_t p = data;
    for (uint8_t next = p & 0x03; next != 0x03; p = p >> 2, next = p & 0x03)
    {
      switch (p)
      {
        case 0x00:
          ++a;
          break;
        case 0x01:
          ++b;
          break;
        case 0x02:
          ++c;
          break;
      }
    }
    return { (uint16_t) ((a << 8) | (b << 4) | c) };
  }

  bool
  operator== (const polymer_t& other) const
  {
    return data == other.data;
  }
};

// Specialize std::hash for the Person struct within the std namespace
namespace std
{
template<>
struct hash<polymer_t>
{
  std::size_t
  operator() (const polymer_t& p) const noexcept
  {
    return p.data;
  }
};
}

struct substance_t
{
  static constexpr long MAX_VAL = std::numeric_limits<long>::max ();

  long a = 0;
  long b = 0;
  long c = 0;
  std::unordered_map<polymer_t, long> data;

  // TODO
  substance_t
  operator+ (const substance_t& other) const
  {
    return { a + other.a, b + other.b, c + other.c };
  }

  substance_t
  operator- (const substance_t& other) const
  {
    return { a - other.a, b - other.b, c - other.c };
  }

  substance_t&
  operator+= (const substance_t& other)
  {
    a += other.a;
    b += other.b;
    c += other.c;
    return *this;
  }

  substance_t&
  operator-= (const substance_t& other)
  {
    a -= other.a;
    b -= other.b;
    c -= other.c;
    return *this;
  }

  substance_t
  operator* (long mult) const
  {
    return { a * mult, b * mult, c * mult };
  }

  substance_t
  operator/ (long div) const
  {
    return { a / div, b / div, c / div };
  }

  substance_t&
  operator*= (long mult)
  {
    a *= mult;
    b *= mult;
    c *= mult;
    return *this;
  }

  substance_t&
  operator/= (long div)
  {
    a /= div;
    b /= div;
    c /= div;
    return *this;
  }

  substance_t
  operator* (substance_t mult) const
  {
    return { a * mult.a, b * mult.b, c * mult.c };
  }

  substance_t
  operator/ (substance_t div) const
  {
    return { a / div.a, b / div.b, c / div.c };
  }

  substance_t&
  operator*= (substance_t mult)
  {
    a *= mult.a;
    b *= mult.b;
    c *= mult.c;
    return *this;
  }

  substance_t&
  operator/= (substance_t div)
  {
    a /= div.a;
    b /= div.b;
    c /= div.c;
    return *this;
  }

  substance_t
  operator* (float mult) const
  {
    mult = fabsf (mult);
    return { (long) (a * mult), (long) (b * mult), (long) (c * mult) };
  }

  substance_t
  operator/ (float div) const
  {
    div = abs (div);
    return { (long) (a / div), (long) (b / div), (long) (c / div) };
  }

  substance_t&
  operator*= (float mult)
  {
    mult = abs (mult);
    a *= mult;
    b *= mult;
    c *= mult;
    return *this;
  }

  substance_t&
  operator/= (float div)
  {
    div = abs (div);
    a /= div;
    b /= div;
    c /= div;
    return *this;
  }

  long
  operator[] (polymer_t p)
  {
    auto iter = data.find (p);
    if (iter != data.end ())
      return iter->second;
    else
      return 0;
  }

  float
  concentrationOf (polymer_t p)
  {
    // TODO
    return (float) (*this)[p] / 10.0;
  }
};

template<typename T>
struct node_t
{
  std::vector<node_t<T>*> inputs {};
  std::vector<float> weights {};
  std::optional<float> signal;
  float bias = 0.0f;
  T val {};

  float
  pull ()
  {
    if (signal)
      return *signal;
    else
    {
      float sum = bias;
      for (size_t i = 0; i < inputs.size (); ++i)
        sum += inputs[i]->pull () * weights[i];
      return sum;
    }
  }
};

struct epigenome_t
{
  std::vector<node_t<polymer_t>> inputLayer;
  std::vector<node_t<polymer_t>> middleLayer;
  std::vector<node_t<polymer_t>> outputLayer;

  std::vector<std::pair<polymer_t, float>>
  evaluate (substance_t& substance)
  {
    for (node_t<polymer_t>& inputNode : inputLayer)
      inputNode.signal = substance.concentrationOf (inputNode.val);

    for (node_t<polymer_t>& middleNode : middleLayer)
      middleNode.signal = middleNode.pull ();

    std::vector<std::pair<polymer_t, float>> rates;
    rates.reserve (outputLayer.size ());
    for (node_t<polymer_t>& outputNode : outputLayer)
    {
      float sum = outputNode.bias;
      for (size_t i = 0; i < outputNode.inputs.size (); ++i)
        sum += outputNode.inputs[i]->pull () * outputNode.weights[i];
      rates.emplace_back (outputNode.val, sum);
    }

    // clear both layers' signals
    for (node_t<polymer_t>& inputNode : inputLayer)
      inputNode.signal = std::nullopt;
    for (node_t<polymer_t>& middleNode : middleLayer)
      middleNode.signal = std::nullopt;

    return rates;
  }
};

struct cell_t
{
  float speed = 0;
  direction_t dir = { 0 };
  long energy = 10;

  substance_t substance {};
  unsigned long timeOfLastXMove = 0;
  unsigned long timeOfLastYMove = 0;

  epigenome_t epigenome { {}, {}, {} };

  motion_t
  getMotion ()
  {
    return dir.getMotion (speed);
  }

  void
  update ()
  {
    auto rates = epigenome.evaluate (substance);

    for (auto& [polymer, rate] : rates)
    {
      long delta = static_cast<long> (rate);
      long current = substance[polymer];
      substance.data[polymer] =
        std::clamp (current + delta, 0L, substance_t::MAX_VAL);
    }
    --energy;
  }
};

struct square_t
{
  motion_t flow {};
  bool isFlowSrc = false;
  substance_t sediment {};
  cell_t* cell = nullptr;
};

inline std::ostream&
operator<< (std::ostream& os, point_t const& arg)
{
  os << "(" << arg.x << ", " << arg.y << ")";
  return os;
};

inline std::ostream&
operator<< (std::ostream& os, rect_t const& arg)
{
  os << "(" << arg.x1 << ", " << arg.y1 << ", " << arg.x2 << ", " << arg.y2
     << ")";
  return os;
};

inline std::ostream&
operator<< (std::ostream& os, motion_t const& arg)
{
  os << "(" << arg.dx << ", " << arg.dy << ")";
  return os;
};

inline std::ostream&
operator<< (std::ostream& os, substance_t const& arg)
{
  os << "( A = " << arg.a << ", B = " << arg.b << ", C = " << arg.c << ")";
  return os;
};

inline size_t
addMod (size_t a, int b, size_t mod)
{
  return (((int) a + b) % (int) mod + mod) % mod;
};

inline substance_t
distributeSediment (substance_t s)
{
  return { s.a / 9 + std::min (s.a, s.b) / 72,
           s.c > s.a ? (s.b > (s.c - s.a) ? (s.b - (s.c - s.a)) / 9 : 0)
                     : s.b / 9 + std::min (s.b, s.a - s.c) / 72,
           s.c > s.b ? (s.c - s.b) / 9 : 0 };
}

class Model
{
public:
  Model (settings_t settings);
  void
  step ();
  const std::vector<std::vector<square_t>>&
  world () const;
  const settings_t
  settings () const;
  bool
  isInsideWorld (point_t p) const;
  bool
  isInsideWorld (rect_t r) const;

private:
  settings_t m_settings;
  std::vector<std::vector<square_t>> m_world;
  unsigned long m_time;
};