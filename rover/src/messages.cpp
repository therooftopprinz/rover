#include "messages.hpp"

namespace messages
{

uint8_t msg_size_by_type[7] = {
/* type  0 'A' */   sizeof(msgb_log_info_request),
/* type  1 'B' */   sizeof(msgb_pinmode_request),
/* type  2 'C' */   sizeof(msgb_digital_write_request),
/* type  3 'D' */   sizeof(msgb_digital_read_request),
/* type  4 'E' */   sizeof(msgb_analog_read_request),
/* type  5 'F' */   sizeof(msgb_pwm_init_request),
/* type  6 'G' */   sizeof(msgb_pwm_duty_request),
};

} // namespace messages
