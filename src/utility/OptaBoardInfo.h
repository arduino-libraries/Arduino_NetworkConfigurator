/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <board.h>

inline uint16_t _getPid_() {
	const struct opta_board_info *const info = opta_get_board_info();

	if (info != nullptr) {
		return info->pid;
	}

	return 0;
}
