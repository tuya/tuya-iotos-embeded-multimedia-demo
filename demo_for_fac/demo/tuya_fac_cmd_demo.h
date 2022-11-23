
#ifndef _TUYA_FAC_CMD_DEMO_H
#define _TUYA_FAC_CMD_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_fac_test.h"

int tuya_fac_set_cfg(tuya_FacCfg_t info);

int tuya_exec_cmd(char *cmd, char *out, int size);

void remove_enter(char *str, int size);

int cmd_strcasecmp(const char *s1, const char *s2);

int tuya_fac_aging_loop(void *phdl, char *path, int active);


#ifdef __cplusplus
}
#endif

#endif /* _TUYA_FAC_CMD_DEMO_H */
