#ifndef MODULE_NM_H
#define MODULE_NM_H

#include <stdint.h>

void nm_module_init(void);
void nm_task_init(void);
void nm_task_function(void *arg);

/* 手动触发 NetworkRequest / NetworkRelease（测试用） */
void nm_request_network(void);
void nm_release_network(void);

/* 获取当前 NM 状态 */
uint8_t nm_get_state(void);

#endif
