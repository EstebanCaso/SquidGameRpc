
const NUM_BRIDGES = 3;
const NUM_STEPS   = 6;
const GAME_TIME   = 500;

struct join_req {
    int bridge_id;
};

struct join_res {
    int player_id;
    int time_left;
};

struct move_req {
    int player_id;
    int bridge_id;
    int steps;
};

struct move_res {
    int new_position;
    int alive;
    int reached_end;
    int survivors;
    int time_left;
};

struct bridge_view_req {
    int bridge_id;
};

struct step_info {
    int position;
    int broken;
    int occupied;
};

struct bridge_view_res {
    step_info steps<8>;
    int num_steps;
    int time_left;
    int survivors;
};

struct status_res {
    int time_left;
    int survivors;
    int game_over;
};

program SQUID_PROG {
    version SQUID_VERS {
        join_res    JOIN_GAME   (join_req)           = 1;
        move_res    MAKE_MOVE   (move_req)           = 2;
        bridge_view_res VIEW_BRIDGE(bridge_view_req) = 3;
        status_res  GET_STATUS  (void)               = 4;
    } = 1;
} = 0x31230001;
