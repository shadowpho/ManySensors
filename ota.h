#pragma once

#include <stdbool.h>



bool check_update(void);

void exec_OTA(void);

//+1 if <3.  If>3 return false and thus backlist it
// also sets FW_version to new version if new and resets
bool ok_incr_update_counter(char fw_version[6]);

void check_accept_new_partition();