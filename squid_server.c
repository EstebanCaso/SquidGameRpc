#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_PLAYERS  9
#define BRIDGE_COUNT NUM_BRIDGES
#define STEP_COUNT   NUM_STEPS
#define CLOCK_SECS   GAME_TIME

typedef struct {
    int player_id, bridge_id, position, alive;
} Player;

typedef struct {
    int broken  [STEP_COUNT+1];
    int weak    [STEP_COUNT+1];
    int occupied[STEP_COUNT+1];
} Bridge;

static Bridge  g_bridges[BRIDGE_COUNT+1];
static Player  g_players[MAX_PLAYERS];
static int     g_num_players = 0;
static int     g_survivors   = 0;
static int     g_game_over   = 0;
static time_t  g_start_time  = 0;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int time_left(void) {
    if (!g_start_time) return CLOCK_SECS;
    int left = CLOCK_SECS - (int)(time(NULL) - g_start_time);
    return left > 0 ? left : 0;
}

static void init_game(void) {
    srand((unsigned)time(NULL));
    for (int b=1;b<=BRIDGE_COUNT;b++) {
        memset(&g_bridges[b],0,sizeof(Bridge));
        for (int s=1;s<=STEP_COUNT;s++)
            g_bridges[b].weak[s] = (rand()%10<4) ? 1 : 0;
    }
    g_start_time = time(NULL);
    printf("[SERVER] Game started. Bridges=%d Steps=%d Time=%ds\n",
           BRIDGE_COUNT, STEP_COUNT, CLOCK_SECS);
    for (int b=1;b<=BRIDGE_COUNT;b++) {
        printf("  Bridge %d pattern: ",b);
        for (int s=1;s<=STEP_COUNT;s++)
            printf("[%s]", g_bridges[b].weak[s]?"WEAK":"SAFE");
        printf("\n");
    }
}

join_res *join_game_1_svc(join_req *req, struct svc_req *rq) {
    static join_res res;
    pthread_mutex_lock(&g_lock);
    if (!g_start_time) init_game();
    res.time_left = time_left();
    if (g_game_over || g_num_players>=MAX_PLAYERS ||
        req->bridge_id<1 || req->bridge_id>BRIDGE_COUNT) {
        res.player_id=-1;
    } else {
        int pid=g_num_players++;
        g_players[pid]=(Player){pid,req->bridge_id,0,1};
        res.player_id=pid;
        printf("[SERVER] Player %d joined bridge %d\n",pid,req->bridge_id);
    }
    pthread_mutex_unlock(&g_lock);
    return &res;
}

move_res *make_move_1_svc(move_req *req, struct svc_req *rq) {
    static move_res res;
    pthread_mutex_lock(&g_lock);
    memset(&res,0,sizeof(res));
    res.survivors=g_survivors;
    res.time_left=time_left();

    if (req->player_id<0||req->player_id>=g_num_players) {
        res.new_position=-2; res.alive=0;
        pthread_mutex_unlock(&g_lock); return &res;
    }
    Player *p=&g_players[req->player_id];
    if (!p->alive||g_game_over||res.time_left==0) {
        if (!p->alive||res.time_left==0) { p->alive=0; if(res.time_left==0) g_game_over=1; }
        res.alive=0; res.new_position=p->position;
        pthread_mutex_unlock(&g_lock); return &res;
    }
    if (req->steps<1||req->steps>2) {
        res.new_position=-2; res.alive=1;
        pthread_mutex_unlock(&g_lock); return &res;
    }

    int target=p->position+req->steps;
    Bridge *br=&g_bridges[p->bridge_id];

    if (target<=STEP_COUNT && br->occupied[target]) {
        res.new_position=p->position; res.alive=1;
        printf("[SERVER] Player %d blocked at step %d – wait.\n",p->player_id,target);
        pthread_mutex_unlock(&g_lock); return &res;
    }

    if (p->position>=1&&p->position<=STEP_COUNT)
        br->occupied[p->position]=0;

    if (target>STEP_COUNT) {
        p->position=STEP_COUNT+1; p->alive=1; g_survivors++;
        res.new_position=p->position; res.alive=1; res.reached_end=1;
        res.survivors=g_survivors;
        printf("[SERVER] Player %d CROSSED bridge %d! Survivors=%d\n",
               p->player_id,p->bridge_id,g_survivors);
    } else if (br->weak[target]) {
        br->broken[target]=1; p->alive=0; p->position=target;
        res.new_position=target; res.alive=0;
        printf("[SERVER] Player %d hits WEAK step %d on bridge %d – DEAD\n",
               p->player_id,target,p->bridge_id);
    } else {
        br->occupied[target]=p->player_id+1;
        p->position=target;
        res.new_position=target; res.alive=1;
        printf("[SERVER] Player %d -> step %d on bridge %d – safe\n",
               p->player_id,target,p->bridge_id);
    }
    res.survivors=g_survivors; res.time_left=time_left();
    pthread_mutex_unlock(&g_lock); return &res;
}

bridge_view_res *view_bridge_1_svc(bridge_view_req *req, struct svc_req *rq) {
    static bridge_view_res res;
    static step_info buf[STEP_COUNT+1];
    pthread_mutex_lock(&g_lock);
    memset(&res,0,sizeof(res));
    res.time_left=time_left(); res.survivors=g_survivors;
    if (req->bridge_id>=1&&req->bridge_id<=BRIDGE_COUNT) {
        Bridge *br=&g_bridges[req->bridge_id];
        for (int s=1;s<=STEP_COUNT;s++) {
            buf[s-1].position=s;
            buf[s-1].broken  =br->broken[s];
            buf[s-1].occupied=(br->occupied[s]!=0)?1:0;
        }
        res.num_steps=STEP_COUNT;
        res.steps.steps_len=STEP_COUNT;
        res.steps.steps_val=buf;
    }
    pthread_mutex_unlock(&g_lock); return &res;
}

status_res *get_status_1_svc(void *req, struct svc_req *rq) {
    static status_res res;
    pthread_mutex_lock(&g_lock);
    res.time_left=time_left();
    res.survivors=g_survivors;
    res.game_over=g_game_over||(res.time_left==0);
    pthread_mutex_unlock(&g_lock); return &res;
}

int squid_prog_1_freeresult(SVCXPRT *t, xdrproc_t xf, caddr_t r) {
    xdr_free(xf,r); return 1;
}
