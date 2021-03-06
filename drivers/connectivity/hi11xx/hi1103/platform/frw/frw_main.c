

/* 头文件包含 */
#include "frw_main.h"
#include "frw_task.h"
#include "frw_event_main.h"
#include "frw_event_deploy.h"
#include "frw_ext_if.h"
#include "frw_timer.h"

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
#include "oal_hcc_host_if.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_MAIN_C

/* 全局变量定义 */
frw_init_enum_uint16 en_wlan_driver_init_state_etc = FRW_INIT_STATE_BUTT;
OAL_STATIC frw_event_sub_table_item_stru frw_timeout_event_sub_table[FRW_TIMEOUT_SUB_TYPE_BUTT];

/*
  3 函数实现
 */
oal_uint32 frw_event_fsm_register_etc(oal_void)
{
    frw_timeout_event_sub_table[FRW_TIMEOUT_TIMER_EVENT].p_func = frw_timer_timeout_proc_etc;
    frw_event_table_register_etc(FRW_EVENT_TYPE_TIMEOUT, FRW_EVENT_PIPELINE_STAGE_0, frw_timeout_event_sub_table);

    return OAL_SUCC;
}

oal_int32 frw_main_init_etc(oal_void)
{
    oal_uint32 ul_ret;

    frw_set_init_state_etc(FRW_INIT_STATE_START);

    /* 事件管理模块初始化 */
    ul_ret = frw_event_init_etc();
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_FRW, "{frw_main_init_etc:: frw_event_init_etc return err code: %d}", ul_ret);
        return -OAL_EFAIL;
    }

    frw_task_init_etc();

#if (_PRE_MULTI_CORE_MODE_PIPELINE_AMP == _PRE_MULTI_CORE_MODE)
    /* 事件核间部署模块初始化 */
    ul_ret = frw_event_deploy_init_etc();
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_FRW, "{frw_main_init_etc:: frw_event_deploy_init_etc return err code: %d}", ul_ret);
        return -OAL_EFAIL;
    }
#endif

    frw_timer_init_etc(FRW_TIMER_DEFAULT_TIME, frw_timer_timeout_proc_event_etc, 0);

    frw_event_fsm_register_etc();

    /* 启动成功后，输出打印 设置状态始终放最后 */
    frw_set_init_state_etc(FRW_INIT_STATE_FRW_SUCC);

    return OAL_SUCC;
}

oal_void frw_main_exit_etc(oal_void)
{
    /* 卸载定时器 */
    frw_timer_exit_etc();

    /* 卸载事件管理模块 */
    frw_event_exit_etc();

    frw_task_exit_etc();

#if (_PRE_MULTI_CORE_MODE_PIPELINE_AMP == _PRE_MULTI_CORE_MODE)
    /* 事件核间部署模块卸载 */
    frw_event_deploy_exit_etc();
#endif

    /* 卸载成功后在置状态位 */
    frw_set_init_state_etc(FRW_INIT_STATE_START);

    /* 卸载成功后，输出打印 */

    return;
}

/*
 * 函 数 名  : frw_set_init_state_etc
 * 功能描述  : 设置初始化状态
 * 输入参数  : 初始化状态
 */
oal_void frw_set_init_state_etc(frw_init_enum_uint16 en_init_state)
{
    if (en_init_state >= FRW_INIT_STATE_BUTT) {
        OAM_WARNING_LOG1(0, OAM_SF_FRW, "{frw_set_init_state_etc:: en_init_state >= FRW_INIT_STATE_BUTT: %d}",
                         en_init_state);
        return;
    }

    en_wlan_driver_init_state_etc = en_init_state;

    return;
}

/*
 * 函 数 名  : frw_get_init_state_etc
 * 功能描述  : 获取初始化状态
 */
frw_init_enum_uint16 frw_get_init_state_etc(oal_void)
{
    return en_wlan_driver_init_state_etc;
}

/*lint -e578*/ /*lint -e19*/
oal_module_symbol(frw_main_init_etc);
oal_module_symbol(frw_main_exit_etc);

oal_module_symbol(frw_set_init_state_etc);
oal_module_symbol(frw_get_init_state_etc);
