#include <algorithm>
#include <cmath>
#include <cstdlib>
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

motion_t
flowFromSrc (point_t pos, point_t src);

class Model
{
public:
  Model (settings_t settings)
      : m_settings (settings),
        m_world (settings.height, std::vector<square_t> (settings.width))
  {
    std::mt19937_64 rng (m_settings.seed);
    std::uniform_real_distribution<float> zeroToOne (0, 1);
    std::uniform_int_distribution<size_t> randX (0, m_settings.width - 1);
    std::uniform_int_distribution<size_t> randY (0, m_settings.height - 1);
    std::uniform_int_distribution<long> randSediment (
      0, m_settings.maxStartSediment);

    /* Generate world */
    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
        m_world[y][x].sediment = {
          zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng)
                                                         : 0,
          zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng)
                                                         : 0,
          zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng)
                                                         : 0
        };

    std::set<point_t> flowSrcs;
    while (flowSrcs.size () < m_settings.numFlowSrcs)
      flowSrcs.insert ({ randX (rng), randY (rng) });
    std::set<point_t> flowSinks;
    while (flowSinks.size () < m_settings.numFlowSinks)
      if (point_t sink { randX (rng), randY (rng) };
          flowSrcs.find (sink) == flowSrcs.end ())
        flowSinks.insert (sink);

    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
      {
        if (flowSrcs.find ({ x, y }) != flowSrcs.end () ||
            flowSinks.find ({ x, y }) != flowSinks.end ())
          m_world[y][x].isFlowSrc = true;
        else
        {
          for (point_t p : flowSrcs)
          {
            motion_t m = flowFromSrc ({ x, y }, p);
            m_world[y][x].flow += m;
          }
          for (point_t p : flowSinks)
          {
            motion_t m = flowFromSrc ({ x, y }, p);
            m_world[y][x].flow -= m;
          }
        }
      }

    for (unsigned i = 0; i < m_settings.numStartCells; ++i)
    {
      point_t loc;
      do
      {
        loc = { randX (rng), randY (rng) };
      } while (m_world[loc.y][loc.x].cell);
      m_world[loc.y][loc.x].cell = new cell_t {};
    }
  }

  void
  step ()
  {
    moveSediment ();
    diffuseSediment ();
  }

  const std::vector<std::vector<square_t>>&
  world () const
  {
    return m_world;
  }

  const settings_t
  settings () const
  {
    return m_settings;
  }

private:
  size_t
  addModWidth (size_t x, int dx)
  {
    return (((int) x + dx) % (int) m_settings.width + m_settings.width) %
           m_settings.width;
  }

  size_t
  addModHeight (size_t y, int dy)
  {
    return (((int) y + dy) % (int) m_settings.height + m_settings.height) %
           m_settings.height;
  }

  // returns the amount of flow in a cell at a certain position caused by a flow
  // source at a position
  motion_t
  flowFromSrc (point_t pos, point_t src)
  {
    const long AFFECTED_RADIUS =
      (std::min (m_settings.width, m_settings.height) - 1) / 2;

    if ((int) (pos.x - src.x) >
        (int) (m_settings.width / 2)) // TODO: cast to int kind of sucks
      src.x += m_settings.width;
    else if ((int) (pos.x - src.x) < -(int) (m_settings.width / 2))
      src.x -= m_settings.width;
    if ((int) (pos.y - src.y) > (int) (m_settings.height / 2))
      src.y += m_settings.height;
    else if ((int) (pos.y - src.y) < -(int) (m_settings.height / 2))
      src.y -= m_settings.height;

    long dx = pos.x - src.x;
    long dy = pos.y - src.y;
    long dist = dx * dx + dy * dy;
    float scale = motion_t::MAX_FLOW *
                  (1 - dist / (float) (AFFECTED_RADIUS * AFFECTED_RADIUS));
    if (dx * dx + dy * dy < AFFECTED_RADIUS * AFFECTED_RADIUS)
      return { ((dx > 0) - (dx < 0)) * dx * dx * scale / dist,
               ((dy > 0) - (dy < 0)) * dy * dy * scale / dist };
    return { 0.0, 0.0 };
  }

  // fairly distribute an amount over some buckets (each with a starting value)
  // so that the bucket with the lowest value's value is maximized
  //
  // returns the value of the bucket with the lowest value, and any left over
  // amount that couldn't be distributed evenly
  //
  // buckets must be non-empty and sorted; the starting values may be negative
  std::pair<long, long>
  fairDistribute (long amt, const std::vector<long>& buckets)
  {
    long lowest = buckets[0];
    long remainder = 0;
    size_t i = 1;
    while (i < buckets.size ())
    {
      long delta = i * (buckets[i] - lowest);
      if (delta > amt)
        break;
      amt -= delta;
      lowest = buckets[i++];
    }
    if (amt > 0)
    {
      lowest += amt / i;
      remainder = amt % i;
    }
    return { lowest, remainder };
  }

  // move the sediment in each cell in m_world according to that cell's flow
  void
  moveSediment ()
  {
    std::vector<std::vector<substance_t>> newSediment (
      m_settings.height, std::vector<substance_t> (m_settings.width));

    /* Move sediment */
    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
      {
        const float dx = m_world[y][x].flow.dx;
        const float dy = m_world[y][x].flow.dy;
        const int dx1 = floor (dx);
        const int dx2 = floor (dx + 1);
        const int dy1 = floor (dy);
        const int dy2 = floor (dy + 1);
        newSediment[addModHeight (y, dy1)][addModWidth (x, dx1)] +=
          m_world[y][x].sediment * ((dx1 + 1 - dx) * (dy1 + 1 - dy));
        newSediment[addModHeight (y, dy2)][addModWidth (x, dx1)] +=
          m_world[y][x].sediment * ((dx1 + 1 - dx) * (dy + 1 - dy2));
        newSediment[addModHeight (y, dy1)][addModWidth (x, dx2)] +=
          m_world[y][x].sediment * ((dx + 1 - dx2) * (dy1 + 1 - dy));
        newSediment[addModHeight (y, dy2)][addModWidth (x, dx2)] +=
          m_world[y][x].sediment * ((dx + 1 - dx2) * (dy + 1 - dy2));
      }

    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
        m_world[y][x].sediment = newSediment[y][x];
  }

  void
  diffuseSediment ()
  {
    std::vector<std::vector<substance_t>> newSediment (
      m_settings.height, std::vector<substance_t> (m_settings.width));

    /* Determine weights for each cell */
    std::vector<std::vector<substance_t>> weights (
      m_settings.height, std::vector<substance_t> (m_settings.width));
    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
      {
        substance_t& w = weights[y][x];
        substance_t& s = m_world[y][x].sediment;
        // lower weight value means more attraction
        w.a = s.b;
        w.b = s.a - s.c;
        w.c = -s.b;
      }

    /* Diffuse sediment */
    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
      {
        const substance_t sediment = m_world[y][x].sediment;
        std::vector<long> aWeights (9);
        std::vector<long> bWeights (9);
        std::vector<long> cWeights (9);
        for (int i = 0; i < 9; ++i)
        {
          const substance_t w =
            weights[addModHeight (y, i / 3 - 1)][addModWidth (x, i % 3 - 1)];
          aWeights[i] = w.a;
          bWeights[i] = w.b;
          cWeights[i] = w.c;
        }
        std::sort (aWeights.begin (), aWeights.end ());
        std::sort (bWeights.begin (), bWeights.end ());
        std::sort (cWeights.begin (), cWeights.end ());
        auto [amtA, remainderA] = fairDistribute (sediment.a, aWeights);
        auto [amtB, remainderB] = fairDistribute (sediment.b, bWeights);
        auto [amtC, remainderC] = fairDistribute (sediment.c, cWeights);

        for (short i = -1; i <= 1; ++i)
          for (short j = -1; j <= 1; ++j)
          {
            const substance_t w =
              weights[addModHeight (y, i)][addModWidth (x, j)];
            newSediment[addModHeight (y, i)][addModWidth (x, j)] +=
              { std::max (amtA - w.a, 0L),
                std::max (amtB - w.b, 0L),
                std::max (amtC - w.c, 0L) };
          }
        newSediment[y][x] += { remainderA, remainderB, remainderC };
      }

    for (size_t x = 0; x < m_settings.width; ++x)
      for (size_t y = 0; y < m_settings.height; ++y)
        m_world[y][x].sediment = newSediment[y][x];
  }

  settings_t m_settings;
  std::vector<std::vector<square_t>> m_world;
};