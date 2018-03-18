#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

uint32_t
calculate_scoop(const uint64_t height, const uint8_t *gensig);

void
calculate_deadlines_sse4(
          uint32_t scoop,  uint64_t base_target, uint8_t* gensig, bool poc2,

          uint64_t addr1,  uint64_t addr2,  uint64_t addr3,  uint64_t addr4,

          uint64_t nonce1, uint64_t nonce2, uint64_t nonce3, uint64_t nonce4,

          uint64_t* deadline1,uint64_t* deadline2,uint64_t* deadline3,uint64_t* deadline4);

void
calculate_deadlines_avx2(
          bool poc2,

          uint64_t base_target1, uint64_t base_target2, uint64_t base_target3, uint64_t base_target4,
          uint64_t base_target5, uint64_t base_target6, uint64_t base_target7, uint64_t base_target8,

          uint32_t scoop_nr1, uint32_t scoop_nr2, uint32_t scoop_nr3, uint32_t scoop_nr4,
          uint32_t scoop_nr5, uint32_t scoop_nr6, uint32_t scoop_nr7, uint32_t scoop_nr8,

          uint8_t *gensig1, uint8_t *gensig2, uint8_t *gensig3, uint8_t *gensig4,
          uint8_t *gensig5, uint8_t *gensig6, uint8_t *gensig7, uint8_t *gensig8,

          uint64_t addr1,  uint64_t addr2,  uint64_t addr3,  uint64_t addr4,
          uint64_t addr5,  uint64_t addr6,  uint64_t addr7,  uint64_t addr8,

          uint64_t nonce1, uint64_t nonce2, uint64_t nonce3, uint64_t nonce4,
          uint64_t nonce5, uint64_t nonce6, uint64_t nonce7, uint64_t nonce8,

          uint64_t* deadline1,uint64_t* deadline2,uint64_t* deadline3,uint64_t* deadline4,
          uint64_t* deadline5,uint64_t* deadline6,uint64_t* deadline7,uint64_t* deadline8);

#ifdef __cplusplus
}
#endif
