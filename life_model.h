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

struct cell_t;

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

  float
  concentrationOf (char type)
  {
    if (type == 'a')
      return a / ((float) a + 1.0f + b + c);
    else if (type == 'b')
      return b / ((float) a + 1.0f + b + c);
    else if (type == 'c')
      return c / ((float) a + 1.0f + b + c);
    return 0.0f;
  }
};

// template<typename T>
// struct node_t
// {
//   std::vector<node_t<T>*> inputs {};
//   std::vector<float> weights {};
//   std::optional<float> signal;
//   float bias = 0.0f;
//   T val {};

//   float
//   pull ()
//   {
//     if (signal)
//       return *signal;
//     else
//     {
//       float sum = bias;
//       for (size_t i = 0; i < inputs.size (); ++i)
//         sum += inputs[i]->pull () * weights[i];
//       return sum;
//     }
//   }
// };

// struct epigenome_t
// {
//   std::vector<node_t<polymer_t>> inputLayer;
//   std::vector<node_t<polymer_t>> middleLayer;
//   std::vector<node_t<polymer_t>> outputLayer;

//   std::vector<std::pair<polymer_t, float>>
//   evaluate (substance_t& substance)
//   {
//     // inputLayer should have size 3
//     char type = 'a';
//     for (node_t<polymer_t>& inputNode : inputLayer)
//       inputNode.signal = substance.concentrationOf (type++);

//     for (node_t<polymer_t>& middleNode : middleLayer)
//       middleNode.signal = middleNode.pull ();

//     std::vector<std::pair<polymer_t, float>> rates;
//     rates.reserve (outputLayer.size ());
//     for (node_t<polymer_t>& outputNode : outputLayer)
//     {
//       float sum = outputNode.bias;
//       for (size_t i = 0; i < outputNode.inputs.size (); ++i)
//         sum += outputNode.inputs[i]->pull () * outputNode.weights[i];
//       rates.emplace_back (outputNode.val, sum);
//     }

//     // clear both layers' signals
//     for (node_t<polymer_t>& inputNode : inputLayer)
//       inputNode.signal = std::nullopt;
//     for (node_t<polymer_t>& middleNode : middleLayer)
//       middleNode.signal = std::nullopt;

//     return rates;
//   }
// };

// struct cell_t
// {
//   // float speed = 0;
//   // direction_t dir = { 0 };
//   motion_t velocity;
//   long energy = 10;

//   substance_t substance {};
//   unsigned long timeOfLastXMove = 0;
//   unsigned long timeOfLastYMove = 0;

//   epigenome_t epigenome { {}, {}, {} };

//   // motion_t
//   // getMotion ()
//   // {
//   //   return dir.getMotion (speed);
//   // }

//   void
//   update ()
//   {
//     auto rates = epigenome.evaluate (substance);

//     for (auto& [polymer, rate] : rates)
//     {
//       if (polymer.a () > 0)
//         velocity.dx += rate;
//       else if (polymer.b ())
//         velocity.dy += rate;
//       // TODO: like polymer rates to other actions instead
//     }
//     --energy;
//   }

//   void
//   randomizeEpigenome(epigenome_t& parentEpigenome)
//   {
//     // TODO
//   }
// };

struct square_t
{
  motion_t flow {};
  bool isFlowSrc = false;
  substance_t sediment {};
  cell_t* cell = nullptr;
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
      // CHANGED: tanh activation so middle layer is nonlinear
      return std::tanh (sum);
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
    char type = 'a';
    for (node_t<polymer_t>& inputNode : inputLayer)
      inputNode.signal = substance.concentrationOf (type++);

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

    for (node_t<polymer_t>& inputNode : inputLayer)
      inputNode.signal = std::nullopt;
    for (node_t<polymer_t>& middleNode : middleLayer)
      middleNode.signal = std::nullopt;

    return rates;
  }
};

struct cell_t
{
  motion_t velocity;
  long energy = 10;
  substance_t substance {};
  unsigned long timeOfLastXMove = 0;
  unsigned long timeOfLastYMove = 0;
  epigenome_t epigenome { {}, {}, {} };
  square_t* sqr;

  void
  update ()
  {
    // CHANGED: reset velocity each tick so output rates don't accumulate
    velocity.dx = 0;
    velocity.dy = 0;

    auto rates = epigenome.evaluate (sqr->sediment);
    for (auto& [polymer, rate] : rates)
    {
      if (polymer.a ())
        velocity.dx += rate;
      else if (polymer.b ())
        velocity.dy += rate;
    }

    // CHANGED: gain energy from c concentration, lose one per tick
    // cells sitting in c will survive, cells without it will die
    energy += static_cast<long> (sqr->sediment.concentrationOf ('c') * 10.0f);
    --energy;
  }

  void
  randomizeEpigenome (epigenome_t& parentEpigenome)
  {
    // CHANGED: full implementation
    // sizes taken from parent so child topology matches
    const size_t nInput = parentEpigenome.inputLayer.size ();
    const size_t nMiddle = parentEpigenome.middleLayer.size ();
    const size_t nOutput = parentEpigenome.outputLayer.size ();

    // reserve before wiring so pointers never invalidate
    epigenome.inputLayer.clear ();
    epigenome.middleLayer.clear ();
    epigenome.outputLayer.clear ();
    epigenome.inputLayer.reserve (nInput);
    epigenome.middleLayer.reserve (nMiddle);
    epigenome.outputLayer.reserve (nOutput);

    const float mutationStrength = 0.1f;

    auto mutate = [&] (float parentVal, float scale) -> float
    {
      // gaussian-ish mutation via sum of two uniforms, centered on parent value
      float noise = ((float) std::rand () / RAND_MAX - 0.5f) +
                    ((float) std::rand () / RAND_MAX - 0.5f);
      return parentVal + noise * mutationStrength * scale;
    };

    // input layer — copy vals from parent, no wiring needed
    for (size_t i = 0; i < nInput; ++i)
    {
      node_t<polymer_t> node;
      node.val = parentEpigenome.inputLayer[i].val;
      node.bias = mutate (parentEpigenome.inputLayer[i].bias, 1.0f);
      epigenome.inputLayer.push_back (node);
    }

    // middle layer — wire to input layer
    float middleScale = 1.0f / std::sqrt ((float) nInput);
    for (size_t i = 0; i < nMiddle; ++i)
    {
      node_t<polymer_t> node;
      node.val = parentEpigenome.middleLayer[i].val;
      node.bias = mutate (parentEpigenome.middleLayer[i].bias, middleScale);
      node.inputs.reserve (nInput);
      node.weights.reserve (nInput);
      for (size_t j = 0; j < nInput; ++j)
      {
        node.inputs.push_back (&epigenome.inputLayer[j]);
        float parentWeight = parentEpigenome.middleLayer[i].weights[j];
        node.weights.push_back (mutate (parentWeight, middleScale));
      }
      epigenome.middleLayer.push_back (node);
    }

    // output layer — wire to middle layer
    float outputScale = 1.0f / std::sqrt ((float) nMiddle);
    for (size_t i = 0; i < nOutput; ++i)
    {
      node_t<polymer_t> node;
      node.val = parentEpigenome.outputLayer[i].val;
      node.bias = mutate (parentEpigenome.outputLayer[i].bias, outputScale);
      node.inputs.reserve (nMiddle);
      node.weights.reserve (nMiddle);
      for (size_t j = 0; j < nMiddle; ++j)
      {
        node.inputs.push_back (&epigenome.middleLayer[j]);
        float parentWeight = parentEpigenome.outputLayer[i].weights[j];
        node.weights.push_back (mutate (parentWeight, outputScale));
      }
      epigenome.outputLayer.push_back (node);
    }
  }

  void
  randomizeEpigenome ()
  {
    const size_t nInput = 3; // a, b, c
    const size_t nMiddle = 4;
    const size_t nOutput = 2; // dx, dy

    epigenome.inputLayer.clear ();
    epigenome.middleLayer.clear ();
    epigenome.outputLayer.clear ();
    epigenome.inputLayer.reserve (nInput);
    epigenome.middleLayer.reserve (nMiddle);
    epigenome.outputLayer.reserve (nOutput);

    // input layer — a, b, c positionally, no wiring
    for (size_t i = 0; i < nInput; ++i)
    {
      node_t<polymer_t> node;
      node.bias = 0.0f;
      epigenome.inputLayer.push_back (node);
    }

    // middle layer — wired to all inputs
    // bias toward reading c (input index 2) by giving that weight a head start
    float middleScale = 1.0f / std::sqrt ((float) nInput);
    for (size_t i = 0; i < nMiddle; ++i)
    {
      node_t<polymer_t> node;
      node.bias = 0.0f;
      node.inputs.reserve (nInput);
      node.weights.reserve (nInput);
      for (size_t j = 0; j < nInput; ++j)
      {
        node.inputs.push_back (&epigenome.inputLayer[j]);
        // CHANGED: weight for c (j==2) is strongly negative so high c
        // concentration suppresses middle node activation, which will
        // suppress movement in the output layer
        float w = (j == 2) ? -2.0f : middleScale;
        node.weights.push_back (w);
      }
      epigenome.middleLayer.push_back (node);
    }

    // output layer — wired to all middle nodes
    // positive weights mean active middle nodes (low c) produce movement
    float outputScale = 1.0f / std::sqrt ((float) nMiddle);
    for (size_t i = 0; i < nOutput; ++i)
    {
      node_t<polymer_t> node;
      node.val = (i == 0) ? polymer_t { 0x03 << 8 } : polymer_t { 0x03 << 4 };
      node.bias = 0.0f;
      node.inputs.reserve (nMiddle);
      node.weights.reserve (nMiddle);
      for (size_t j = 0; j < nMiddle; ++j)
      {
        node.inputs.push_back (&epigenome.middleLayer[j]);
        // small random sign variation so dx and dy aren't identical,
        // giving the cell some directional diversity to evolve from
        float sign = (std::rand () % 2 == 0) ? 1.0f : -1.0f;
        node.weights.push_back (sign * outputScale);
      }
      epigenome.outputLayer.push_back (node);
    }
  }
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