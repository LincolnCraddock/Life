#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "life_model.h"

/* Forward declarations */

// returns the amount of flow in a cell at a certain position caused by a flow
// source at a position
motion_t
flowFromSrc (point_t pos, point_t src, size_t width, size_t height);

// fairly distribute an amount over some buckets (each with a starting value)
// so that the bucket with the lowest value's value is maximized
//
// returns the value of the bucket with the lowest value, and any left over
// amount that couldn't be distributed evenly
//
// buckets must be non-empty and sorted; the starting values may be negative
std::pair<long, long>
fairDistribute (long amt, const std::vector<long>& buckets);

// move the sediment in each cell according to that cell's flow
void
moveSediment (std::vector<std::vector<square_t>>& world);

// move the sediment in each cell according to its attraction to other sediment
void
groupSediment (std::vector<std::vector<square_t>>& world);

// spread the sediment out in all directions
void
diffuseSediment (std::vector<std::vector<square_t>>& world);

Model::Model (settings_t settings)
    : m_settings (settings),
      m_world (settings.height, std::vector<square_t> (settings.width))
{
  /* Fix invalid settings */
  // TODO: accound for numSqrs overflowing
  unsigned numSqrs = m_settings.width * m_settings.height;
  if (m_settings.numStartCells > numSqrs)
    m_settings.numStartCells = numSqrs;
  if (numSqrs > m_settings.numFlowSinks &&
      m_settings.numFlowSrcs > numSqrs - m_settings.numFlowSinks)
  {
    m_settings.numFlowSinks = 0;
    m_settings.numFlowSrcs = 0;
  }

  /* RNGs */
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
        zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng) : 0,
        zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng) : 0,
        zeroToOne (rng) < m_settings.freqStartSediment ? randSediment (rng) : 0
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
          motion_t m =
            flowFromSrc ({ x, y }, p, m_settings.width, m_settings.height);
          m_world[y][x].flow += m;
        }
        for (point_t p : flowSinks)
        {
          motion_t m =
            flowFromSrc ({ x, y }, p, m_settings.width, m_settings.height);
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
Model::step ()
{
  moveSediment (m_world);
  groupSediment (m_world);
}

const std::vector<std::vector<square_t>>&
Model::world () const
{
  return m_world;
}

const settings_t
Model::settings () const
{
  return m_settings;
}

bool
Model::isInsideWorld (point_t p) const
{
  return p.x < m_settings.width && p.y < m_settings.height;
}

bool
Model::isInsideWorld (rect_t r) const
{
  return r.x1 < m_settings.width && r.y1 < m_settings.height &&
         r.x2 < m_settings.width && r.y2 < m_settings.height;
}

motion_t
flowFromSrc (point_t pos, point_t src, size_t width, size_t height)
{
  const long AFFECTED_RADIUS = (std::min (width, height) - 1) / 2;

  if ((int) (pos.x - src.x) >
      (int) (width / 2)) // TODO: cast to int kind of sucks
    src.x += width;
  else if ((int) (pos.x - src.x) < -(int) (width / 2))
    src.x -= width;
  if ((int) (pos.y - src.y) > (int) (height / 2))
    src.y += height;
  else if ((int) (pos.y - src.y) < -(int) (height / 2))
    src.y -= height;

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

void
moveSediment (std::vector<std::vector<square_t>>& world)
{
  size_t height = world.size ();
  size_t width = height > 0 ? world[0].size () : 0;
  std::vector<std::vector<substance_t>> newSediment (
    world.size (), std::vector<substance_t> (world[0].size ()));

  /* Move sediment */
  for (size_t y = 0; y < world.size (); ++y)
    for (size_t x = 0; x < world[0].size (); ++x)
    {
      const float dx = world[y][x].flow.dx;
      const float dy = world[y][x].flow.dy;
      const int dx1 = floor (dx);
      const int dx2 = floor (dx + 1);
      const int dy1 = floor (dy);
      const int dy2 = floor (dy + 1);
      newSediment[addMod (y, dy1, height)][addMod (x, dx1, width)] +=
        world[y][x].sediment * ((dx1 + 1 - dx) * (dy1 + 1 - dy));
      newSediment[addMod (y, dy2, height)][addMod (x, dx1, width)] +=
        world[y][x].sediment * ((dx1 + 1 - dx) * (dy + 1 - dy2));
      newSediment[addMod (y, dy1, height)][addMod (x, dx2, width)] +=
        world[y][x].sediment * ((dx + 1 - dx2) * (dy1 + 1 - dy));
      newSediment[addMod (y, dy2, height)][addMod (x, dx2, width)] +=
        world[y][x].sediment * ((dx + 1 - dx2) * (dy + 1 - dy2));
    }

  for (size_t x = 0; x < width; ++x)
    for (size_t y = 0; y < height; ++y)
      world[y][x].sediment = newSediment[y][x];
}

void
groupSediment (std::vector<std::vector<square_t>>& world)
{
  size_t height = world.size ();
  size_t width = height > 0 ? world[0].size () : 0;
  std::vector<std::vector<substance_t>> newSediment (
    height, std::vector<substance_t> (width));

  /* Determine weights for each cell */
  std::vector<std::vector<substance_t>> weights (
    height, std::vector<substance_t> (width));
  for (size_t x = 0; x < width; ++x)
    for (size_t y = 0; y < height; ++y)
    {
      substance_t& w = weights[y][x];
      substance_t& s = world[y][x].sediment;
      // lower weight value means more attraction
      w.a = s.b;
      w.b = s.a - s.c;
      w.c = -s.b;
    }

  /* Diffuse sediment */
  for (size_t x = 0; x < width; ++x)
    for (size_t y = 0; y < height; ++y)
    {
      const substance_t sediment = world[y][x].sediment;
      std::vector<long> aWeights (9);
      std::vector<long> bWeights (9);
      std::vector<long> cWeights (9);
      for (int i = 0; i < 9; ++i)
      {
        const substance_t w =
          weights[addMod (y, i / 3 - 1, height)][addMod (x, i % 3 - 1, width)];
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
            weights[addMod (y, i, width)][addMod (x, j, height)];
          newSediment[addMod (y, i, height)][addMod (x, j, width)] +=
            { std::max (amtA - w.a, 0L),
              std::max (amtB - w.b, 0L),
              std::max (amtC - w.c, 0L) };
        }
      newSediment[y][x] += { remainderA, remainderB, remainderC };
    }

  for (size_t x = 0; x < width; ++x)
    for (size_t y = 0; y < height; ++y)
      world[y][x].sediment = newSediment[y][x];
}
