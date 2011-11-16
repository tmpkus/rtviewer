/*
 * filtermodule.cc
 *
 *  This file is part of RTViewer.
 *
 *	copyright (c) 2011  Jan Rinze Peterzon (janrinze@gmail.com)
 *
 *  RTViewer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTViewer.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filtermodule.h"
#include <iostream>
using namespace std;

module * modules = NULL;
module * pre_raw_filters = NULL;
module * post_raw_filters = NULL;

void insert_by_rank(module & to_insert, module * & anchor) {
	module * found = anchor;
	module * next = anchor;
	while (next && (next->rank < to_insert.rank)) {
		found = next;
		next = found->next;
	}
	if ((anchor == 0) || (next == anchor)) {
		to_insert.next = anchor;
		anchor = &to_insert;
	} else {
		to_insert.next = found->next;
		found->next = &to_insert;
	}
}

void list_filters(void) {
	module * list, *mods[3] = { pre_raw_filters, post_raw_filters, modules };
	const char * names[3] = { "raw filters:", "post demosaic filters:",
			"filter modules:" };
	for (int i = 0; i < 3; i++) {
		list = mods[i];
		cout << names[i] << endl;
		while (list) {
			cout << "  filter: " << list->name << " rank: " << list->rank << endl;
			list = list->next;
		}
	}
}

void addmodule(module & moduleinfo) {
	insert_by_rank(moduleinfo,modules);
}

void add_prerawfilter(module & moduleinfo) {
	insert_by_rank(moduleinfo,pre_raw_filters);
}
