typedef stuct mc_hole {
  long time;
  u_int32 start,end;
  struct mc_hole *next;
} Mc_Hole;

typedef struct mc_lossState {
  int lostPkts;
  long timeMean,timeVar;
  u_int32 nextSeq;
  Mc_Hole *tail, *head;
} Mc_LossState;

typedef union mc_congstate {
  Mc_LossState  *lossState;
} Mc_CongState;
