#include "rte_common.h"
#include "rte_mbuf.h"
#include "rte_meter.h"
#include "rte_red.h"
#include "rte_cycles.h"

#include "qos.h"

#define FLOWS_NUM 4

static struct rte_meter_srtcm_params app_srtcm_params[FLOWS_NUM] = {
  {.cir = 160000000, .cbs = 80000, .ebs = 80000},
  {.cir = 80000000, .cbs = 40000, .ebs = 40000},
  {.cir = 40000000, .cbs = 20000, .ebs = 20000},
  {.cir = 20000000, .cbs = 10000, .ebs = 10000}
};

struct rte_meter_srtcm app_srtcm[FLOWS_NUM];

/**
 * srTCM
 */
int
qos_meter_init(void)
{
    /* to do */
    int ret;
    for(int i = 0; i < FLOWS_NUM; i++){
      ret = rte_meter_srtcm_config(&app_srtcm[i], &app_srtcm_params[i]);
      if(ret)
        rte_panic("srtcm init failed!\n");
    }
    return 0;
}

enum qos_color
qos_meter_run(uint32_t flow_id, uint32_t pkt_len, uint64_t time)
{
    /* to do */
    return rte_meter_srtcm_color_blind_check(&app_srtcm[flow_id], time, pkt_len);
}


/**
 * WRED
 */

static struct rte_red_params app_red_params[FLOWS_NUM][3] = {
  {
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9}, //green
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9}, //yellow
    {.min_th = 0, .max_th = 1, .maxp_inv = 10, .wq_log2 = 9}  //red
  },
  {
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 0, .max_th = 1, .maxp_inv = 10, .wq_log2 = 9}
  },
  {
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 0, .max_th = 1, .maxp_inv = 10, .wq_log2 = 9}
  },
  {
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
    {.min_th = 0, .max_th = 1, .maxp_inv = 10, .wq_log2 = 9}
  }
};

static struct rte_red app_reds[FLOWS_NUM][3];
static struct rte_red_config app_red_configs[FLOWS_NUM][3];
unsigned queues[FLOWS_NUM];
static uint64_t last_time;

int
qos_dropper_init(void)
{
    /* to do */
    int tmp_ret;
    for(int i = 0; i < FLOWS_NUM; i++){
      for(int j = 0; j < 3; i++){
        tmp_ret = rte_red_rt_data_init(&app_reds[i][j]);
        if(tmp_ret)
          rte_panic("red data init failed!\n");

        tmp_ret = rte_red_config_init(&app_red_configs[i][j], app_red_params[i][j].wq_log2,
          app_red_params[i][j].min_th, app_red_params[i][j].max_th, app_red_params[i][j].maxp_inv);

        if(tmp_ret)
          rte_panic("red config init failed!\n");
      }
      //init queues
      queues[i] = 0;
    }

    last_time = 0;
    return 0;
}

int
qos_dropper_run(uint32_t flow_id, enum qos_color color, uint64_t time)
{
    /* to do */
    if (time != last_time) {
        for (int i = 0; i < FLOWS_NUM; i++)
            queues[i] = 0;
        last_time = time;
    }

    if(rte_red_enqueue(&app_red_configs[flow_id][color], &app_reds[flow_id][color],
      queues[flow_id], time) == 0){
        queues[flow_id]++;
        return 0;
    }

    return 1;
}
