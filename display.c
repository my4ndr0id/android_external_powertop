/*
 * Copyright 2007, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * Authors:
 * 	Arjan van de Ven <arjan@linux.intel.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <ncurses.h>

#include "powertop.h"

static WINDOW *title_bar_window;
static WINDOW *cstate_window;
static WINDOW *wakeup_window;
static WINDOW *acpi_power_window;
static WINDOW *timerstat_window;
static WINDOW *suggestion_window;

static void cleanup_curses(void) {
	werase(stdscr);
	refresh();
	endwin();
}

static void zap_windows(void)
{
	if (title_bar_window) {
		delwin(title_bar_window);
		title_bar_window = NULL;
	}
	if (cstate_window) {
		delwin(cstate_window);
		cstate_window = NULL;
	}
	if (wakeup_window) {
		delwin(wakeup_window);
		wakeup_window = NULL;
	}
	if (acpi_power_window) {
		delwin(acpi_power_window);
		acpi_power_window = NULL;
	}
	if (timerstat_window) {
		delwin(timerstat_window);
		timerstat_window = NULL;
	}
	if (suggestion_window) {
		delwin(suggestion_window);
		suggestion_window = NULL;
	}
}


int maxx, maxy;

int maxtimerstats;
int maxwidth;

void setup_windows(void) 
{
	getmaxyx(stdscr, maxy, maxx);

	zap_windows();	

	title_bar_window = subwin(stdscr, 1, maxx, 0, 0);
	cstate_window = subwin(stdscr, 6, maxx, 2, 0);
	wakeup_window = subwin(stdscr, 1, maxx, 9, 0);
	acpi_power_window = subwin(stdscr, 1, maxx, 10, 0);
	timerstat_window = subwin(stdscr, maxy-15, maxx, 12, 0);
	maxtimerstats = maxy-15  -2;
	maxwidth = maxx - 18;
	suggestion_window = subwin(stdscr, 3, maxx, maxy-3, 0);	
}

void initialize_curses(void) 
{
	initscr();
	keypad(stdscr, TRUE);	/* enable keyboard mapping */
	nonl();			/* tell curses not to do NL->CR/NL on output */
	cbreak();		/* take input chars one at a time, no wait for \n */
	noecho();		/* dont echo input */
	curs_set(0);		/* turn off cursor */
	start_color();

	init_pair(PT_COLOR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
	init_pair(PT_COLOR_HEADER_BAR, COLOR_BLACK, COLOR_WHITE);
	init_pair(PT_COLOR_ERROR, COLOR_BLACK, COLOR_RED);
	init_pair(PT_COLOR_RED, COLOR_BLACK, COLOR_RED);
	init_pair(PT_COLOR_YELLOW, COLOR_BLACK, COLOR_YELLOW);
	init_pair(PT_COLOR_GREEN, COLOR_BLACK, COLOR_GREEN);
	init_pair(PT_COLOR_BRIGHT, COLOR_WHITE, COLOR_BLACK);
	
	bkgd(COLOR_PAIR(PT_COLOR_DEFAULT));

	atexit(cleanup_curses);
}

void show_title_bar(void) 
{
	wattrset(title_bar_window, COLOR_PAIR(PT_COLOR_HEADER_BAR));
	wbkgd(title_bar_window, COLOR_PAIR(PT_COLOR_HEADER_BAR));   
	werase(title_bar_window);

	mvwprintw(title_bar_window, 0, 0,  "PowerTOP version 1.3       (C) 2007 Intel Corporation");

	wrefresh(title_bar_window);
}

void show_cstates(void) 
{
	int i;
	wattrset(cstate_window, COLOR_PAIR(PT_COLOR_DEFAULT));
	wbkgd(cstate_window, COLOR_PAIR(PT_COLOR_DEFAULT));   
	werase(cstate_window);

	for (i=0; i<6; i++)
		mvwprintw(cstate_window, i, 0, "%s", cstate_lines[i]);

	wrefresh(cstate_window);
}


void show_acpi_power_line(double rate, double cap)
{
	wattrset(acpi_power_window, COLOR_PAIR(PT_COLOR_DEFAULT));
	wbkgd(acpi_power_window, COLOR_PAIR(PT_COLOR_DEFAULT));   
	werase(acpi_power_window);
	if (rate > 0) {
		mvwprintw(acpi_power_window, 0, 0, "Power usage (ACPI estimate) : %5.1f W (%3.1f hours left)", rate, cap/rate);
	} else {
		mvwprintw(acpi_power_window, 0, 0, "no ACPI power usage estimate available");
	}
	wrefresh(acpi_power_window);
}

void show_wakeups(double d)
{
	wattrset(wakeup_window, COLOR_PAIR(PT_COLOR_DEFAULT));
	wbkgd(wakeup_window, COLOR_PAIR(PT_COLOR_DEFAULT));   
	werase(wakeup_window);

	wbkgd(wakeup_window, COLOR_PAIR(PT_COLOR_RED));   
	if (d <= 10.0)
		wbkgd(wakeup_window, COLOR_PAIR(PT_COLOR_YELLOW));   
	if (d <= 3.0)
		wbkgd(wakeup_window, COLOR_PAIR(PT_COLOR_GREEN));   
		
	mvwprintw(wakeup_window, 0, 0, "Wakeups-from-idle per second : %4.1f", d);
	wrefresh(wakeup_window);
}

void show_timerstats(int nostats, int ticktime)
{
	int i;
	wattrset(timerstat_window, COLOR_PAIR(PT_COLOR_DEFAULT));
	wbkgd(timerstat_window, COLOR_PAIR(PT_COLOR_DEFAULT));   
	werase(timerstat_window);

	if (!nostats) {
		int counter = 0;
		mvwprintw(timerstat_window, 0, 0, _("Top causes for wakeups:"));
		for (i = 0; i < linehead; i++)
			if (lines[i].count > 0 && counter++ < maxtimerstats) {
				if ((lines[i].count * 1.0 / ticktime) >= 10.0)
					wattrset(title_bar_window, COLOR_PAIR(PT_COLOR_BRIGHT));
				else
					wattrset(title_bar_window, COLOR_PAIR(PT_COLOR_DEFAULT));
				mvwprintw(timerstat_window, i+1, 0," %5.1f%% (%4.1f)   %s ", lines[i].count * 100.0 / linectotal,
						lines[i].count * 1.0 / ticktime, 
						lines[i].string);
				}
	} else {
		if (getuid() == 0) {
			mvwprintw(timerstat_window, 0, 0, _("No detailed statistics available; please enable the CONFIG_TIMER_STATS kernel option\n"));
			mvwprintw(timerstat_window, 1, 0, _("This option is located in the Kernel Debugging section of menuconfig\n"));
			mvwprintw(timerstat_window, 2, 0, _("(which is CONFIG_DEBUG_KERNEL=y in the config file)\n"));
			mvwprintw(timerstat_window, 3, 0, _("Note: this is only available in 2.6.21 and later kernels\n"));
		} else
			mvwprintw(timerstat_window, 0, 0, _("No detailed statistics available; PowerTOP needs root privileges for that\n"));
	}


	wrefresh(timerstat_window);
}

void show_suggestion(char *sug)
{
	wattrset(suggestion_window, COLOR_PAIR(PT_COLOR_DEFAULT));
	wbkgd(suggestion_window, COLOR_PAIR(PT_COLOR_DEFAULT));   
	werase(suggestion_window);
	mvwprintw(suggestion_window, 0, 0, "%s", sug);
	wrefresh(suggestion_window);
}