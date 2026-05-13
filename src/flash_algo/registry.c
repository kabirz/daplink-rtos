#include <stddef.h>
#include "flash_blob.h"

extern const program_target_t ama3b1kk_flash;
extern const program_target_t musca_a_flash;
extern const program_target_t musca_b_flash;
extern const program_target_t k20dx_flash;
extern const program_target_t k22f_flash;
extern const program_target_t k28f_flash;
extern const program_target_t k32l3a6_flash;
extern const program_target_t k64f_flash;
extern const program_target_t k66f_flash;
extern const program_target_t k82f_flash;
extern const program_target_t ke15z_flash;
extern const program_target_t ke18f_flash;
extern const program_target_t kl02z_flash;
extern const program_target_t kl03z_flash;
extern const program_target_t kl05z_flash;
extern const program_target_t kl25z_flash;
extern const program_target_t kl26z_flash;
extern const program_target_t kl27z_flash;
extern const program_target_t kl28z_flash;
extern const program_target_t kl43z_flash;
extern const program_target_t kl46z_flash;
extern const program_target_t kl82z_flash;
extern const program_target_t kv58f_flash;
extern const program_target_t kw41z_flash;
extern const program_target_t mimxrt1020_spi_flash_flash;
extern const program_target_t mimxrt1050_hyper_flash_flash;
extern const program_target_t max32620_flash;
extern const program_target_t max32630_flash;
extern const program_target_t max32650_flash;
extern const program_target_t max32666_flash;
extern const program_target_t max32670_flash;
extern const program_target_t max32690_flash;
extern const program_target_t nrf51822_flash;
extern const program_target_t nrf52_flash;
extern const program_target_t m252kg6ae_flash;
extern const program_target_t m263kiaae_flash;
extern const program_target_t m467hjhae_flash;
extern const program_target_t m487jidae_flash;
extern const program_target_t lpc1114_flash;
extern const program_target_t lpc1768_flash;
extern const program_target_t lpc4088_flash;
extern const program_target_t lpc43xx_flash;
extern const program_target_t lpc54018_flash;
extern const program_target_t lpc54114_flash;
extern const program_target_t lpc54608_flash;
extern const program_target_t lpc55s6x_flash;
extern const program_target_t lpc812_flash;
extern const program_target_t lpc824_flash;
extern const program_target_t mimxrt1060_spi_flash_flash;
extern const program_target_t mimxrt1170_spi_flash_flash;
extern const program_target_t rtl8195am_flash;
extern const program_target_t nz32_sc151_flash;
extern const program_target_t stm32f072rb_flash;
extern const program_target_t stm32f103rb_flash;
extern const program_target_t stm32f207zg_flash;
extern const program_target_t stm32f334r8_flash;
extern const program_target_t stm32f401re_flash;
extern const program_target_t stm32f407ve_flash;
extern const program_target_t stm32f411_flash;
extern const program_target_t stm32f429zi_flash;
extern const program_target_t stm32f439zi_flash;
extern const program_target_t stm32f746zg_flash;
extern const program_target_t stm32l082cz_flash;
extern const program_target_t stm32l476rg_flash;
extern const program_target_t stm32l4xx_1024_flash;
extern const program_target_t xdot_l151_flash;

const program_target_t * const flash_algo_table[] = {
    &ama3b1kk_flash,
    &musca_a_flash,
    &musca_b_flash,
    &k20dx_flash,
    &k22f_flash,
    &k28f_flash,
    &k32l3a6_flash,
    &k64f_flash,
    &k66f_flash,
    &k82f_flash,
    &ke15z_flash,
    &ke18f_flash,
    &kl02z_flash,
    &kl03z_flash,
    &kl05z_flash,
    &kl25z_flash,
    &kl26z_flash,
    &kl27z_flash,
    &kl28z_flash,
    &kl43z_flash,
    &kl46z_flash,
    &kl82z_flash,
    &kv58f_flash,
    &kw41z_flash,
    &mimxrt1020_spi_flash_flash,
    &mimxrt1050_hyper_flash_flash,
    &max32620_flash,
    &max32630_flash,
    &max32650_flash,
    &max32666_flash,
    &max32670_flash,
    &max32690_flash,
    &nrf51822_flash,
    &nrf52_flash,
    &m252kg6ae_flash,
    &m263kiaae_flash,
    &m467hjhae_flash,
    &m487jidae_flash,
    &lpc1114_flash,
    &lpc1768_flash,
    &lpc4088_flash,
    &lpc43xx_flash,
    &lpc54018_flash,
    &lpc54114_flash,
    &lpc54608_flash,
    &lpc55s6x_flash,
    &lpc812_flash,
    &lpc824_flash,
    &mimxrt1060_spi_flash_flash,
    &mimxrt1170_spi_flash_flash,
    &rtl8195am_flash,
    &nz32_sc151_flash,
    &stm32f072rb_flash,
    &stm32f103rb_flash,
    &stm32f207zg_flash,
    &stm32f334r8_flash,
    &stm32f401re_flash,
    &stm32f407ve_flash,
    &stm32f411_flash,
    &stm32f429zi_flash,
    &stm32f439zi_flash,
    &stm32f746zg_flash,
    &stm32l082cz_flash,
    &stm32l476rg_flash,
    &stm32l4xx_1024_flash,
    &xdot_l151_flash,
    NULL
};
// Total: 66 algorithms
