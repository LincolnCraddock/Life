#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <set>
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

struct substance_t
{
  static constexpr long MAX_VAL = std::numeric_limits<long>::max ();

  long a = 0;
  long b = 0;
  long c = 0;

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
};

struct cell_t
{
  motion_t motion;
  substance_t substance;
};

struct square_t
{
  motion_t flow;
  bool isFlowSrc;
  substance_t sediment;
  cell_t* cell;
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
};