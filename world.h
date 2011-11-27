#ifndef BRMLIFE__WORLD_H
#define BRMLIFE__WORLD_H

struct world {
	const static int newborn_energy = 5000;
	const static int move_cost = -50;
	const static int attack_cost = -400;
	const static int defense_cost = -200;
	const static int sun_energy = 10;

	const static int dead_body_energy = 2000;
	const static double dead_body_energy_carryover = 0.5;
	const static int dead_decay = -10;
};

#endif
