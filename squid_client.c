
#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOVE_DELAY_MS 800


static void print_bridge(bridge_view_res *bv, int bridge_id, int my_pos)
{
    printf("\n");
    printf("  ╔══════════════════════════════════════════╗\n");
    printf("  ║  Bridge %d   |  Time left: %3ds  |  Survivors: %d\n",
           bridge_id, bv->time_left, bv->survivors);
    printf("  ╚══════════════════════════════════════════╝\n");
    printf("  [A] ");
    for (int i = 0; i < (int)bv->steps.steps_len; i++) {
        step_info *s = &bv->steps.steps_val[i];
        if (s->position == my_pos)
            printf("[*ME*]");
        else if (s->broken)
            printf("[DEAD]");
            else if (s->occupied)
                printf("[WAIT]");
                else
                    printf("[    ]");
    }
    printf(" [B]\n\n");
}

static int is_broken(bridge_view_res *bv, int pos)
{
    if (!bv) return 0;
    for (int i = 0; i < (int)bv->steps.steps_len; i++) {
        step_info *s = &bv->steps.steps_val[i];
        if (s->position == pos && s->broken)
            return 1;
    }
    return 0;
}

static int is_occupied(bridge_view_res *bv, int pos)
{
    if (!bv) return 0;
    for (int i = 0; i < (int)bv->steps.steps_len; i++) {
        step_info *s = &bv->steps.steps_val[i];
        if (s->position == pos && s->occupied)
            return 1;
    }
    return 0;
}

static int decide_jump(bridge_view_res *bv, int my_pos)
{
    int next1 = my_pos + 1;
    int next2 = my_pos + 2;

    int broken1   = is_broken(bv, next1);
    int broken2   = is_broken(bv, next2);
    int occupied1 = is_occupied(bv, next1);
    int occupied2 = is_occupied(bv, next2);

    if (broken1 && broken2) {
        printf(" Both next steps are broken, trapped, waiting...\n");
        return 0;
    }

    if (broken1 && !broken2) {
        printf("  Step %d is broken, jumping 2\n", next1);
        return 2;
    }

    if (occupied1 && !broken2) {
        printf("  Step %d is blocked by a player, jumping 2!\n", next1);
        return 2;
    }

    if (occupied1 && (broken2 || occupied2)) {
        printf("  Step %d blocked and step %d unsafe, waiting...\n",
               next1, next2);
        return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <host> <bridge_id 1-3> [name]\n", argv[0]);
        exit(1);
    }

    char *host      = argv[1];
    int   bridge_id = atoi(argv[2]);
    char *name      = (argc >= 4) ? argv[3] : "Player";

    if (bridge_id < 1 || bridge_id > NUM_BRIDGES) {
        fprintf(stderr, "bridge_id must be 1..%d\n", NUM_BRIDGES);
        exit(1);
    }

    CLIENT *cl = clnt_create(host, SQUID_PROG, SQUID_VERS, "tcp");
    if (!cl) { clnt_pcreateerror(host); exit(1); }

    printf("\n");
    printf("  ╔══════════════════════════════════════════╗\n");
    printf("  ║       SQUID GAME — GLASS BRIDGE          ║\n");
    printf("  ╚══════════════════════════════════════════╝\n");
    printf("  Player : %s\n", name);
    printf("  Host   : %s\n", host);
    printf("  Bridge : %d\n\n", bridge_id);

    join_req jreq = { bridge_id };
    join_res *jres = join_game_1(&jreq, cl);
    if (!jres || jres->player_id < 0) {
        fprintf(stderr, "  ERROR Could not join game.\n");
        clnt_destroy(cl);
        exit(1);
    }

    int my_id = jres->player_id;
    printf("  %s joined as player #%d on bridge %d (time=%ds)\n\n",
           name, my_id, bridge_id, jres->time_left);

    int my_pos  = 0;
    int alive   = 1;
    int crossed = 0;

    while (alive && !crossed) {
        usleep(MOVE_DELAY_MS * 1000);

        status_res *sr = get_status_1(NULL, cl);
        if (!sr || sr->game_over || sr->time_left == 0) {
            printf("  %s: TIME'S UP, all remaining players die\n", name);
            break;
        }
        printf("  Clock: %ds remaining | Survivors so far: %d\n",
               sr->time_left, sr->survivors);

        bridge_view_req vreq = { bridge_id };
        bridge_view_res *bv  = view_bridge_1(&vreq, cl);
        if (bv && bv->steps.steps_len > 0)
            print_bridge(bv, bridge_id, my_pos);

        int jump = decide_jump(bv, my_pos);

        if (jump == 0) {
            printf("  %s is waiting...\n", name);
            continue;
        }

        move_req mreq = { my_id, bridge_id, jump };
        move_res *mr  = make_move_1(&mreq, cl);
        if (!mr) {
            fprintf(stderr, " ERROR RPC call failed.\n");
            break;
        }

        if (mr->new_position == my_pos) {
            printf("  %s: step blocked by another player, waiting...\n", name);
            continue;
        }

        my_pos = mr->new_position;

        if (!mr->alive) {
            printf("\n  ☠ %s: DEAD at step %d, weak glass!\n\n",
                   name, my_pos);
            alive = 0;
        } else if (mr->reached_end) {
            printf("\n  %s: CROSSED THE BRIDGE! Total survivors: %d\n\n",
                   name, mr->survivors);
            crossed = 1;
        } else {
            printf("  %s: moved to step %d (t=%ds)\n",
                   name, my_pos, mr->time_left);
        }
    }

    clnt_destroy(cl);
    return 0;
}
