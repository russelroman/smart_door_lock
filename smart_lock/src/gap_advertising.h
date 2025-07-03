#ifndef GAP_ADVERTISING_H_
#define GAP_ADVERTISING_H_

#include <zephyr/kernel.h>

void advertise_with_acceptlist(struct k_work *work);
void advetising_start(void);


#endif  /* GAP_H_ */