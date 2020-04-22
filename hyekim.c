#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>
#	include <stdbool.h>
#	include <math.h>
#	include <float.h>
#	define MAX_ID 12
#	define MAX_DV 8
#	define SIMULATE_TIME 4
#	define SNAFFLE "SNAFFLE"
#	define WIZARD "WIZARD"
#	define BLUDGER "BLUDGER"
#	define OPPONENT_WIZARD "OPPONENT_WIZARD"
#	define INTERVAL 0.1
#	define MOVE_POWER 150
#	define THROW_POWER 500
#	define MAP_MAX_X 16000
#	define MAP_MAX_Y 7500
#	define POLE_R 300
#	define POLE_UL_X 0
#	define POLE_UL_Y 1750
#	define POLE_UR_X 16000
#	define POLE_UR_Y 1750
#	define POLE_DL_X 0
#	define POLE_DL_Y 5750
#	define POLE_DR_X 16000
#	define POLE_DR_Y 5750
int	g_goal_y = 3750;
int	g_goal_x = -1;
int g_my_goal_x = -1;
int	g_debug = 0;
int g_my_team_id = 0;
int g_wizard_id = -1;
int g_snaffle_id = -1;
int	g_grab_turns_src[4] = {0, 0, 0, 0};
int	g_grab_turns[4] = {0, 0, 0, 0};
int	g_my_score = 0;
int g_op_score = 0;
/**
 * Grab Snaffles and try to throw them through the opponent's goal!
 * Move towards a Snaffle to grab it and use your team id to determine towards where you need to throw it.
 * Use the Wingardium spell to move things around at your leisure, the more magic you put it, the further they'll move.
 **/

typedef struct		s_entity
{
	double			x;
	double			y;
	double			vx;
	double			vy;
	double			f;
	double			m;
	double			dist;
	int				r;
	int				id;
	int				state;
	int				is_goal;
	char			*type;
	struct s_entity	*target;
}					t_entity;

typedef struct	s_event
{
	t_entity	**entities;
	int			dx;
	int			dy;
	double		point;
}				t_event;

double		g_target_x = 0;
double		g_target_y = 0;

double	dist_entity(t_entity *e1, t_entity *e2)
{
	return (hypot(e1->x - e2->x, e1->y - e2->y));
}

double	dist_to_goal(t_entity *entity)
{
	double y;

	if (entity->y <= 2050)
		y = 2050;
	else if (entity->y >= 5450)
		y = 5450;
	else
		y = entity->y;
	return (hypot(entity->x - g_goal_x, entity->y - y));
}

double	sum_g_series(double a, double r, double t)
{
	return (a * (1 - pow(r, t)) / (1 - r));
}

void	thrust(t_entity *entity, double dx, double dy, int power)
{
	entity->vx += power * dx / hypot(dx, dy) / entity->m;
	entity->vy += power * dy / hypot(dx, dy) / entity->m;
}

void	print_entity(t_entity *entity)
{
	fprintf(stderr, "%d %s (%f, %f) (%f, %f)\n", entity->id, entity->type, entity->x, entity->y, entity->vx, entity->vy);
}

t_entity	*init_entity(int id, char *type, double x, double y, double vx, double vy, int state)
{
	t_entity	*entity;

	entity = calloc(1, sizeof(t_entity));
	entity->id = id;
	entity->type = strdup(type);
	entity->x = x;
	entity->y = y;
	entity->vx = vx;
	entity->vy = vy;
	entity->state = state;
	if (strcmp(entity->type, WIZARD) == 0 || strcmp(entity->type, OPPONENT_WIZARD) == 0)
	{
		entity->m = 1;
		entity->f = 0.75;
		entity->r = 400;
	}
	else if (strcmp(entity->type, SNAFFLE) == 0)
	{
		entity->m = 0.5;
		entity->f = 0.75;
		entity->r = 150;
	}
	else if (strcmp(entity->type, BLUDGER) == 0)
	{
		entity->m = 8;
		entity->f = 0.9;
		entity->r = 200;
	}
	else
		printf("init_err\n");
	return (entity);
}

t_entity	*copy_entity(t_entity *entity)
{
	t_entity	*new;

	new = calloc(1, sizeof(t_entity));
	memcpy(new, entity, sizeof(t_entity));
	new->type = strdup(entity->type);
	return (new);
}

t_event	*init_event(t_entity **entities, int id, double dx, double dy, int power)
{
	t_event		*event;
	t_entity	*entity;

	if (!(event = calloc(1, sizeof(t_event))))
		printf("init_err\n");
	event->dx = dx;
	event->dy = dy;
	event->entities = calloc(MAX_ID + 1, sizeof(t_entity *));
	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (entities[i] == NULL)
			continue ;
		entity = copy_entity(entities[i]);
		event->entities[i] = entity;
		entity->target = NULL;
		if (entity->id == id)
			thrust(entity, dx, dy, power);
	}
	return (event);
}

int		goal_cmp(t_entity *entity)
{
	if (strcmp(entity->type, SNAFFLE) != 0)
		return (0);
	if (2050 <= entity->y && entity->y <= 5450)
	{
		if (entity->x < 0 || entity->x > 16000)
		{
			if ((entity->x - 8000) * (g_goal_x - 8000) < 0)
				return (-1);
			return (1);
		}
	}
	return (0);
}

int		is_wall_conflict(t_entity *entity)
{
	if ((strcmp(entity->type, SNAFFLE) == 0 && 2050 <= entity->y && entity->y <= 5450))
		return (0);
	return (entity->x - entity->r < 0 || entity->x + entity->r > MAP_MAX_X
			|| entity->y - entity->r < 0 || entity->y + entity->r > MAP_MAX_Y);
}

void	wall_conflict(t_entity *entity)
{
	if (entity->x - entity->r < 0)
	{
		entity->x += 2 * (entity->r - entity->x);
		entity->vx *= -1;
	}
	else if (entity->y - entity->r < 0)
	{
		entity->y += 2 * (entity->r - entity->y);
		entity->vy *= -1;
	}
	else if (entity->x + entity->r > MAP_MAX_X)
	{
		entity->x -= 2 * (entity->x - (MAP_MAX_X - entity->r));
		entity->vx *= -1;
	}
	else if (entity->y + entity->r > MAP_MAX_Y)
	{
		entity->y -= 2 * (entity->y - (MAP_MAX_Y - entity->r));
		entity->vy *= -1;
	}
}

int		is_grabbable(t_entity *entity1, t_entity *entity2)
{
	double r;
	if ((entity1->id == g_wizard_id && entity2->id == g_snaffle_id) ||
		(entity2->id == g_wizard_id && entity1->id == g_snaffle_id))
		return (0);
	if (strcmp(entity1->type, SNAFFLE) == 0)
	{
		if (((strcmp(entity2->type, WIZARD) != 0) && (strcmp(entity2->type, OPPONENT_WIZARD) != 0)) ||
			g_grab_turns[entity2->id] > 0)
			return (0);
		r = entity2->r;
		if ((strcmp(entity2->type, OPPONENT_WIZARD) == 0))
			r += 300;
		return (hypot(entity1->x - entity2->x, entity1->y - entity2->y) < r - 1);
	}
	else if (strcmp(entity2->type, SNAFFLE) == 0)
	{
		if (((strcmp(entity1->type, WIZARD) != 0) && (strcmp(entity1->type, OPPONENT_WIZARD) != 0)) ||
			g_grab_turns[entity1->id] > 0)
			return (0);
		r = entity1->r;
		if ((strcmp(entity1->type, OPPONENT_WIZARD) == 0))
			r += 300;
		return (hypot(entity1->x - entity2->x, entity1->y - entity2->y) <= r - 1);
	}
	return (0);
}

void	grab(t_entity *entity1, t_entity *entity2)
{
	if (strcmp(entity1->type, SNAFFLE) == 0)
	{
		if ((strcmp(entity2->type, WIZARD) != 0) && (strcmp(entity2->type, OPPONENT_WIZARD) != 0))
			printf("ERR!");
		entity1->state = 1;
		entity2->state = 1;
		entity1->x = entity2->x;
		entity1->y = entity2->y;
		entity1->vx = entity2->vx;
		entity1->vy = entity2->vy;
	}
	if (strcmp(entity2->type, SNAFFLE) == 0)
	{
		if ((strcmp(entity1->type, WIZARD) != 0) && (strcmp(entity1->type, OPPONENT_WIZARD) != 0))
			printf("ERR!");
		entity1->state = 1;
		entity2->state = 1;
		entity2->x = entity1->x;
		entity2->y = entity1->y;
		entity2->vx = entity1->vx;
		entity2->vy = entity1->vy;
	}
	else
		printf("ERR!");
}

int		is_conflict(t_entity *entity1, t_entity *entity2)
{
	if ((strcmp(entity1->type, WIZARD) == 0 && strcmp(entity2->type, OPPONENT_WIZARD) == 0) ||
		(strcmp(entity2->type, WIZARD) == 0 && strcmp(entity1->type, OPPONENT_WIZARD) == 0))
		return (0);
	if (strcmp(entity1->type, SNAFFLE) == 0)
	{
		if ((strcmp(entity2->type, WIZARD) == 0) || (strcmp(entity2->type, OPPONENT_WIZARD) == 0))
			return (0);
	}
	else if (strcmp(entity2->type, SNAFFLE) == 0)
	{
		if ((strcmp(entity1->type, WIZARD) == 0) || (strcmp(entity1->type, OPPONENT_WIZARD) == 0))
			return (0);
	}

	double dist = hypot(entity1->x - entity2->x, entity1->y - entity2->y);
	if (dist == 0)
		return (0);
	return (dist <= entity1->r + entity2->r);
}

void	pole_conflict(t_entity *entity, double pole_x, double pole_y)
{
	double dist = hypot(pole_x - entity->x, pole_y - entity->y);
	double nx = (pole_x - entity->x) / dist;
	double ny = (pole_y - entity->y) / dist;
	double s = entity->r + POLE_R - dist;
	entity->x -= nx * s / 2;
	entity->y -= ny * s / 2;
	double k = -2 * (-entity->vx * nx + -entity->vy * ny) / (1 / entity->m);
	double temp = hypot(2 * nx, 2 * ny);
	if (temp * abs(k) < 100)
	{
		if (k < 0)
			k = 100 / temp * -1;
		else
			k = 100 / temp;
	}
	entity->vx -= k * nx / entity->m;
	entity->vy -= k * ny / entity->m;
}

void	check_pole_conflict(t_entity *entity)
{
	if (hypot(entity->x - POLE_UL_X, entity->y - POLE_UL_Y) <= entity->r + 300)
		pole_conflict(entity, POLE_UL_X, POLE_UL_Y);
	else if (hypot(entity->x - POLE_UR_X, entity->y - POLE_UR_Y) <= entity->r + 300)
		pole_conflict(entity, POLE_UR_X, POLE_UR_Y);
	else if (hypot(entity->x - POLE_DL_X, entity->y - POLE_DL_Y) <= entity->r + 300)
		pole_conflict(entity, POLE_DL_X, POLE_DL_Y);
	else if (hypot(entity->x - POLE_DR_X, entity->y - POLE_DR_Y) <= entity->r + 300)
		pole_conflict(entity, POLE_DR_X, POLE_DR_Y);
}

void	conflict(t_entity *entity1, t_entity *entity2)
{
	double dist = hypot(entity2->x - entity1->x, entity2->y - entity1->y);
	double nx = (entity2->x - entity1->x) / dist;
	double ny = (entity2->y - entity1->y) / dist;
	double s = entity1->r + entity2->r - dist;
	entity1->x -= nx * s / 2;
	entity1->y -= ny * s / 2;
	entity2->x += nx * s / 2;
	entity2->y += ny * s / 2;
	double k = -2 * ((entity2->vx - entity1->vx) * nx + (entity2->vy - entity1->vy) * ny) / (1 / entity1->m + 1 / entity2->m);
	double temp = hypot(2 * nx, 2 * ny);
	if (temp * abs(k) < 100)
	{
		if (k < 0)
			k = 100 / temp * -1;
		else
			k = 100 / temp;
	}
	entity1->vx -= k * nx / entity1->m;
	entity1->vy -= k * ny / entity1->m;
	entity2->vx += k * nx / entity2->m;
	entity2->vy += k * ny / entity2->m;
	if ((strcmp(entity1->type, BLUDGER) == 0) &&
		((strcmp(entity2->type, WIZARD) == 0) || (strcmp(entity2->type, OPPONENT_WIZARD) == 0)))
		entity1->state = entity2->id;
	else if (strcmp(entity2->type, BLUDGER) == 0 &&
		((strcmp(entity1->type, WIZARD) == 0) || (strcmp(entity1->type, OPPONENT_WIZARD) == 0)))
		entity2->state = entity1->id;
}

void	*simulate_entity(t_entity *entity, double time)
{
	t_entity	*new;

	new = copy_entity(entity);
	new->x += sum_g_series(new->vx, new->f, time);
	new->y += sum_g_series(new->vy, new->f, time);
	new->vx *= pow(new->f, time);
	new->vy *= pow(new->f, time);
	new->is_goal = goal_cmp(new);
	if (new->is_goal != 0)
	{
		free(entity);
		return (new);
	}
	if (is_wall_conflict(new))
		wall_conflict(new);
	check_pole_conflict(new);
	return (new);
}

void	simulate_interval(t_event *event)
{
	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (event->entities[i] == NULL || event->entities[i]->is_goal != 0)
			continue ;
		event->entities[i] = simulate_entity(event->entities[i], INTERVAL);
	}
	for (int i = 0; i < MAX_ID + 1 - 1; i++)
	{
		if (event->entities[i] == NULL)
			continue ;
		t_entity *entity1 = event->entities[i];
		for (int j = i + 1; j < MAX_ID + 1; j++)
		{
			if (event->entities[j] == NULL)
				continue ;
			t_entity *entity2 = event->entities[j];
			if (entity1->is_goal != 0 || entity2->is_goal != 0)
				continue;
			if (is_grabbable(entity1, entity2))
			{
				grab(entity1, entity2);
			}
			else if (is_conflict(entity1, entity2))
			{
				conflict(entity1, entity2);
			}
		}
	}
}

int		compare_dist(const void *entity_addr1, const void *entity_addr2)
{
	t_entity	*entity1 = *(t_entity **)entity_addr1;
	t_entity	*entity2 = *(t_entity **)entity_addr2;
	
	double dist1 = hypot(g_target_x - entity1->x - entity1->vx, g_target_y - entity1->y - entity1->vy);
	double dist2 = hypot(g_target_x - entity2->x - entity2->vx, g_target_y - entity2->y - entity2->vy);
	return (dist1 - dist2);
}

void	set_bludgers_target(t_event *event)
{
	double		min_dist;
	double		dist;
	t_entity	*target;
	t_entity	*bludger;

	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (event->entities[i] == NULL)
			continue ;
		if (strcmp(event->entities[i]->type, BLUDGER) == 0)
		{
			min_dist = -1;
			bludger = event->entities[i];
			for (int j = 0; j < MAX_ID + 1; j++)
			{
				if (event->entities[j] == NULL ||
					bludger->state == event->entities[j]->id ||
					(strcmp(event->entities[j]->type, WIZARD) != 0 &&
					strcmp(event->entities[j]->type, OPPONENT_WIZARD) != 0))
					continue ;
				dist = hypot(event->entities[j]->x - bludger->x, event->entities[j]->y - bludger->y);
				if (min_dist < 0 || dist < min_dist)
				{
					target = event->entities[j];
					min_dist = dist;
				}
			}
			thrust(bludger, target->x - bludger->x, target->y - bludger->y, 1000);
		}
	}
}

void	free_entities(t_entity **entities)
{
	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (entities[i] == NULL)
			continue ;
		free(entities[i]);
	}
	free(entities);
}

void	free_event(t_event *event)
{
	free_entities(event->entities);
	free(event);
}

double		simulate_move_event(t_event *event, int time, int wizard_id, int target_id)
{
	double	current;

	current = 0;
	memcpy(g_grab_turns, g_grab_turns_src, sizeof(int) * 4);
	for (int i = 0; i < time; i++)
	{
		set_bludgers_target(event);
		for (int j = 0; j < round(1.0 / INTERVAL); j++)
		{
			current += INTERVAL;
			simulate_interval(event);
			if (event->entities[wizard_id]->x == event->entities[target_id]->x &&
				event->entities[wizard_id]->y == event->entities[target_id]->y)
				// return (-SIMULATE_TIME + current);
				return (-fabs(event->entities[wizard_id]->vx));
		}
		for (int k = 0; k < 4; k++)
			g_grab_turns[k] -= 1;
	}
	return (hypot(event->entities[wizard_id]->x - event->entities[target_id]->x,
			event->entities[wizard_id]->y - event->entities[target_id]->y));

}

t_event	*simulate_move_events(t_entity **entities, t_entity *wizard, t_entity *target)
{
	t_event	*event;
	t_event	*optimal_event;
	double	min_score;
	double	score;

	optimal_event = init_event(entities, wizard->id, target->x + target->vx - wizard->x,
								target->y + target->vy - wizard->y, MOVE_POWER);
	min_score = simulate_move_event(optimal_event, SIMULATE_TIME, wizard->id, target->id);
	for (int dx = MAX_DV * 3000; dx >= -MAX_DV * 3000; dx -= 3000)
	{
		for (int sign = 1; sign >= -1; sign -= 2)
		{
			int dy = sign * (MAX_DV * 3000 - abs(dx));
			event = init_event(entities, wizard->id, dx, dy, MOVE_POWER);
			score = simulate_move_event(event, SIMULATE_TIME, wizard->id, target->id);
			if (score < min_score)
			{
				free_event(optimal_event);
				optimal_event = event;
				min_score = score;
			}
			else
				free_event(event);
		}
	}
	return (optimal_event);
}

t_entity	*get_grabbed_one(t_entity **entities, t_entity *wizard)
{
	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (entities[i] == NULL)
			continue ;
		if (wizard->id != entities[i]->id && wizard->x == entities[i]->x && wizard->y == entities[i]->y)
			return (entities[i]);
	}
	return (NULL);
}

int		get_grabbed_id(t_entity **entities, t_entity *wizard)
{
	for (int i = 0; i < MAX_ID + 1; i++)
	{
		if (entities[i] == NULL)
			continue ;
		if (wizard->id != entities[i]->id && wizard->x == entities[i]->x && wizard->y == entities[i]->y)
			return (entities[i]->id);
	}
	return (-1);
}
double		simulate_throw_event(t_event *event, int time, int snaffle_id, int wizard_id)
{
	double	score;
	double	current;
	t_entity	*snaffle;

	score = dist_to_goal(event->entities[snaffle_id]);
	current = 0;
	g_wizard_id = wizard_id;
	g_snaffle_id = snaffle_id;
	memcpy(g_grab_turns, g_grab_turns_src, sizeof(int) * 4);
	for (int i = 0; i < time; i++)
	{
		set_bludgers_target(event);
		for (int j = 0; j < round(1.0 / INTERVAL); j++)
		{
			current += INTERVAL;
			simulate_interval(event);
		}
		for (int k = 0; k < 4; k++)
			g_grab_turns[k] -= 1;
	}
	g_wizard_id = -1;
	g_snaffle_id = -1;
	snaffle = event->entities[snaffle_id];
	if (snaffle->is_goal != 0)
	{
		return ((hypot(snaffle->vx, snaffle->vy) + 17670) * snaffle->is_goal * 1000);
	}
	score -= dist_to_goal(snaffle);
	if (score > 0 && (snaffle->id == get_grabbed_id(event->entities, event->entities[!g_my_team_id * 2]) ||
		snaffle->id == get_grabbed_id(event->entities, event->entities[!g_my_team_id * 2 + 1])))
		return (round(score * -500));
	if (score > 0 && snaffle->id == get_grabbed_id(event->entities, event->entities[g_my_team_id * 2 + !wizard_id]))
		return (round(score * 5));
	return (score);
};

int		is_match_point_chance(t_entity **snaffles, int snaffle_count)
{
	int is_match_point = (g_my_score == ((snaffle_count + g_my_score + g_op_score) / 2));
	if (is_match_point == 0)
		return (0);
	fprintf(stderr, "MATCH POINT!\n");
	g_target_x = g_goal_x;
	g_target_y = g_goal_y;
	qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
	for (int i = 0; i <snaffle_count; i++)
	{
		t_entity *snaffle = snaffles[i];
		if (abs(g_goal_x - snaffle->x) < 1000 && (3750 - 1600 <= snaffle->y && snaffle->y <= 3750 + 1600))
			return (1);
	}
	return (0);
}

t_event		*simulate_throw_events(t_entity **entities, t_entity *wizard)
{
	t_event	*event;
	t_event	*optimal_event;
	t_entity	*snaffle;
	double	max_score;
	double	score;

	snaffle = get_grabbed_one(entities, wizard);
	optimal_event = init_event(entities, snaffle->id, g_goal_x - snaffle->x,
								g_goal_y - snaffle->y, THROW_POWER);
	max_score = simulate_throw_event(optimal_event, SIMULATE_TIME * 2, snaffle->id, wizard->id);
	for (int dx = MAX_DV * 3000; dx >= -MAX_DV * 3000; dx -= 3000)
	{
		for (int sign = 1; sign >= -1; sign -= 2)
		{
			int dy = sign * (MAX_DV * 3000 - abs(dx));
			event = init_event(entities, snaffle->id, dx, dy, THROW_POWER);
			score = simulate_throw_event(event, SIMULATE_TIME * 2, snaffle->id, wizard->id);
			if (score > max_score)
			{
				free_event(optimal_event);
				optimal_event = event;
				max_score = score;
			}
			else
				free_event(event);
		}
	}
	return (optimal_event);
}

int		compare_target(t_entity *snaffle1, t_entity *snaffle2)
{
	return (0);
}

int		is_snaffle_goal(t_entity *snaffle)
{
	return (0);
}

void	set_wizard_target(t_entity **snaffles, t_entity wizards[])
{
}

int		is_danger(t_entity **snaffles, t_entity *enemy_wizards[], int snaffle_count)
{
	if (abs(enemy_wizards[0]->x - g_my_goal_x) >= 6000 && abs(enemy_wizards[1]->x - g_my_goal_x) >= 6000)
		return (0);
	g_target_x = g_my_goal_x;
	g_target_y = g_goal_y;
	qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
	for (int i = 0; i < snaffle_count; i++)
	{
		t_entity *snaffle = snaffles[i];
		double goal_distance = hypot(snaffle->x - g_my_goal_x, snaffle->y - g_goal_y);
		if (snaffle->vx * (8000 - g_my_goal_x) < 0 && abs(snaffle->vx) > 2 && snaffle->state == 0
			&& (3750 - 2000 <= snaffle->y && snaffle->y <= 3750 + 2000))
		{
			return (goal_distance < 4000 && goal_distance / abs(snaffle->vx) < 4000);
		}
	}
	return (0);
}


void	get_perp_vector(int vx, int vy, int *new_vx, int *new_vy)
{
	*new_vx = -vy;
	*new_vy = vx;
	if ((g_my_team_id == 0 && *new_vx < 0) ||(g_my_team_id == 1 && *new_vx > 0))
	{
		*new_vx *= -1;
		*new_vy *= -1;
	}
}

void	set_early_target(t_entity *snaffles[], int snaffle_count, t_entity *wizards[])
{

}

int main()
{
	int my_team_id;
	scanf("%d", &my_team_id);
	g_my_team_id = my_team_id;
	if (my_team_id == 0)
	{
		g_my_goal_x = 0;
		g_goal_x = 16000;
	}
	else
	{
		g_my_goal_x = 16000;
		g_goal_x = 0;
	}
	int turn;
	turn = 0;
	while (1) {
		turn++;
		int my_magic;
		t_entity	**entities;
		t_entity	*wizards[2];
		t_entity	*enemies[2];
		t_entity	*snaffles[7];
		t_entity 	*candicate;
		t_event		*event;
		int snaffle_count;
		scanf("%d%d", &g_my_score, &my_magic);
		int opponent_magic;
		scanf("%d%d", &g_op_score, &opponent_magic);
		int e_num;
		scanf("%d", &e_num);
		entities = calloc(MAX_ID + 1, sizeof(t_entity *));
		snaffle_count = 0;
		
		for (int i = 0; i < e_num; i++) {
			int			entity_id;
			char		entity_type[21];
			t_entity	*entity;
			int x;
			int y;
			int vx;
			int vy;
			int state;
			scanf("%d%s%d%d%d%d%d", &entity_id, entity_type, &x, &y, &vx, &vy, &state);
			entity = init_entity(entity_id, entity_type, x, y, vx, vy, state);
			entities[entity_id] = entity;
			if (strcmp(entity->type, WIZARD) == 0)
				wizards[entity->id % 2] = entity;
			if (strcmp(entity->type, SNAFFLE) == 0)
			{
				snaffles[snaffle_count] = entity;
				snaffle_count++;
			}
			if (strcmp(entity->type, OPPONENT_WIZARD) == 0)
				enemies[entity->id % 2] = entity;
			if ((strcmp(entity->type, WIZARD) == 0) || (strcmp(entity->type, OPPONENT_WIZARD) == 0))
			{
				if (entity->state == 1)
					g_grab_turns_src[entity->id] = 2;
				else
					g_grab_turns_src[entity->id] -= 1;
			}
		}
		if (turn <= 14)
		{
			if (snaffle_count == 7)
			{
				wizards[0]->target = entities[10];
				wizards[1]->target = entities[10];
			}
			else
			{
				wizards[0]->target = entities[8];
				wizards[1]->target = entities[8];
			}
		}
		else
		{
			g_target_x = (wizards[0]->x);
			g_target_y = (wizards[0]->y);
			qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
			candicate = NULL;
			for (int i = 0; i < snaffle_count; i++)
			{
				t_entity *temp = copy_entity(snaffles[i]);
				for (int j = 0; j < round(4.0 / INTERVAL); j++)
				{
					temp = simulate_entity(temp, INTERVAL);
				}
				if (wizards[0]->target != NULL)
				{
					candicate = snaffles[i];
					break ;
				}
				else if (temp->is_goal == 0 || i == snaffle_count - 1)
				{
					wizards[0]->target = snaffles[i];
				}
			}
			g_target_x = (wizards[1]->x);
			g_target_y = (wizards[1]->y);
			qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
			for (int i = 0; i < snaffle_count; i++)
			{
				t_entity *temp = copy_entity(snaffles[i]);
				for (int j = 0; j < round(4.0 / INTERVAL); j++)
					temp = simulate_entity(temp, INTERVAL);
				if (temp->is_goal == 0 || i == snaffle_count - 1)
				{
					wizards[1]->target = snaffles[i];
					if (wizards[1]->target->id == wizards[0]->target->id)
					{
						if (candicate != NULL &&
							hypot(wizards[0]->x - snaffles[i]->x - snaffles[i]->vx, wizards[0]->y - snaffles[i]->y - snaffles[i]->vy) > 
							hypot(wizards[1]->x - snaffles[i]->x - snaffles[i]->vx, wizards[1]->y - snaffles[i]->y - snaffles[i]->vy))
						{
							wizards[0]->target = candicate;
						}
						else
							continue ;
					}
					break ;
				}
			}
		}
		int is_spelled = 0;
		for (int i = 0; i < 2; i++)
		{
			t_entity *wizard = wizards[i];
			if (wizard->state == 1)
			{
				event = simulate_throw_events(entities, wizard);
				printf("THROW %d %d %d\n", (int)(wizard->x + event->dx),
						(int)(wizard->y + event->dy), THROW_POWER);
				free_event(event);
			}
			else if (my_magic >= 25 && is_danger(snaffles, enemies, snaffle_count) && is_spelled == 0)
			{
				g_target_x = g_my_goal_x;
				g_target_y = g_goal_y;
				qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
				if (my_magic >= 50)
					my_magic = 35;
				for (int j = 0; j < snaffle_count; j++)
				{
					t_entity *snaffle = snaffles[j];
					if (snaffle->vx * (8000 - g_my_goal_x) < 0 && fabs(snaffle->vx) > 2 && snaffle->state == 0
							&& (POLE_UL_Y <= snaffle->y && snaffle->y <= POLE_DL_Y))
					{
						int perp_vx = 0;
						int perp_vy = 0;
						get_perp_vector(snaffle->vx, snaffle->vy, &perp_vx, &perp_vy);
						int fit_magic = (int)hypot(snaffle->vx, snaffle->vy);
						if (fit_magic > my_magic)
							fit_magic = my_magic;
						printf("WINGARDIUM %d %d %d %d\n", snaffle->id, (int)snaffle->x + 4 * perp_vx,
								(int)snaffle->y + perp_vy, fit_magic);
						my_magic -= fit_magic;
						is_spelled = 1;
						break ;
					}
				}
			}
			else if (my_magic >= 20 && is_match_point_chance(snaffles, snaffle_count) && is_spelled == 0)
			{
				g_target_x = g_goal_x;
				g_target_y = g_goal_y;
				qsort(snaffles, snaffle_count, sizeof(t_entity *), compare_dist);
				for (int i = 0; i < snaffle_count; i++)
				{
					t_entity *snaffle = snaffles[i];
					int	y;
					if (abs(g_goal_x - snaffle->x) < 1500 && (3750 - 1600 <= snaffle->y && snaffle->y <= 3750 + 1600))
					{
						is_spelled = 1;
						fprintf(stderr, "hi %d\n", my_magic);
						if (snaffle->y <= 2250)
							y = 2050;
						else if (snaffle->y >= 5250)
							y = 5450;
						else
							y = snaffle->y;
						printf("WINGARDIUM %d %d %d 20\n", snaffle->id, g_goal_x, y);
						break ;
					}
				}
			}
			else if (wizard->state == 0)
			{
				event = simulate_move_events(entities, wizard, wizard->target);
				printf("MOVE %d %d %d\n", (int)(wizard->x + event->dx),
						(int)(wizard->y + event->dy), MOVE_POWER);
				free_event(event);
			}
		}
		free_entities(entities);
	}
	return 0;
}