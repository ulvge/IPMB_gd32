
#include "project_select.h"

void platform_init(void)
{
//    NVIC_SetPriority(FPU_IRQn, APP_IRQ_PRIORITY_LOW);
//    NVIC_EnableIRQ(FPU_IRQn)
}
void BkpDateWrite(bkp_data_register_enum register_number, uint16_t data)
{
    /* enable PMU and BKPI clocks */
    rcu_periph_clock_enable(RCU_BKPI);
    rcu_periph_clock_enable(RCU_PMU);
    /* allow access to BKP domain */
    pmu_backup_write_enable();
    bkp_data_write(register_number, data);
}
uint16_t BkpDateRead(bkp_data_register_enum register_number)
{
    return bkp_data_read(register_number);
}


