// Career reward hooks adapted from monscriser/UFC3MONS (licenses/UFC3MONS-MIT.txt).
#pragma once

#include <bit>
#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/ppc/func.h>

REXCVAR_DEFINE_BOOL(ufc3_full_training_rewards, false, "ufc3", "Best grade for automatic career training");
REXCVAR_DEFINE_BOOL(ufc3_full_technique_rewards, false, "ufc3", "Maximum automatic Camp Session technique reward");
REXCVAR_DEFINE_BOOL(ufc3_double_career_creds, false, "ufc3", "Double Creds earned from career fights");

REX_EXTERN(__imp__sub_822FCF38);
REX_EXTERN(__imp__sub_822F8EF0);
REX_EXTERN(__imp__sub_822F5590);

namespace ufc3_career {

template <typename T = uint32_t>
inline T Load(const uint8_t* base, uint32_t addr) {
  T value;
  std::memcpy(&value, base + addr, sizeof(value));
  return std::byteswap(value);
}

inline void Store32(uint8_t* base, uint32_t addr, uint32_t value) {
  value = std::byteswap(value);
  std::memcpy(base + addr, &value, sizeof(value));
}

constexpr uint32_t kCareerManager = 0x8335D4A0;

}  // namespace ufc3_career

extern "C" REX_FUNC(sub_822FCF38) {
  using namespace ufc3_career;
  const uint32_t training = ctx.r3.u32;
  const bool automatic = uint32_t(ctx.lr) == 0x822FD7CC;
  __imp__sub_822FCF38(ctx, base);
  if (!REXCVAR_GET(ufc3_full_training_rewards) || !automatic || !training) return;

  const uint32_t activity = Load(base, training + 4);
  if (activity >= 14 || Load(base, training + 8) != 1 || Load(base, training + 12) > 1) return;

  const uint32_t row = training + activity * 52;
  const auto best = Load<int16_t>(base, row + 74);
  const auto next = Load<int16_t>(base, row + 76);
  const auto last = Load<int16_t>(base, row + 78);
  if (last < 0 || next < last || best < next) return;

  Store32(base, training + 16, 0);
  Store32(base, training + 20, uint32_t(best));
}

extern "C" REX_FUNC(sub_822F8EF0) {
  using namespace ufc3_career;
  const uint32_t session = ctx.r3.u32;
  const bool automatic = ctx.r4.u32 != 0 && uint32_t(ctx.lr) == 0x822FAD84;
  __imp__sub_822F8EF0(ctx, base);
  if (!REXCVAR_GET(ufc3_full_technique_rewards) || !automatic || !session) return;

  const uint32_t manager = Load(base, kCareerManager);
  if (!manager || Load(base, manager + 0x3C) != session) return;
  const uint32_t table = Load(base, manager + 0xDC);
  const uint32_t grade = Load(base, session + 0x5C);
  if (!table || grade > 3 || Load(base, session + 0x58) > 1000) return;

  // The caller awards move XP from the low 16 bits of the selected grade entry.
  const uint32_t xp = table + 0xAE4;
  if ((Load(base, xp) & 0xFFFFu) < (Load(base, xp + grade * 4) & 0xFFFFu)) return;
  Store32(base, session + 0x5C, 0);
  Store32(base, session + 0x58, 1000);
}

extern "C" REX_FUNC(sub_822F5590) {
  using namespace ufc3_career;
  const uint32_t career = ctx.r3.u32;
  if (!career || !REXCVAR_GET(ufc3_double_career_creds)) {
    __imp__sub_822F5590(ctx, base);
    return;
  }

  const uint32_t balance = career + 0xA8;
  const int32_t before = Load<int32_t>(base, balance);
  __imp__sub_822F5590(ctx, base);
  const int32_t after = Load<int32_t>(base, balance);
  if (after <= before) return;

  const uint32_t manager = Load(base, kCareerManager);
  const uint32_t table = manager ? Load(base, manager + 0xDC) : 0;
  if (!table) return;
  const int32_t cap = Load<int32_t>(base, table + 0x1C);
  if (after >= cap) return;
  const int64_t doubled = int64_t(after) + (int64_t(after) - before);
  Store32(base, balance, doubled > cap ? uint32_t(cap) : uint32_t(doubled));
}
